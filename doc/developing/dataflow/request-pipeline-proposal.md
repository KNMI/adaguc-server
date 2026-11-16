# Proposal: an explicit request pipeline

[Back to readme](../../../Readme.md)

This document is a proposal for discussion. It describes a problem in how the C++ request handling is structured, and suggests a step-by-step refactoring towards an explicit pipeline. The [plan](./request-pipeline-plan.md) works this out for all frequently used request types.

Paths are relative to `adagucserverEC/`, and line numbers refer to `master` at `36989f97` (version 10.0.0).

## Summary

The request handling is a deep call tree, in which each function does part of the work and then calls down a level. The real work happens a few levels down, under names that hide what it does. Data passes between steps by changing one large object (`CDataSource`) in place. This makes the code hard to read, to test and to change.

The proposal is to restructure it, gradually, into a pipeline. A top-level function runs named stages in order, and each stage has explicit input and output. The goal is not just to flatten the call tree, but to make the data flow visible.

## The current situation

### Names describe the first step, not the work

Following a WMS GetMap request:     

| Function | What the name suggests | What it does |
|---|---|---|
| `CRequest::handleGetMapRequest` (`CRequest.cpp:2196`) → `CImageDataWriter::addData` (`CImageDataWriter.cpp:1109`) | adds data to a list | **renders** all layers for one time step: creates the palette, calls `warpImage`, and draws title, subtitle and dimension text on the image (about 300 lines) |
| `CImageDataWriter::warpImage` (`CImageDataWriter.cpp:865`) | reprojects an image | **loads, reprojects and renders** one layer: reads the data, resolves the style, sets up the reprojection, and runs all applicable renderers |
| `CDataReader::open(dataSource, mode)` (`CDataReader.cpp:849`) | opens a file | depending on `mode`: opens the file, parses dimensions and geometry, runs format converters, runs post-processors **before** reading, reads the data, applies scale and offset, runs post-processors **after** reading, and calculates statistics |

The `mode` argument of `CDataReader::open` (`Definitions.h:90-94`) selects how far this staged process runs, using early returns: `OPEN_DIMENSIONS`, `OPEN_HEADER`, `GET_METADATA`, `OPEN_ALL` or `OPEN_EXTENT`. So one function with one generic name is in fact five different operations.

### Data flows through side effects

All steps share one mutable object, `CDataSource` (`CDataSource.h`, over 100 members). Each step reads some members and changes others. Nothing at the call site shows which.

A concrete example, a layer with `<DataPostProc algorithm="convert_uv_components"/>` (wind barbs):

- Before the call `reader.open(dataSource, CNETCDFREADER_MODE_OPEN_ALL)` (`CImageDataWriter.cpp:897`), the data source has **2** data objects: the x and y wind components.
- After the call it has **6**: speed, direction, eastward and northward wind, plus the original two. The wind has been rotated to true north, and statistics have been calculated. The post-processor inserted the new objects at the front of the list (`CDataPostProcessor_UVComponents.cpp:111-114`), so every index into `dataObjects` has shifted.
- The post-processor was configured in the **style**, not in the layer. `getProcessorList` (`CDataPostProcessors/CDataPostProcessor.cpp:67`) collects both, which you can only find out by reading the code.

### Dependencies run in both directions

The steps are not a simple sequence:

- The style is needed **before** reading: `CDataReader::open` asks for it (`CDataReader.cpp:879`) to decide whether statistics are needed for an automatic legend scale.
- The style is changed **after** reading: the statistics stretch the legend (`CDataReader.cpp:1296-1309`).
- Post-processors run both before reading, where they change which data objects exist, and after it, where they change the data (`CDataReader.cpp:1039` and `1291`).
- Format converters (`CConvert*`, tried in order from `CDataReader.cpp:372`) can replace the reading step entirely.

These loops are real requirements, but now they are hidden inside the call tree.

### What this costs us

- **Reading:** to understand one request you need to read the whole depth of the call tree, and keep track of what each level changes in `CDataSource`.
- **Reuse:** GetMap, GetFeatureInfo, WCS and EDR all go through the same overloaded functions with flags and modes, instead of combining shared steps. For example, GetFeatureInfo has its own sequence of `reader.open` calls (`CImageDataWriter.cpp:468` onwards) with a special single-cell mode.
- **Testing:** the steps can't be tested on their own. A test needs a complete request, a database and a data file.
- **Changing:** a change in one step can break a step that relies on a side effect several calls away. For example, fixing the bounding box in single-cell mode changed wind directions in GetFeatureInfo, through the wind post-processor.
- **Profiling and errors:** there are no clear boundaries to time a stage at, or to report an error from.

## The proposal: an explicit pipeline

### Depth-first versus breadth-first

Today the code is "depth-first": `addData` does a bit, calls `warpImage`, which does a bit and calls `open`, which does most of the work. In a pipeline, the top level lists the stages one after another ("breadth-first"), and each stage is a function with a clear input and output:

```
parse request
  → resolve layers
  → resolve dimensions to files and indices (database)
  → for each (layer, time step):
        load header (geometry, CRS, variables)
        → run post-processors that change the data model
        → read data
        → scale and offset, post-processors on the data
        → statistics
        → resolve style (including legend stretch)
        → reproject and render
  → overlays (title, legend, scale bar)
  → encode (PNG, JSON, NetCDF, ...)
```

### What this gives us

- **The flow is visible in one place.** The pipeline function reads like the diagrams in the flow document.
- **Request types become combinations of stages.** GetMap, GetFeatureInfo, WCS and EDR share the loading stages and differ in the last ones: render, sample a point, write a coverage. They no longer need flags and modes inside shared functions.
- **Stages can be tested on their own.** For example, "read data" can be tested against a small file, without a database or a renderer.
- **Clear boundaries for profiling and errors.** Each stage can be timed (`STOPWATCH`/trace timings) and can fail with a clear message.
- **The loops become explicit.** "Resolve the style, read, then stretch the legend" becomes visible in the pipeline instead of hidden in `open`.

### Design constraints

1. **Make the data explicit, not just the calls.** If we only move the calls up a level and keep passing one mutable `CDataSource`, we get a flat call list with the same hidden coupling. The real gain comes from giving stages their own input and output types, for example: layer configuration → time steps and files → grid geometry → data grids → resolved style. That means gradually splitting `CDataSource`.
2. **Run the pipeline per unit of work, not over all data at once.** The current structure processes one layer and one time step at a time, which keeps memory use bounded: animations and GetFeatureInfo time series can involve many files. The pipeline should run its stages per (layer, time step), not "read everything, then render everything". This also keeps the existing threading over time steps (`CRequest.cpp`, around line 2225) possible.
3. **Leave room for the variants:** format converters that replace reading, single-cell reads for GetFeatureInfo, tiled layers, `liveupdate` and virtual data sets without files, and the WCS writers.

## Plan: small steps, same behaviour

A big rewrite would be risky and would block other work for a long time. Instead, each step below keeps behaviour identical, can be reviewed as a separate PR, and leaves the code better than before even if we stop there. The functional tests (443 tests, including image comparisons) are the safety net for every step.

1. **Split `CDataReader::open` internally into named stages**: for example `openHeader`, `applyFormatConverters`, `runPostProcessorsBeforeReading`, `readData`, `applyScaleAndOffset`, `runPostProcessorsAfterReading` and `computeStatistics`. `open(mode)` keeps its signature and calls these stages in the same order. This is a pure code move.
2. **Lift the stages to the caller, for GetMap only**: `warpImage` and `addData` call the stages directly instead of `open(OPEN_ALL)`, and are renamed for what they do (for example `loadAndRenderLayer` and `renderLayers`).
3. **Do the same for GetFeatureInfo, then WCS and EDR**, one request type per PR.
4. **Introduce stage types one at a time**, moving members out of `CDataSource`, starting with the ones with the clearest owner (grid geometry, statistics).
5. **Write the pipeline function** for each request type, once the stages exist.

## Risks and open questions

- **Size:** this is several months of calendar time, working on it part time, spread over many PRs.
- **Conflicts:** it touches code everyone works in, and will conflict with open branches. Agreeing on an order, and merging steps quickly, will help.
- **Test coverage:** some paths have little coverage, e.g. GetFeatureInfo over several time steps in non-JSON formats, and some format converters. These should get tests before they are refactored.
- **Performance:** the work done stays the same, so performance should not change. Clearer stages may later make it easier to parallelise reading.
- **Open question:** do we want to start with step 1 only, and decide on the rest after seeing the result?

## See also

- [Plan: from a deep call tree to request pipelines](./request-pipeline-plan.md): the flows of all frequently used request types, the choice between one shared pipeline and several, the final design, and the steps.
