# AGENTS.md

Guidance for AI coding agents working in this repository.

## What this project is

Adaguc-server is a C++ WMS/WCS geographical information server for meteorological,
climatological and remote sensing data. Core server code lives in `adagucserverEC/`,
the low-level data model in `CCDFDataModel/`, and shared helper classes in `hclasses/`.

## `adagucserverEC/` — server core

- `adagucserver.cpp` — `main()` entry point.
- `CRequest.cpp/.h` — central request handler (`process_querystring()` dispatches to
  per-operation methods for WMS GetMap/GetCapabilities/GetFeatureInfo/GetLegendGraphic
  and WCS GetCapabilities/DescribeCoverage/GetCoverage).
- `CServerParams.cpp/.h` — parsed request/query params plus server config;
  `CServerConfig_CPPXSD.h` holds the generated classes for `adaguc.dataset.xml`.
- `CDataSource.cpp/.h` — one configured layer/dataset instance to render (style,
  projection, dimensions), built by `CRequest`.
- `CDataReader.cpp/.h` — opens/reads the data for a `CDataSource` via `CCDFDataModel`.
- DB/catalog layer: `CDBAdapterPostgreSQL.cpp/.h` (concrete, no abstract interface)
  wraps `CPGSQLDB.cpp/.h` (libpq); accessed as a singleton through
  `CDBFactory::getDBAdapter()`. `CDBFileScanner.cpp/.h` scans data directories and
  populates the DB. `CDBStore.cpp/.h` holds query result rows/columns.
- `CConvert*` classes translate source-specific formats into the internal CDM model:
  `CConvertADAGUCPoint`/`CConvertADAGUCVector` (ADAGUC point/vector formats),
  `CConvertASCAT` (ASCAT scatterometer wind), `CConvertCurvilinear` (curvilinear
  grids), `CConvertEProfile` (E-PROFILE ceilometer/lidar), `CConvertGeoJSON`,
  `CConvertH5VolScan(Utils)` (HDF5 ODIM radar volume scans), `CConvertHexagon`,
  `CConvertKNMIH5EchoToppen` (KNMI radar echo-top), `CConvertLatLonBnds`/
  `CConvertLatLonGrid` (each split into Header/Data files), `CConvertTROPOMI`
  (Sentinel-5P), `CConvertUGRIDMesh` (unstructured mesh).
- Warping/rendering pipeline: `CImageWarper.cpp/.h` (PROJ-based reprojection),
  `CGenericDataWarper.h` + `GenericDataWarper/` (generic per-pixel warp/draw
  dispatch), `CImgWarpGeneric/` and siblings (`CImgWarpNearestNeighbour`,
  `CImgWarpNearestRGBA`, `CImgWarpBilinear`, `CImgWarpHillShaded`) implement concrete
  warpers, `CDrawImage.cpp/.h` is the canvas, `CCairoPlotter.cpp/.h` is the
  Cairo-based plotting backend, and `CImageDataWriter`/`CGDALDataWriter`/
  `CNetCDFDataWriter` encode output (PNG/GDAL/NetCDF).

Subdirectories:

- `CDataPostProcessors/` — pluggable post-processing steps on read data (unit
  conversion, clipping, Beaufort, UV components, feature filtering, etc.) via
  `CDPPInterface.h` — see "Adding a new data post processor" below.
- `Types/` — shared value types (`GeoParameters`, `ProjectionStore`,
  `LayerMetadataType.h`).
- `CImageOperators/` — raster ops (`drawContour`, `smoothRasterField`).
- `CImgRenderers/` — point/vector style rendering (`getPointStyle`, `getVectorStyle`).
- `CUniqueRequests/` — dedupes/coordinates identical concurrent requests.
- `CLegendRenderers/` — legend image generation (continuous/discrete variants).
- `utils/` — query string parsing, config utils, layer metadata, geometry utils, logging.
- `LayerTypeLiveUpdate/` — live-updating (auto-refreshing) layer type support.

## `CCDFDataModel/` — in-memory data model

An in-memory, CF-NetCDF-like generic data model that represents variables,
dimensions and attributes uniformly regardless of source format.

- `CCDFObject.cpp/.h` — top-level container (like a NetCDF dataset): variables,
  dimensions, global attributes.
- `CCDFVariable.cpp/.h` — a variable (data plus its own attributes/dimensions).
- `CCDFAttribute.cpp/.h` — key/value metadata attribute.
- `CCDFDimension.h` — dimension definition.
- `CCDFStore.cpp/.h` — storage/collection management for objects.
- `CCDFReader.h` — abstract `CDFReader` interface (`open()`, `close()`,
  `_readVariableData()`). Format-specific readers implement it as a strategy so
  `CDataReader` (in `adagucserverEC/`) can read any backing format into the same
  `CCDFObject` model: `CCDFNetCDFIO` (NetCDF), `CCDFHDF5IO`/`CCDFHDF5IO_ODIM.cpp`
  (HDF5, incl. ODIM radar conventions), `CCDFPNGIO`, `CCDFGeoJSONIO`, `CCDFCSVReader`.
- `traceTimings/` — lightweight tracing utility. Public API:
  `traceTimingsEnableAndInit()`, `traceTimingsSpanStart(TraceTimingType)`,
  `traceTimingsSpanEnd(TraceTimingType)`, `traceTimingsCheckEnabled()`.

## `hclasses/` — shared low-level utilities

Project-agnostic utility classes used by both `adagucserverEC/` and
`CCDFDataModel/`.

- `CTString.cpp/.h` — `CT::string`, the custom string class used pervasively
  throughout the codebase. Prefer `std::string` in new/changed code unless you
  specifically need `CT::string`-only API (e.g. `concat`, `printconcat`,
  `equalsIgnoreCase`, `startsWith`, `split`, `substring`) — the two convert
  implicitly, but `CT::string`-only methods don't exist on `std::string`.
- `CDebugger.h` — `CDBDebug`/`CDBWarning`/`CDBError`, printf-style variadic
  logging macros; `%s` arguments need `.c_str()` (passing `std::string` directly
  is undefined behaviour). Don't add `.c_str()` where it's not needed elsewhere
  (e.g. `nlohmann::json::parse()` accepts a `std::string` directly).
- `CReportMessage.cpp/.h` / `CReporter.cpp/.h` / `CReportWriter.cpp/.h` — a
  message-with-severity (`INFO`/`WARNING`/`ERROR`) reporting mechanism collected
  via a `CReporter` singleton and written out as a JSON report
  (`CReportWriter::writeJSONReportToFile()`).
- `CStopWatch.cpp/.h` — timing utility backing the `StopWatch_Stop` macro used
  around the codebase. When gating debug/timing statements, prefer a file-local
  `static const bool debug = false;` / `static const bool measureTime = false;`
  flag with a plain `if (...)` over an `#ifdef`/`#define` macro — the guarded code
  stays compiled (and therefore doesn't silently rot or hide bugs) even when the
  flag is off.
- `CDirReader.cpp/.h` — directory listing/traversal.
- `CHTTPTools.cpp/.h` — HTTP request helpers.
- `CReadFile.cpp/.h` — generic file reading.
- `CXMLParser.cpp/.h` — XML parsing.
- `json_adaguc.h/.cpp` + `json.hpp` — `nlohmann::json` integration (`using json = nlohmann::json;`).
- `CppUnitLite/` — bundled minimal C++ unit-test framework used by the project's
  tests (e.g. `testhclasses.cpp`, `testccdfdatamodel.cpp`).

## Building and testing

Use `./compile.sh` from the repository root to build. It configures CMake into `./bin`
and builds there.

- `./compile.sh --debug` — Debug build, then runs `ctest --verbose`. **Use this to test
  changes** after editing C++ code (e.g. data post processors, readers, renderers).
- `./compile.sh` (no args) — Release build, then runs the test suite.
- `./compile.sh --sanitize` — Build with sanitizers enabled.
- `./compile.sh --profile` — Profiling build.
- `./compile.sh --clean` — Wipes build artifacts (`bin/`, `CMakeFiles`, `CMakeCache.txt`
  in the root and in `hclasses/`, `CCDFDataModel/`, `adagucserverEC/`) and reconfigures.
  Only run this when a stale build is actually causing problems.

The build requires a working CMake generator/toolchain (e.g. `Unix Makefiles` or Ninja)
and the project's C/C++ compilers to be available on `PATH`.

## Adding a new data post processor

Data post processors live in `adagucserverEC/CDataPostProcessors/`. To add one:

1. Create `CDataPostProcessor_<Name>.h` / `.cpp`, subclassing `CDPPInterface`
   (see `CDPPInterface.h`) and following the pattern of an existing processor such as
   `CDataPostProcessor_PointsFromGrid.cpp`.
2. Register the new class in `CDataPostProcessor.cpp`: add the `#include` and a
   `dataPostProcessorList->push_back(new CDPP<Name>());` line in `CDPPExecutor::CDPPExecutor()`.
3. Add both new files to the source list in `adagucserverEC/CMakeLists.txt`.
4. Build and test with `./compile.sh --debug`.

## Conventions

- Don't add error handling for cases that can't occur; trust internal invariants.
- Match the existing style of the file/class you're extending rather than introducing
  new patterns.