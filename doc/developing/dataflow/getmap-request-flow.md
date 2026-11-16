# WMS GetMap: code flow and data flow (C++)

[Back to readme](../../../Readme.md)

This document follows a single `SERVICE=WMS&REQUEST=GetMap` request through `bin/adagucserver`. It covers a normal `database` layer that serves a regular gridded NetCDF file. Paths are relative to `adagucserverEC/` unless stated otherwise. Line numbers refer to `master` at `36989f97` (version 10.0.0).

Each request runs in its own process, either a fresh process or a child forked by the fork server. The Python wrapper sets `QUERY_STRING` and `ADAGUC_CONFIG` in the environment, and the binary writes HTTP headers plus the image body to stdout. Global singletons and caches (`CDFObjectStore`, `CDBFactory`, proj caches) are cleared when the process finishes (`adagucserver.cpp`, `run_adaguc_once`).

## Key objects

| Object | What it holds | Lifetime |
|---|---|---|
| `CServerParams` (`CServerParams.h`) | The parsed request (`geoParams` = BBOX/WIDTH/HEIGHT/CRS, `requestDims`, `Styles`, `imageFormat`, ...) plus `cfg`, the merged XML config | Whole request; owned by `CRequest` |
| `CServerConfig::XMLE_*` (`CServerConfig_CPPXSD.h`) | The XML config as a tree of structs (`cfg->Layer[i]`, `cfg->Style`, `cfg->Legend`, ...) | Whole request |
| `CDataSource` (`CDataSource.h`) | One layer to draw: `cfgLayer`, `requiredDims`, `timeSteps` (one filename per step + dim indices), `dataObjects` (one per `<Variable>`), and the native grid geometry (`dWidth/dHeight/dfBBOX/nativeProj4`) | Created per requested layer in `CRequest::addDataSources` |
| `DataObject` (`CDataSource.h`) | Links one `<Variable>` to its `CDF::Variable` (data buffer), nodata value, points, features | Inside `CDataSource` |
| `CDFObject` / `CDF::Variable` (`../CCDFDataModel/`) | In-memory NetCDF-like model. `variable->data` is the raw `void*` grid | Cached per file in `CDFObjectStore` |
| `CStyleConfiguration` | The resolved style: `renderMethod` bitmask, legend index, shade/contour intervals, min/max/scale | Per `CDataSource` (`getStyle()`) |
| `CImageDataWriter` | Owns the output canvas `CDrawImage drawImage` and runs the renderers | Stack object in `handleGetMapRequest` |
| `CDrawImage` | RGBA (or palette) pixel buffer plus Cairo for text and lines. It encodes to PNG/WebP | Inside `CImageDataWriter` |

## 1. Code flow

Rendered image, best opened on its own to zoom and pan: [getmap-code-flow.svg](./getmap-code-flow.svg) ([PNG](./getmap-code-flow.png))

```mermaid
sequenceDiagram
    autonumber
    participant Main as adagucserver.cpp<br/>main / run_adaguc_once
    participant Cfg as utils/ConfigurationUtils.cpp
    participant Req as CRequest
    participant DS as CDataSource
    participant DB as CDBAdapterPostgreSQL<br/>(via CDBFactory)
    participant IDW as CImageDataWriter
    participant Rdr as CDataReader
    participant Store as CDFObjectStore<br/>+ CDFReader (CCDFDataModel)
    participant Warp as CImageWarper (PROJ)<br/>+ CImgWarpGeneric / GenericDataWarper
    participant Img as CDrawImage

    Main->>Main: processCMDArgs() returns -1 (no CLI mode)<br/>→ processQueryStringRequest()
    Main->>Cfg: setCRequestConfigFromEnvironment()
    Cfg->>Cfg: parseAndCheckConfigFile(ADAGUC_CONFIG)<br/>parse server.xml, extra xmls, dataset.xml, Includes<br/>CAutoResource::configureDataset, DATASET= param
    Main->>Req: runRequest() → process_querystring()
    Req->>Req: parse KVP into CServerParams<br/>(BBOX, WIDTH, HEIGHT, CRS, LAYERS, STYLES, FORMAT, TIME/ELEVATION/DIM_*)<br/>WMS 1.3.0: swap BBOX axes if needed, map FORMAT → imageFormat
    Req->>Req: process_wms_getmap_request() → process_all_layers()

    loop for each name in LAYERS
        Req->>Req: findLayerConfigForRequestedLayer()
        Req->>DS: addDataSources(): new CDataSource, setCFGLayer()<br/>(one DataObject per Variable, dLayerType)<br/>+ AdditionalLayers
    end

    Req->>Req: determineTypesForDataSources()
    opt layer has no Dimension
        Req->>DS: CAutoConfigure::autoConfigureDimensions()
    end
    Req->>Req: setDimValuesForDataSource()
    Req->>Req: fillDimValuesForDataSource()<br/>requested dims, fixvalue, then defaults for omitted dims
    Req->>DB: getDefaultDimensionValue / getMax ("current")
    Req->>DB: queryDimValuesForDataSource() → getFilesAndIndicesForDimensions()<br/>(or handleTileRequest() when TileSettings)
    DB-->>DS: rows (path, dimval, dimindex, ...) → addStep(path) + CDFDims per step
    Req->>Req: autoDetectBBOX() (only if BBOX missing)

    Req->>IDW: handleGetMapRequest() → init()
    IDW->>Img: createImage(geoParams), createPalette(Legend)

    loop for each timestep k (normally 1, more for an animation)
        Req->>DS: setTimeStep(k)
        Req->>IDW: addData(dataSources)
        loop for each CDataSource (bottom → top)
            IDW->>IDW: warpImage(dataSource, &drawImage)
            IDW->>Rdr: open(ds, CNETCDFREADER_MODE_OPEN_ALL)
            Rdr->>Store: getCDFObject(filename) → pick reader (NetCDF/HDF5/GeoJSON/PNG/CSV), open, cache
            Rdr->>DS: attachCDFObject(): variable name → CDF::Variable
            Rdr->>Rdr: parseDimensions(): try CConvert* format converters,<br/>find x/y dims, bbox, cellsize, CRS
            Rdr->>Rdr: run DataPostProcessors (before reading)
            Rdr->>Store: variable->readData(start, count, stride)<br/>start[non-xy dims] = DB dim index
            Rdr->>Rdr: scale_factor/add_offset, swapXY, lon wrap<br/>run DataPostProcessors (after reading)<br/>Statistics min/max → stretchLegend()
            IDW->>DS: getStyle() → renderMethod
            IDW->>Warp: initreproj(source CRS → request CRS)
            IDW->>Warp: render(): RM_NEAREST → CImgWarpNearestNeighbour<br/>RM_GENERIC/BILINEAR/SHADED/CONTOUR/VECTOR/BARB → CImgWarpGeneric<br/>RM_RGBA, HILLSHADED, STIPPLING, POINT, POLYLINE → their renderers
            Warp->>Img: setPixel(x, y, color), drawContour(), lines/text via Cairo
            IDW->>Rdr: close()
        end
    end

    Req->>Img: optional title/subtitle/dims/legend/scalebar overlays
    Req->>IDW: end()
    IDW->>Img: printImagePng8/24/32 or printImageWebP32
    Img-->>Main: "Content-Type:image/png" headers + bytes on stdout
    Main->>Main: clear caches, close log, exit status
```

Notes on the code flow:

- **Dispatch** happens in two places. `process_querystring` (`CRequest.cpp`, about line 802 to 1944) is one large function that parses parameters and then branches on `srvParam->requestType`. `process_all_layers` (`CRequest.cpp:596`) is the shared path for all layer-based requests: GetMap, GetFeatureInfo, GetLegendGraphic, GetCoverage, etc. GetMap goes to `handleGetMapRequest` (`CRequest.cpp:2196`).
- **Errors** are mostly `CDBError(...)` followed by `return 1` or `throw(__LINE__)` (an `int`). `CServerError` / `readyHandleError()` in `main` turns the collected errors into a ServiceException in the right format (XML, plain text, or an image for GetMap).
- **Multiple layers** (`LAYERS=a,b`) give multiple `CDataSource`s. All of them are drawn onto the same `drawImage`, in order. The palette is re-created per layer in `addData` (`CImageDataWriter.cpp:1109`).
- **Threading** is used only when the layer has `<TileSettings threads="N">`. In that case timesteps are spread over pthreads, each with cloned datasources (`CRequest.cpp`, inside `handleGetMapRequest`).
- **`liveupdate` layers** skip the DB and file path entirely (`layerTypeLiveUpdateRender`). **`grid` (graticule)** layers add an empty step and draw lines only.

## 2. Data flow

Rendered image: [getmap-data-flow.svg](./getmap-data-flow.svg) ([PNG](./getmap-data-flow.png))

The code flow is mostly function calls, but the data changes form several times: query string → structs → SQL → file index → raw grid → screen grid → pixels → PNG bytes. The diagram below follows the data instead of the calls.

```mermaid
flowchart TD
    subgraph INPUT["Inputs"]
        QS["QUERY_STRING<br/>LAYERS, STYLES, CRS, BBOX,<br/>WIDTH, HEIGHT, FORMAT, TIME, DIM_*"]
        XML["ADAGUC_CONFIG XML files<br/>server.xml + dataset.xml + includes"]
        PG[("PostgreSQL<br/>one table per (FilePath, filter, dimension):<br/>path | dimvalue | dimindex | filedate")]
        FILE[("Data file<br/>NetCDF / HDF5 / GeoJSON / PNG / CSV")]
    end

    XML -->|"CXMLParser to CServerConfig::XMLE_* tree"| CFG["srvParam->cfg<br/>Layers, Styles, Legends, Projections"]
    QS -->|"CT::split '&' and '=', upper-case keys"| SP["CServerParams<br/>geoParams {bbox, width, height, crs}<br/>requestDims [{name, value}]<br/>Styles, imageFormat"]

    CFG --> DSRC
    SP --> DSRC["CDataSource per layer<br/>cfgLayer, dataObjects[variableName]"]

    DSRC -->|"fillDimValuesForDataSource:<br/>requested, else fixvalue, else DB default (min/max)"| RD["requiredDims<br/>[{name, netCDFDimName, value}]<br/>e.g. time = 2024-01-01T00:00:00Z"]
    RD -->|"getFilesAndIndicesForDimensions<br/>SELECT path, time, dimtime ... WHERE time = 'X'"| PG
    PG -->|"records"| TS["timeSteps[k]<br/>{fileName, CDFDims[{name, value, index}]}"]

    TS -->|"filename"| FILE
    FILE -->|"CDFReader opens the header<br/>(cached in CDFObjectStore)"| CDFO["CDFObject<br/>dims, vars, attributes"]
    CDFO -->|"attachCDFObject + parseDimensions<br/>(CConvert* may synthesise a grid)"| GEO["Source geometry on CDataSource<br/>dWidth, dHeight, dfBBOX, cellsize,<br/>nativeProj4, dimX/YIndex"]
    TS -->|"dim index sets start[] of non-xy dims"| RDDATA
    GEO --> RDDATA["CDF::Variable::readData(start, count, stride)<br/>variable->data = void* source grid<br/>(native type: byte/short/float/...)"]
    RDDATA -->|"scale_factor / add_offset (to float/double)<br/>DataPostProcessors (e.g. unit conversion)<br/>_FillValue to dfNodataValue"| SRC["Prepared source grid"]
    SRC -->|"Statistics::calculate (if no fixed scale)"| STY
    CFG -->|"getStyle(STYLES)"| STY["CStyleConfiguration<br/>renderMethod, legend, min/max,<br/>shade / contour intervals"]

    SRC --> WARP
    SP -->|"destination geoParams"| WARP
    GEO -->|"source CRS"| WARP["CImageWarper (PROJ)<br/>source CRS to request CRS"]
    WARP -->|"GenericDataWarper:<br/>linearTransformGrid or warpTransformGrid<br/>(source cells as triangles in screen space)"| DGRID["float destinationGrid[W x H]<br/>value per output pixel<br/>(nearest or bilinear sampling)"]
    STY --> COLOR
    DGRID -->|"memoizedDeterminePixelColorFromValue<br/>value to legend colour / shade interval"| COLOR["CDrawImage pixel buffer<br/>RGBA or palette"]
    DGRID -->|"drawContour (if contour lines)"| COLOR
    COLOR -->|"overlays: title, legend, scalebar"| COLOR
    COLOR -->|"printImagePng8/24/32 / printImageWebP32"| OUT["stdout:<br/>Content-Type header + PNG/WebP bytes"]
```

Notes on the data flow:

- **The database is an index, not a data store.** It holds only filenames and dimension values plus their index along the file's dimension. It is filled offline by `--updatedb` (`CDBFileScanner`), or on the fly for autowms/`source=` requests (`autoUpdateAndScanDimensionTables`). The pixel data always comes from the file.
- **The dimension index** from the DB (`dimtime` etc.) becomes `start[j]` for the non-spatial dimensions in `CDataReader::open` (`CDataReader.cpp`, around line 976). The whole x/y slab is read (`count = dOrigWidth x dHeight`) with an optional `stride2DMap` for downsampling. The BBOX is **not** used to subset the read; `usePixelExtent` is hard-coded to `false` in `warpImage`.
- **Format converters** (`CConvert*`, tried in order in `CDataReader::parseDimensions`) are where non-regular inputs become something the renderers can handle. Examples are points, curvilinear grids, GeoJSON, UGRID, TROPOMI, and volume scans. When a converter is active it fills the data itself, and `readData` is skipped (`formatConverterActive`).
- **Two grids in the generic renderer:** the *source* grid (native type, native CRS) and a *destination* `float` grid at the output size. `GenericDataWarper` walks the source cells, projects their corners to screen space, and calls a draw function for every covered pixel. That function writes the sampled value into the destination grid and, if `drawgrid` is on, the colour into `CDrawImage`. Contours are computed afterwards from the destination float grid.
- **Colour** comes from the style: either a continuous legend scaled with `min/max` (`legendScale/legendOffset`, auto-stretched from the data statistics if not configured), or discrete `ShadeInterval`s.

## Which renderer draws the pixels

`CImageDataWriter::warpImage` (`CImageDataWriter.cpp:981` onwards) picks renderers with a series of **independent** `if` blocks on the style's `renderMethod` bitmask, not an `else if` chain. The keywords in `<RenderMethod>` are turned into bits by `getRenderMethodFromString` (`CStyleConfiguration.cpp:10`). That function uses substring matching, so `shadedcontour` sets both `RM_SHADED` and `RM_CONTOUR`.

| `<RenderMethod>` keyword | Bit | Renderer |
|---|---|---|
| `nearest` (also `linearinterpolation`) | `RM_NEAREST` | `CImgWarpNearestNeighbour` |
| `generic`, `bilinear`, `shaded`, `contour`, `vector`, `barb` | `RM_GENERIC`, `RM_BILINEAR`, `RM_SHADED`, `RM_CONTOUR`, `RM_VECTOR`, `RM_BARB` | `CImgWarpGeneric` |
| `rgba` | `RM_RGBA` | `CImgWarpNearestRGBA` |
| `point`, `polyline`, `stippling`, `hillshaded`, ... | their own bits | their own renderers |

Both the nearest and the generic renderer use `GenericDataWarper::render`, which walks the source cells and calls a draw function per covered destination pixel. The difference is in the draw function:

- **`CImgWarpNearestNeighbour`** (`CImgWarpNearestNeighbour.cpp:203`) builds a `GDWArgs` from the `CDataSource` geometry and calls `render<T>` with `CImgWarpNearestNeighbour::drawFunction` (line 463). That function maps the value to a colour and writes it **directly into `CDrawImage`**. There is no intermediate float destination grid.
- **`CImgWarpGeneric`** (`CImgWarpGeneric.cpp:174`) writes the sampled value into a `float` destination grid first, optionally draws the colour, and only then computes contour lines from that grid. This is the pipeline shown in the data-flow diagram above.

`GenericDataWarper::render` (`CGenericDataWarper.h:359`) chooses `linearTransformGrid` when `warper->isProjectionRequired()` is false (source CRS equals the request CRS) and `warpTransformGrid` otherwise. Both renderers go through this choice.

Because the `if` blocks are independent, a style with `nearest,shaded` runs `CImgWarpNearestNeighbour` and then `CImgWarpGeneric` on the same image.

### Worked examples

| Example | Config | Result |
|---|---|---|
| KNMI radar, `STYLES=radar/nearest`, `CRS=EPSG:3857` (CLion run configuration "adagucserver GetMap radar") | Layer `precipitation_real_time` with `<DataReader>HDF5</DataReader>`, style `radar` has `<RenderMethod>nearest</RenderMethod>` | `CImgWarpNearestNeighbour` → `GenericDataWarper`. `EPSG:3857` differs from the HDF5 source CRS (not checked), so `warpTransformGrid`; the "native grid" run configurations should take `linearTransformGrid`. |
| Same layer with the second style `rainrate-blue-to-purple` | `<RenderMethod>nearest,shaded</RenderMethod>` | Both renderers run, see above. |
| HARMONIE netCDF, layer `air_temperature_hagl`, style `temperature` (`knmi_ha43_nl_2p5km.xml` with `includes/StandardStyles_WIK.xml`, from the kdp-adaguc repository) | netCDF files (`...geoweb_nc_...nc`, default reader), `<RenderMethod>shaded,shadedcontour</RenderMethod>` | `RM_SHADED \| RM_CONTOUR` → `CImgWarpGeneric` with `ShadeInterval` colours plus `ContourLine` lines. |

## See also

- [Proposal: an explicit request pipeline](./request-pipeline-proposal.md): why the deep call tree (`addData` → `warpImage` → `CDataReader::open`) is hard to follow, and a step-by-step plan to restructure it.

## Where to start reading

1. `CRequest::process_all_layers` (`CRequest.cpp:596`): the main dispatch.
2. `CRequest::setDimValuesForDataSource` (`CRequest.cpp:181`) and `CDBAdapterPostgreSQL::getFilesAndIndicesForDimensions` (`CDBAdapterPostgreSQL.cpp:404`): from dimensions to files.
3. `CRequest::handleGetMapRequest` (`CRequest.cpp:2196`): the GetMap loop.
4. `CImageDataWriter::warpImage` (`CImageDataWriter.cpp:865`): read, pick renderer, render.
5. `CDataReader::open` (`CDataReader.cpp:849`): file to memory.
6. `CImgWarpGeneric::render` (`CImgWarpGeneric/CImgWarpGeneric.cpp:174`) and `GenericDataWarper::render` (`CGenericDataWarper.h:359`): reprojection and pixel drawing.

---

# WMS GetCapabilities: code flow and data flow (C++)

`REQUEST=GetCapabilities` follows a much shorter path than GetMap. It never reads pixel data and never creates a `CImageDataWriter`. It also does not go through `process_all_layers`: `process_querystring` dispatches straight to `process_wms_getcap_request` (`CRequest.cpp:123`). `CXMLGen::OGCGetCapabilities` (`CXMLGen.cpp:794`) builds its own `CDataSource` for every configured layer (skipping `type="autoscan"` and `hidden="true"`), so the `LAYERS=` parameter is not used.

## 1. Code flow

Rendered image: [getcap-code-flow.svg](./getcap-code-flow.svg) ([PNG](./getcap-code-flow.png))

```mermaid
sequenceDiagram
    autonumber
    participant Req as CRequest
    participant Gen as CXMLGen
    participant Pop as utils/XMLGenUtils.cpp<br/>populateMetadataLayerStruct
    participant Store as utils/LayerMetadataStore.cpp
    participant DB as CDBAdapterPostgreSQL<br/>(via CDBFactory)
    participant Rdr as CDataReader
    participant Out as stdout

    Req->>Req: process_querystring(): REQUEST=GETCAPABILITIES<br/>→ requestType = REQUEST_WMS_GETCAPABILITIES
    Req->>Req: process_wms_getcap_request()<br/>(skips process_all_layers)
    Req->>Gen: generateOGCGetCapabilities() → OGCGetCapabilities()

    loop each cfg->Layer[j] (skip autoscan and hidden)
        Gen->>Pop: new MetadataLayer, populateMetadataLayerStruct(layer, readFromDB=true)
        Pop->>Pop: new CDataSource + setCFGLayer(-1)<br/>name, group, abstract, title, queryable, EDR flag (config only)
        Pop->>Store: loadLayerMetadataStructFromMetadataDb()
        Store->>DB: getLayerMetadataStore(dataset) → JSON blob "layermetadata"
        alt cache hit
            DB-->>Pop: bbox, grid size, cellsize, projstring, latlonbox, variables
        else miss, disabled (enablemetadatacache=false) or no dataset
            Pop->>Pop: getFileNameForLayer(), getTitleForLayer()
            Pop->>Rdr: open(ds, CNETCDFREADER_MODE_OPEN_HEADER)
            Rdr-->>Pop: geometry, CRS, variables (no pixel data read)
        end
        Pop->>Store: loadLayerDimensionListFromMetadataDb()
        opt miss
            Pop->>DB: getDimsForLayer(): queryTimeStampListFromDb()<br/>makeIntervalFromTimeList() → start/end/period
        end
        Pop->>Pop: getProjectionInformationForLayer(), getStylesForLayer()<br/>de-duplicate and sort projections and dims
    end

    Gen->>Gen: getWMS_1_0_0 / 1_1_1 / 1_3_0_Capabilities(metadataLayerList)<br/>header template + per-layer printf
    Gen->>Gen: delete MetadataLayers and CDataSources
    Gen-->>Req: XMLdocument
    Req->>Out: "Content-Type:text/xml" headers + XML
```

Notes:

- **`populateMetadataLayerStruct`** (`utils/XMLGenUtils.cpp:18`) fills a `MetadataLayer.layerMetadata` struct. The `getWMS_*_Capabilities` generators only format that struct.
- **Metadata cache:** `--updatedb` stores one JSON blob per layer (`updateMetaDataTable`, `utils/LayerMetadataStore.cpp:336`, called from `CDBFileScanner.cpp:995`). When the blob is found, GetCapabilities does no file I/O. It is skipped with `<Settings enablemetadatacache="false">` (`CServerParams::useMetadataTable`) or when no dataset is set. `getLayerMetadataFromDb` throws an `int` when no entry exists, and `loadLayerMetadataStructFromMetadataDb` catches it and returns non-zero, which triggers the slow path.
- **Slow path:** `getFileNameForLayer` picks a representative file and `CDataReader::open` runs in `CNETCDFREADER_MODE_OPEN_HEADER`, which reads geometry, CRS and variable attributes but no grid data.
- **Dimensions:** time dimensions with regular steps are compressed to `start/end/period` by `makeIntervalFromTimeList`; otherwise `handleMultipleValueDim` lists the values.
- **Errors per layer** set `hasError`. The layer is skipped (or gets an XML comment via `addErrorInXMLForMisconfiguredLayer`) and the document is still returned with status 200.
- **Root-level `<CRS>` and `<BoundingBox>`** in the WMS 1.3.0 document come from the first layer without an error (`getFirstLayerWithoutError`), not from the union of all layers.

## 2. Data flow

Rendered image: [getcap-data-flow.svg](./getcap-data-flow.svg) ([PNG](./getcap-data-flow.png))

```mermaid
flowchart TD
    subgraph INPUT["Inputs"]
        XML["ADAGUC_CONFIG XML files<br/>server.xml + dataset.xml + includes"]
        META[("PostgreSQL metadata table<br/>one JSON blob per layer:<br/>layermetadata, projections, dimensions, styles")]
        DIMT[("PostgreSQL dimension tables<br/>path | dimvalue | dimindex")]
        FILE[("Data file header<br/>only on a cache miss")]
    end

    XML -->|"CXMLParser"| CFG["cfg->Layer[]<br/>name, title, abstract, group, Dimension, Styles"]
    CFG -->|"skip autoscan / hidden"| ML["MetadataLayer per layer<br/>+ its own CDataSource"]
    ML -->|"config-only fields"| LM
    META -->|"loadLayerMetadataStructFromMetadataDb"| LM["layerMetadata<br/>name, title, bbox, width/height, cellsize,<br/>projstring, latlonbox, variableList,<br/>projectionList, dimList, styleList"]
    FILE -.->|"cache miss: CDataReader::open<br/>(OPEN_HEADER)"| LM
    META -->|"loadLayerDimensionListFromMetadataDb"| LM
    DIMT -.->|"cache miss: queryTimeStampListFromDb,<br/>makeIntervalFromTimeList"| LM
    CFG -->|"getProjectionInformationForLayer,<br/>getStylesForLayer"| LM
    LM -->|"unique + sorted projections and dims"| GEN["CXMLGen::getWMS_*_Capabilities<br/>header template + per-layer printf<br/>(root CRS/BBOX from first layer without error)"]
    GEN --> OUT["stdout: Content-Type:text/xml + XML"]

    UPD["--updatedb (CDBFileScanner)<br/>updateMetaDataTable()"] -.->|"populateMetadataLayerStruct(false),<br/>store JSON blob"| META
```
