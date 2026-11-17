# Plan: from a deep call tree to request pipelines

[Back to readme](../../../Readme.md)

This plan follows on from the [proposal](./request-pipeline-proposal.md). The proposal describes the problem using GetMap. This document looks at all frequently used request types, answers the question "one pipeline or several?", describes the final design, and lists the steps to get there. It is meant for discussion in the team.

Paths are relative to `adagucserverEC/`. Line numbers refer to `master` at `36989f97` (version 10.0.0).

## Summary

- **A middle ground, not one pipeline and not one per request.** We build a shared *library of stages* (resolve layers, resolve dimensions, load header, read data, statistics, ...). On top of that, each request family gets a short pipeline function of 20 to 40 lines that lists its stages in order. The requests share the front half and differ at the end: render, resample into a coverage, sample values, draw a legend, or write XML.
- **There are four families:** *render* (GetMap, GetLegendGraphic), *resample* (GetCoverage), *sample* (GetFeatureInfo, GetPointValue, GetTrajectory, and later an EDR data call), and *catalogue* (GetCapabilities, DescribeCoverage, the GetMetadata variants). Some requests use no data at all: GetScalebar, OpenDAP, and `SERVICE=METADATA`. They only share the front end and the error handling.
- **The central new idea is a `ReadWindow`.** GetMap (the visible extent), WCS (`optimizeextent`), GetFeatureInfo (a single cell) and the EDR time series (hyperslabs) all read "part of a grid", each in its own way. One explicit read window makes them a single read stage. It also makes reading only the visible extent possible.
- **Recommended route: change the code in place, step by step, keeping the output identical.** We don't build a second pipeline next to the old one. There is one exception: the GetFeatureInfo end stage, which already has two implementations today. It can be rebuilt next to the old one and compared.
- **Two tracks that hardly conflict.** The front end (`process_querystring`, errors) and the data path (`CDataReader`, `CDataSource`, writers) can be done in parallel, by different people.
- **Size:** about 30 PRs. Each one can be merged on its own, and each one leaves the code better than before, even if we stop halfway.

## 1. What the requests do today

All data requests start the same way, with `process_querystring` (`CRequest.cpp:802`), then `process_all_layers` (`:596`), then `addDataSources`, then `determineTypesForDataSources`, which also resolves dimensions against the database. After that they split up, and each goes into its own writer.

| Request (tests) | Layers + DB | How data is read | End stage | Output |
|---|---|---|---|---|
| GetMap (191) | yes | `open(OPEN_ALL)` per layer and time step in `warpImage` (`CImageDataWriter.cpp:897`) | choose a renderer, draw, overlays (`handleGetMapRequest`, `CRequest.cpp:2196`) | PNG/WebP |
| GetLegendGraphic (17) | yes | its own `OPEN_ALL` or `OPEN_HEADER` in `CLegendRenderers/CCreateLegend.cpp:93/98`, with its own min/max logic | draw the legend | PNG |
| GetCoverage (31) | yes, dimensions resolved twice (`CRequest.cpp:2470`) | `OPEN_HEADER`, then `OPEN_ALL` or `OPEN_EXTENT` in the writer | the destination grid is computed *inside* the writer's `init()`, then warped into one large output buffer | NetCDF / GeoTIFF / AAIGrid |
| GetFeatureInfo (45), GetPointValue (10), GetTrajectory (3) | yes | **two read paths**: a single-cell `open(ALL, x, y)` per step, or direct hyperslab reads that bypass `CDataReader` (`CURUniqueRequests.cpp`) | sample, then format everything in `end()` (`CImageDataWriter.cpp:1464`, about 500 lines) | HTML / text / JSON / GML |
| GetHistogram (0) | yes | `OPEN_ALL`, twice | warp to the request grid, then histogram | JSON |
| GetCapabilities WMS/WCS (78) | no | metadata cache in the DB, otherwise `OPEN_HEADER` / `OPEN_DIMENSIONS` (`XMLGenUtils.cpp:107`) | build the XML | XML |
| DescribeCoverage (9) | creates layers, then returns before the dimensions are resolved | same as GetCapabilities | build the XML | XML |
| GetMetadata text / JSON (15) | text: yes / JSON: no | text: `OPEN_HEADER` + `CDF::dump`; JSON: blobs from the DB | | text / JSON |
| GetReferenceTimes (0) | yes (a full dimension resolution, although only one table is needed) | none | | JSON |
| GetScalebar, OpenDAP, `SERVICE=METADATA` | no | none / raw header + `readData` / raw header | | PNG / DAP / JSON |

The Python side: EDR position uses `GetPointValue`, EDR trajectory uses `GetTrajectory`, and EDR cube uses WCS `GetCoverage`. All three use `info_format=application/json` or NetCDF, and the CoverageJSON is made in Python.

Findings that matter for the design:

1. **GetPointValue and GetTrajectory are not separate request types in C++.** During parsing they are rewritten into GetFeatureInfo (`CRequest.cpp:1588-1608` and `:1365`), with a fake 1x1 BBOX. So the "sample" family has one request with three kinds of *location*: a pixel, a point, or a trajectory.
2. **GetCoverage is "GetMap without the rendering" only for the first half.** Parsing, layers, dimensions, `CDataReader::open` and the `GenericDataWarper` are shared. The *destination grid* is computed separately, and each WCS writer (`CNetCDFDataWriter` for NetCDF, `CGridFileDataWriter` for GeoTIFF and AAIGrid) has its own copy of that code, with different rounding. All time steps also go into one file, so the output buffer is held in memory as a whole.
3. **Tiles are modelled as time steps** (`handleTileRequest`, inside `queryDimValuesForDataSource`). `liveupdate` and `grid` layers get fake steps with an empty filename. A "step" is therefore not always a time step, but in general a *unit of work*: a file plus indices, a tile, or a virtual step.
4. **The style also plays a part in requests that don't draw anything.** `CDataReader::open` calls `getStyle()` (`CDataReader.cpp:879`). As a result, post-processors configured in the style also run for WCS and GetFeatureInfo, and statistics are computed even when nobody uses them.
5. **The request type changes the geometry.** `parseDimensions` returns early for GetFeatureInfo (`CDataReader.cpp:664`), and so skips the longitude wrap detection. The reader therefore depends on the request.
6. **Format converters (13 of them) draw into the viewport.** Their "grid" is the output image (`srvParams->geoParams`). They are not a header step but a combined "read and rasterise" step. There are two `if` chains (`CDFObjectStore.cpp:311-380` and `CDataReader.cpp:372-398`) that must stay in the same order, and nothing enforces that.
7. **Post-processors (18 of them) receive the whole `CDataSource`** and can do anything with it: add or remove data objects, change the geometry, or open other layers themselves (`IncludeLayer`, `AddFeatures`).

## 2. One pipeline, one per request, or a middle ground?

**One pipeline for everything doesn't fit.** The requests differ in more than their last step:

- the *unit of work*: GetMap works per (layer, step), GetFeatureInfo per layer over all steps at once, and WCS per (layer, step) but into one shared buffer;
- *what is read*: a visible extent, a single cell, hyperslabs, or nothing at all;
- the *output*: an image, a file, text, or XML.

A pipeline that covers all of this needs flags and modes again, which is the same problem `CDataReader::open(mode)` has now.

**A completely separate pipeline per request type doesn't fit either.** About two thirds of the work is the same: parsing, resolving layers and styles, resolving dimensions, opening the header, reading, post-processors, and statistics. Separate pipelines would copy that, as the two WCS writers already copy the destination-grid code.

**The middle ground:** a set of stages with explicit input and output types, plus a short pipeline function per family that calls those stages in order. The pipeline function is what you read to understand a request. The stages are what you test and reuse.

```
                ┌─ render    : GetMap, GetLegendGraphic
front end ──────┼─ resample  : GetCoverage (later also tile generation)
(parse,         ├─ sample    : GetFeatureInfo / PointValue / Trajectory (later EDR data call)
 normalise,     ├─ catalogue : GetCapabilities, DescribeCoverage, GetMetadata, GetReferenceTimes
 dispatch)      └─ no data   : GetScalebar, OpenDAP, SERVICE=METADATA
```

## 3. The final design

### 3.1 Stages and their types

| Stage | Input → output | Today |
|---|---|---|
| `parseRequest` | query string → `ParsedRequest` (raw values per parameter) | the parameter loop in `process_querystring` (`CRequest.cpp:899-1284`) |
| `normaliseRequest` | `ParsedRequest` + config → `RequestContext` (kind, location, geometry, dimensions, format, `ErrorPolicy`) | the rewrites for GetPointValue and Trajectory, `WCS_GoNative`, the BBOX axis swap, `setErrorMode` |
| `configureSources` | `RequestContext` + config → config with extra layers | `CAutoResource::configure` (`source=`). `dataset=` is handled earlier, in `ConfigurationUtils.cpp`. |
| `resolveLayers` | names → `LayerPlan[]`: config, layer type, **resolved style**, **list of post-processors** | `addDataSources`, `setCFGLayer`, the lazy `getStyle()`, `getProcessorList` |
| `resolveSteps` | `LayerPlan` + dimensions + `DimensionIndex` (DB) → `StepList` (file or tile or virtual, plus indices) | `setDimValuesForDataSource`, `handleTileRequest`, the liveupdate and grid fake steps |
| `loadHeader` | `LayerPlan` + `Step` → `SourceHeader`: native `GridGeometry` (immutable), `AxisInfo`, variables, `ConverterKind` | the first half of `open`, `parseDimensions` |
| `chooseReadWindow` | `SourceHeader` + target → `ReadWindow`: full, pixel extent, cell, or hyperslab runs, plus stride | the locals `start/count/stride`, `gridExtent`, `(x, y)`, `sortAndAggregate` |
| `readData` | `SourceHeader` + `Step` + `ReadWindow` → `DataGrid` (buffers, nodata, units, **geometry of what is in memory**), including scale/offset and post-processors | the second half of `open`, or a converter |
| `computeStatistics` | `DataGrid` → `Statistics` | `CDataReader.cpp:1296-1303` |
| `finaliseStyle` | style + `Statistics` → `FinalStyle` (stretched legend, feature intervals, default vectors) | `stretchLegend` in the reader, and the style changes in `warpImage` (`CImageDataWriter.cpp:916-967`, `:1010-1035`) |
| end stages | `renderLayer`, `resampleInto`, `sample`, `drawLegend`, `layerMetadata` | `warpImage`, the WCS writers' `addData`, `getFeatureInfo`, `createLegend`, `populateMetadataLayerStruct` |
| `encode` | result → bytes on stdout, **the only stage that writes** | `CImageDataWriter::end`, the writers' `end()`, the XML generators |

The types are deliberately value types with a clear owner. `GridGeometry` already half exists as `GeoParameters` (with `setGeo`/`makeGeoParams`). `Statistics` already exists as a separate type (`utils/minMax.h`).

### 3.2 The pipelines (target, in pseudocode)

```cpp
// render family
int runGetMap(RequestContext &req) {
  auto layers = resolveLayers(req);
  auto steps  = resolveSteps(req, layers, dimensionIndex);
  Canvas canvas = createCanvas(req.map);
  for (auto &unit : stepsToDraw(steps)) {                 // time steps or tiles; may run in threads
    for (auto &layer : layers) {
      auto header = loadHeader(layer, unit);
      auto window = chooseReadWindow(header, req.map, layer.style);   // full grid or visible extent
      auto grid   = readData(layer, header, unit, window);
      auto stats  = layer.statsFor(grid);                 // once per layer, as now
      auto style  = finaliseStyle(layer.style, stats, grid);
      renderLayer(canvas, grid, style, req.map);          // renderer table instead of a chain of ifs
    }
  }
  drawOverlays(canvas, req, layers);                      // title, dimensions, legend, scale bar
  return encodeImage(canvas, req.format);
}

// resample family
int runGetCoverage(RequestContext &req) {
  auto layers = resolveLayers(req);
  auto steps  = resolveSteps(req, layers, dimensionIndex);
  auto dest   = computeDestinationGrid(req.coverage, loadHeader(layers[0], steps.first()));  // pure, one copy
  CoverageBuffer out(dest, layers, steps);
  for (auto &unit : steps) for (auto &layer : layers) {
    auto header = loadHeader(layer, unit);
    auto grid   = readData(layer, header, unit, chooseReadWindow(header, dest));
    resampleInto(out.slot(layer, unit), grid, dest);
  }
  return encodeCoverage(out, req.format);                 // NetCDF, GeoTIFF or AAIGrid, now an encoder only
}

// sample family
int runGetFeatureInfo(RequestContext &req) {
  auto layers = resolveLayers(req);
  auto steps  = resolveSteps(req, layers, dimensionIndex);
  SampleResult result;                                    // numbers + dimensions + metadata, not text
  for (auto &layer : layers) {
    auto header = loadHeader(layer, steps.first(layer));
    auto cells  = locate(header, req.location);           // pixel, point or trajectory
    result.add(sampleLayer(layer, header, steps[layer], cells));   // series or per-step strategy
  }
  return writeFeatureInfo(result, req.infoFormat);        // html / text / json / gml
}

// catalogue family
int runGetCapabilities(RequestContext &req) {
  std::vector<LayerMetadata> metas;
  for (auto &layer : listLayers(req.cfg)) metas.push_back(layerMetadata(layer));   // DB cache or header
  return writeCapabilities(metas, req.service, req.version);
}
```

Requirements that this design keeps:

- **Memory stays bounded.** The work is still done per (layer, step), and nothing is read that isn't used right away. WCS keeps one output buffer, as it does today.
- **Threading over time steps and tiles is still possible.** Each unit gets its own `SourceHeader`/`DataGrid`, so there is no need any more for the hand-written `clone()` (`CDataSource.cpp:614-662`), which silently forgets members.
- **Nothing is written to stdout before `encode`.** This is needed so that an error image or error XML can still be sent. The fork server also depends on it, because `fclose(stdout)` closes the socket.
- **The process model doesn't change:** CGI or the fork server, environment in, one response out, caches cleared at the end.

### 3.3 `ReadWindow`: one read stage instead of four

| Who | Window today | Where |
|---|---|---|
| GetMap | always the full grid. An extent variant exists, but `usePixelExtent` is fixed to `false`. | `CImageDataWriter.cpp:872-898` |
| WCS | full, or the extent with `optimizeextent=true` | `CNetCDFDataWriter.cpp:612-648` |
| GetFeatureInfo per step | one cell `(x, y)`, set by three separate code paths in the reader | `CDataReader.cpp:472-477`, `:660`, `:980-992` |
| GetFeatureInfo JSON / EDR | hyperslabs of consecutive indices, read *without* `CDataReader` | `CURUniqueRequests.cpp` |

With an explicit `ReadWindow`, `readData` gets a single input that describes what is read, and the `DataGrid` it produces carries the geometry of the part in memory. Today `open` overwrites the full-grid geometry in `CDataSource` with that of the part read (`dWidth`, `dfBBOX`), so afterwards nobody knows the full grid any more. That is exactly what blocks extent reading, and it is the likely cause of a bug in GetFeatureInfo time series (see section 7).

Format converters fall outside this: they always read everything and rasterise into the viewport. `ConverterKind` in the `SourceHeader` makes this explicit, and `chooseReadWindow` then simply returns "full".

### 3.4 The loops made explicit

The proposal describes dependencies that run in both directions. In the design they become visible stages:

- **Style and statistics:** the style is resolved once, in `resolveLayers`. `readData` only receives what it needs from it (stride, list of post-processors). `finaliseStyle` makes a stretched *copy* of the style, instead of `stretchLegend` changing the cached style (`CDataReader.cpp:1305-1309`). GetLegendGraphic then uses the same stages, instead of its own min/max logic in `CLegendRenderers/CCreateLegend.cpp:77-98`.
- **Post-processors:** the list is fixed in the `LayerPlan`, and the two hooks become explicit calls in `readData`: "before reading", which changes the data model, and "after reading", which changes values. Their `CDataSource*` interface stays in this plan, see section 4.
- **Converters:** one registry instead of two `if` chains, with `ConverterKind` as the outcome of `loadHeader`. GetFeatureInfo then chooses its strategy from that, instead of from file attributes such as `ADAGUC_POINT` (`CImageDataWriter.cpp:488-531`).

### 3.5 Errors

What has to stay the same (pinned by tests and by Python):

- the exit code is the HTTP status: 0, 32 (404) or 33 (422) (`CServerError.cpp`, `wmswcs.py:83-97`);
- the error format depends on the request: plain text, WMS 1.1.1 or 1.3.0 XML, an image, a blank image, or JSON;
- some log lines are asserted literally in `TestWMS.py:638-641`;
- a failed time step in an animation is skipped, not fatal (`CRequest.cpp:2325-2335`).

Target:

- `ErrorPolicy` is fixed in `normaliseRequest`, before the first stage that can fail. Today `setErrorMode` is called several times during parsing, and each call **also clears the messages collected so far** (`CServerError.cpp:56`).
- Stages return a `StageError { ServiceExceptionType type; std::string stage; std::vector<std::string> messages; bool skippable; }`. At the pipeline boundary this becomes `setExceptionType` plus the existing message list, so the output stays the same.
- There is one top-level `catch` around the pipeline, for `StageError`, `std::exception` and the remaining `int`. Today there is none, so a `std::bad_alloc` kills the process.
- `throw(__LINE__)` (about 130) and `CDBError` + `return 1` are converted stage by stage, when that stage is extracted. Not in one large PR.
- **Behaviour change, to be decided separately:** a `CDBError` without an exception type now exits with 0, so the client gets HTTP **200** with an error text. The refactor should keep that at first. Changing it, for example to 500, is a separate PR that can be reverted.

### 3.6 The front end

- Replace the about 40 local `dFound_*` variables, plus `SERVICE`, `REQUEST`, `Version` and `Exceptions`, with one `ParseState` struct. This is mechanical, and it is the biggest coupling.
- Split the function into `parseCommonParams`, `parseWmsParams`, `parseWcsParams` and `parseMetadataParams`, each pure on (query, config) → fields. The rewrites (GetPointValue, Trajectory) go into `normaliseRequest`, as an explicit `RequestKind` plus `Location`, instead of changing `requestType`.
- A dispatch table `(service, request) → pipeline function` replaces the `if (dErrorOccured == 0 && ...)` chain and the second dispatch in `process_all_layers`. The inline handlers become named functions: GetScalebar (`:1727`), GetMetadata JSON (`:1789`), OpenDAP (`:853`) and DescribeCoverage (`:661`).
- `setCRequestConfigFromEnvironment` needs only the `CServerParams`, so the config step can stand on its own.
- Requirement: `fillDimValuesForDataSource` is also used by the file scanner and `CAutoConfigure`. It stays available as a free function.

### 3.7 Later: an EDR data call

EDR now goes through GetPointValue/GetTrajectory, then GetFeatureInfo, then text formatting with `%f` (6 decimals), then an XML tree, then JSON, and Python parses that again. The design keeps room for a later call, for example `REQUEST=GetSamples`:

- It uses the same `resolveLayers` / `resolveSteps` / `loadHeader` / `locate` / `sampleLayer` stages as GetFeatureInfo.
- `SampleResult` is a structure with numbers. That is why the plan separates sampling from formatting: `writeFeatureInfo` formats it for WMS, and a new `writeSamples` writes it as compact JSON (or binary) for Python.
- Python keeps making the CoverageJSON. The same applies to the cube: WCS `resampleInto` + `encodeCoverage` stays usable for it.
- What EDR needs from C++ is clear from `edr_covjson.py`: per layer and data object the name, standard_name and units (after unit conversion); the cell coordinates; and per dimension combination a value, nodata or status flag, with the order of the dimensions.

That call is not part of this plan. The plan only makes sure that the "sample" stages don't depend on the WMS text output.

## 4. Route: three options

| | A. In place, step by step | B. New pipeline next to the old one | C. Mix of A and B |
|---|---|---|---|
| How | Each PR moves code into stages. The output stays identical, and the functional tests are the safety net. | Build new pipelines behind a switch (`ADAGUC_PIPELINE=new`), compare with the old path, then remove the old path. | A for the shared stages. B only for an end stage that is rebuilt anyway. |
| For | Always shippable. No duplicate code. Small reviews. Can stop at any time. | Freedom in the new design, and no compromises with the old structure. | B only where the old code is no basis to build on. |
| Against | The design is limited by what can be moved in small steps. The intermediate states look strange: `CDataSource` becomes a "bag of stage outputs" for a while. | Two implementations for months, while colleagues keep changing the old one. Hidden side effects (sections 1 and 7) make differences between old and new hard to explain. Converters and post-processors would have to be adapted for both. | You have to choose where the boundary is. |

**Recommendation: A, with C for the GetFeatureInfo end stage only.** GetFeatureInfo already has two read paths that behave differently (JSON skips converters and before-reading post-processors) and one suspected bug. A new `sampleLayer` next to the old code, compared via the tests plus extra tests, is safer there than rebuilding the old code piece by piece.

**What deliberately stays in this plan:** the 13 converters and 18 post-processors keep their `CDataSource*` interface. During the migration `CDataSource` holds the new types as members (`header`, `grid`, `step`), with the old fields as accessors. Narrowing their interface is a separate, optional follow-up project.

## 5. Steps

Each step is one PR, unless stated otherwise, and keeps the output identical unless stated otherwise. "Done" = `compile.sh` + `runtests_psql.sh` + the Docker build on amd64 and arm64.

### Phase 0: Safety net and decisions (before the refactor)

| # | Step | Why |
|---|---|---|
| 0.1 | Tests for the paths that lack them: GetFeatureInfo text/html over **several** time steps on gridded data; GetCoverage with `TIME=a/b` and with `COVERAGE=a,b` (several levels with `elevation=1/3` are already tested); GetFeatureInfo with two layers; GetReferenceTimes (after a decision on its output format, see section 7); error bodies for WMS 1.1.1/1.3.0 XML and `EXCEPTIONS=BLANK`. | These are the paths that the later steps change. The multi-step GetFeatureInfo test checks the suspected bug from section 7. |
| 0.2 | Decide on GetHistogram: keep it (+ a test) or remove it. | No test, no doc, no Python user. Removing it saves a whole path. |
| 0.3 | Remove dead code: the `cascaded` remnants, `showWCSNotEnabledErrorMessage`, the unused `CDataReader_open_lock`, the unreachable `StopWatch_Measure` (`CRequest.cpp:1941`). Leave `usePixelExtent` in place for phase 5. | Less code to move. |
| 0.4 | Add stage names to `TraceTimingType` (PARSE, RESOLVE_LAYERS, RESOLVE_STEPS, LOAD_HEADER, READ, STATS, RENDER, ENCODE). | Profiling per stage from the start, through the existing `X-Trace-Timings`. |

### Track 1: Front end and errors (independent of track 2)

| # | Step |
|---|---|
| 1.1 | `ParseState` struct instead of the `dFound_*` locals and the `Version`/`Exceptions` members. Mechanical. |
| 1.2 | Split the parameter loop into `parseCommonParams` and per-service functions, without changing behaviour. |
| 1.3 | `normaliseRequest`: the GetPointValue/Trajectory rewrite as `RequestKind` + `Location`. `requestType` stays for the existing code, and is derived from the kind. |
| 1.4 | Fix `ErrorPolicy` once, instead of calling `setErrorMode` several times. **Check:** messages that are cleared today now stay. If a test shows a difference, write down the old behaviour on purpose. |
| 1.5 | Dispatch table. The inline handlers become named functions. The second dispatch in `process_all_layers` becomes calls from the table. |
| 1.6 | `StageError` + a top-level `catch`. From this point, extracted stages use it. |
| 1.7 | (Behaviour change, optional) A `CDBError` without a type no longer gives exit 0 → decide 500 or 422. |

### Track 2: Data path

**Phase 2: split the reader into stages (pure code move)**

| # | Step |
|---|---|
| 2.1 | Split `CDataReader::open` internally into named stages. The research suggests 16 boundaries, S0 to S15, for example `acquireCDFObject`, `parseGeometry`, `prepareReadWindow`, `runPostProcBeforeReading`, `readData`, `applyScaleOffset`, `computeStatistics`, `stretchLegend`. `open(mode)` calls them in the same order. Only a few local variables cross a boundary (`start/count/stride`, `dataIsRead` per object, `singleCellMode`). Those become a small struct. |
| 2.2 | Split `parseDimensions` the same way: converters / XY axes / read X/Y and CRS / cell size and bbox / lon wrap. Replace the `requestType` check at `:664` with an explicit input ("skip lon wrap"). |
| 2.3 | One converter registry instead of the two `if` chains. `ConverterKind` as a result, `formatConverterActive` derived from it. |
| 2.4 | First C++ unit tests: header and data of `data/datasets/testdata.nc` without the DB (config via `parseConfig`, `ds.addStep(file)`, `open`). This proves the stages can be tested on their own. |

**Phase 3: style and post-processors explicit**

| # | Step |
|---|---|
| 3.1 | Resolve the style once, at layer creation (`LayerPlan`), instead of lazily in `getStyle()`. **Check:** the style currently also runs for WCS and GetFeatureInfo. The plan keeps that at first, see decision D3. |
| 3.2 | `finaliseStyle`: the style changes from `warpImage` (feature intervals, default vectors, the `PointsFromGrid` call at `:1033`) and `stretchLegend` move to one stage that returns a copy. |
| 3.3 | GetLegendGraphic uses the same statistics/stretch stages instead of its own logic in `CCreateLegend`. |

**Phase 4: types out of `CDataSource`**, in this order, based on how easy they are to take out:

| # | Step |
|---|---|
| 4.1 | `Step` explicit: the stages get the step as an argument, instead of reading `currentAnimationStep` via `getFileName()`. The cursor stays in place for converters and post-processors. |
| 4.2 | `GridGeometry` (native, immutable, from `loadHeader`), separate from the geometry of what is in memory (`DataGrid`). This is the core step for extent reading and for the time-series bug. |
| 4.3 | `Statistics` + `stretchMinMax` as output of `computeStatistics`. |
| 4.4 | `CDataSource::clone()` reduced to copying value types, or removed once threads get their own unit. |

**Phase 5: pipelines per family**, one PR each, in this order:

| # | Step | Notes |
|---|---|---|
| 5.1 | GetMap: `addData` → `renderLayers`, `warpImage` → `loadAndRenderLayer`, which call the stages directly. Renderer choice as a table. Overlays out of `handleGetMapRequest` into `drawOverlays`. | the biggest win in readability |
| 5.2 | GetMap reads only the visible extent (`chooseReadWindow`). Benchmark with `run-benchmark-all-versions.sh`. | depends on 4.2 and decision D1 |
| 5.3 | GetCoverage: `computeDestinationGrid` as one pure function for both writers; the writers become encoders. Remove the duplicate dimension resolution. | **check:** the GeoTIFF/AAIGrid writer rounds differently (no +0.5) |
| 5.4 | GetFeatureInfo: a new `sampleLayer` + `SampleResult` next to the old code (route C), behind a switch, compared via the tests; then remove the old code. Separate `writeFeatureInfo` from `end()`. | decision D2 |
| 5.5 | Catalogue: GetCapabilities / DescribeCoverage / GetMetadata / GetReferenceTimes as their own short pipelines. DescribeCoverage no longer builds data sources it doesn't use. GetReferenceTimes skips the full dimension resolution. | |
| 5.6 | `DimensionIndex` interface in front of `CDBAdapterPostgreSQL` for `resolveSteps`, so that it can be tested without Postgres. | can also come earlier |

### Order and parallelism

```
Phase 0 ──┬── Track 1: 1.1 → 1.2 → 1.3 → 1.4 → 1.5 → 1.6 (→ 1.7)
          └── Track 2: 2.1 → 2.2 → 2.3 → 2.4 → 3.x → 4.x → 5.1 → 5.2
                                                      └── 5.3, 5.4, 5.5 (in any order, after 4.2)
```

Track 1 mainly touches `process_querystring` and `CServerError`. Track 2 touches `CDataReader`, `CDataSource` and the writers. They only meet in `process_all_layers` (1.5 versus 5.x), so do 1.5 first.

## 6. Decisions for the team

| | Question | Options | Advice |
|---|---|---|---|
| D1 | **Statistics with extent reading.** Automatic legend stretching uses min/max of the data read. If we read only the visible extent, the colours change with the zoom level, and adjacent tiles in a map client get different colours. | (a) read the full grid when the style needs stretching, otherwise only the extent; (b) statistics on the extent; (c) statistics from the metadata cache or a coarse read | (a): no visible change. (c) later as an optimisation. |
| D2 | **GetFeatureInfo: one read path or two?** The JSON path (and so EDR) reads all time steps of a cell in one hyperslab, straight through the CDF data model. It therefore skips what `CDataReader::open` does: format converters, before-reading post-processors, lon wrap and stride. Unit conversion is a separate implementation that ignores `from_units` and style-level conversions. Which layers go to the text path is decided by a hand-kept list of file attributes, which misses some converters (curvilinear, TROPOMI, ASCAT). Sharing the read needs the stage split (2.1) and a `ReadWindow` with hyperslab runs (3.3) first, so a merge can only be part of 5.4. | merge them (behaviour change for some layers); or keep both as named strategies, chosen explicitly from `ConverterKind` and the post-processors | First keep both, named and explicit (no change). Merge later, in a separate PR with output differences reviewed. Bugs in one path (section 7) are fixed on their own before that. |
| D3 | **Style in requests that don't draw** (WCS, GetFeatureInfo). Post-processors from the style now also run there. | keep (it works and tests rely on it); or only layer post-processors for non-drawing requests | Keep, but make it visible in `LayerPlan`. Ask whether this is intended. |
| D4 | **HTTP 200 with an error body** for a `CDBError` without a type | keep, or 500 | Separate PR (1.7) after the refactor. |
| D5 | **GetHistogram** | keep + test, or remove | Remove, unless someone uses it. |
| D6 | **Who and when.** Two tracks allow two people. How long do we keep the PRs open, so that other branches don't conflict too much? | | Merge quickly and announce in advance. |

## 7. Found along the way

The research turned up a number of possible bugs and some dead code. They are not part of the refactor, but some of them get in its way. Items marked "suspected" have not been checked by running code.

| What | Where | Status |
|---|---|---|
| GetFeatureInfo text/html and text/plain over several time steps: after the first single-cell read, `dWidth/dHeight=1` and `dfBBOX` holds the cell. The header is not opened again, so from step 2 on the clicked point falls outside that "grid" and every value is `nodata`. | `CImageDataWriter.cpp:590`, `CDataReader.cpp:981` | **confirmed**, fixed in a separate PR |
| Packed data (`scale_factor`/`add_offset`) without `_FillValue`: the first read scales the default nodata value 0 and writes `_FillValue = add_offset` on the *cached* variable. From the second read of the same file in one request (WCS over time or levels, GetMap animation, multi-step GetFeatureInfo) the value `add_offset` is nodata. An example of the request-modified cache state from section 1. | `CDataReader.cpp:1232` | **confirmed**, fixed in a separate PR |
| GetFeatureInfo JSON (and so EDR position) on packed data with a `_FillValue`: the hyperslab read unpacks the data, but the nodata check and the unit conversion used the packed nodata value, so a fill cell gave `-999.9` (or a converted value) instead of `nodata`. The text path is correct. | `CUniqueRequests/CURUniqueRequests.cpp`, about `:400` | **confirmed**, fixed in a separate PR |
| `CDF::Variable::readData(..., true)` unpacks the data and also rescaled the `_FillValue` attribute of the cached variable, on **every** call. After a second read of the same variable in one request the attribute no longer matched the data. | `../CCDFDataModel/CCDFVariable.cpp:60` | **confirmed**, fixed in a separate PR (one helper, `unpackFillValueOnce`, used by `readData` and `CDataReader`) |
| GetFeatureInfo JSON (and so EDR position) on a layer with more than one variable: the results of the first variable were not cleared before the second one. Because `std::sort` is not stable, with enough time steps the second variable showed values of the first (30 of 60 in a test). Layers with `convert_uv_components` take the text path and were not affected. | `CUniqueRequests/CURUniqueRequests.cpp`, the loop over data objects | **confirmed**, fixed in a separate PR |
| GetFeatureInfo JSON: NaN only counts as nodata when the variable has a nodata value (`_FillValue`). Without one, a NaN is a valid value and is written as `nan`. Arguably NaN should always be nodata, but that changes the output. | `CUniqueRequests/CURUniqueRequests.cpp`, the nodata check | **confirmed** from the code, not changed |
| GetReferenceTimes joins all times into **one** JSON string (`["t1,t2"]` instead of `["t1","t2"]`), and an empty list gives `[""]`. | `CRequest.cpp:81-85` | **confirmed**, not fixed: clients may parse the current form |
| `statistics` is computed only once per data source: animation frames use the min/max of the first frame. `clone()` doesn't copy it, so threaded animations can get different legends per thread. | `CDataReader.cpp:1296`, `CDataSource.cpp:614` | suspected |
| `setDimValuesForDataSource` runs twice for GetCoverage. | `CRequest.cpp:2470` | confirmed |
| `pthread_join(..., (void **)&status)` with `int status`: writes 8 bytes into 4. | `CRequest.cpp:2296` | confirmed, UB |
| A missing X/Y in GetFeatureInfo sets `dErrorOccured`, but the handler runs anyway. | `CRequest.cpp:1692-1709` | from reading the code |
| DescribeCoverage always exits with 0, even on errors. | `CRequest.cpp:1853-1860` | from reading the code |
| The GeoTIFF/AAIGrid writer silently uses only the first layer when `COVERAGE=a,b`. | `CGridFileDataWriter.cpp:179` | from reading the code |
| `bool dimsAreAutoConfigured = -1` (truthy). The constructor fixes it. | `CDataSource.h:91` | harmless |
| The `CDFObjectStore` evicts the oldest file at 500 files, not the least recently used, and does so while `CDataSource` still holds raw pointers into it. | `CDFObjectStore.cpp:210` | risk with long animations |

## 8. Risks

- **Hidden dependencies we don't know about yet.** Converters and post-processors rely on `CDataSource` fields in ways that only show up when you move them. Mitigation: phase 0 tests, keeping the `CDataSource*` interface, and small PRs.
- **Intermediate states.** For a while `CDataSource` holds both new types and old fields. This must not stay that way. Every phase removes old fields once nothing uses them any more.
- **Image tests that differ between amd64 and arm64.** Always run both in CI. Only accept differences in a PR that is meant to change behaviour.
- **Performance:** moving code changes nothing. Extent reading (5.2) is the only intended performance change, and it gets a benchmark.
- **Calendar time:** about 30 PRs. With one person part-time that is several months. With two tracks in parallel, less. Every phase is useful on its own.

## See also

- [Proposal: an explicit request pipeline](./request-pipeline-proposal.md)
- [WMS GetMap and GetCapabilities: code flow and data flow](./getmap-request-flow.md)
