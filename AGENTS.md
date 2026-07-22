# AGENTS.md

Guidance for AI coding agents working in this repository.

## What this project is

Adaguc-server is a C++ WMS/WCS geographical information server for meteorological,
climatological and remote sensing data. Core server code lives in `adagucserverEC/`,
the low-level data model in `CCDFDataModel/`, and shared helper classes in `hclasses/`.

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
