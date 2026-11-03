# Trajectory implementation

Trajectory samples the native cells crossed by a 2D geographic `LINESTRING` and
returns a CoverageJSON Section. It does not interpolate environmental values.
The supported request contract is documented in
[tutorials/Configure_EDR_service.md](tutorials/Configure_EDR_service.md#trajectory-queries).

## Request and response flow

1. `edr_trajectory.py` validates the supported geometry, CRS, format and options.
   Shared EDR helpers resolve the dataset, instance, parameters and vertical
   selection. `native_query()` URL-encodes one native `GetTrajectory` data call.
2. `CRequest` selects the existing GetFeatureInfo pipeline directly, retaining
   `isTrajectory` to distinguish spatial selection. It validates COORDS rather
   than pretending that X/Y parameters were supplied.
3. The legacy writer still requires a one-pixel image context. This is prepared
   explicitly at request dispatch; its dummy extent is never used for trajectory
   sampling. Removing this dependency would require a broader writer refactor.
4. `CImageDataWriter::getFeatureInfo()` checks trajectory source restrictions and
   enters `CMakeJSONTimeSeries`, which groups selected dimensions by source file.
5. `CURUniqueRequests` combines dimension ranges and executes spatial row runs.
   `trajectorySampling.h` projects the path and builds the sampling plan;
   `trajectory.h` contains WKT parsing, regular-axis checks and cell traversal.
6. Native JSON records are emitted in path order. `trajectory_covjson.py` groups
   them by parameter and other selected dimensions, then constructs Sections.

There is one metadata call and one data call. GetTrajectory/GetFeatureInfo are
internal dispatch choices, not successive HTTP requests.

## Sampling and reading

The plan separates two orders:

- `samples`: cells in path order, including later revisits, with geographic cell
  centres for output.
- `runs`: unique cells sorted by row and combined into contiguous x ranges.

For example, visits `(10,20), (11,20), (11,21), (10,21), (10,20)` produce five
output samples but only two contiguous read runs covering four unique cells.

Segments are straight in input longitude/latitude coordinates. Adaptive
subdivision approximates their transformed shape in native grid coordinates.
Traversal clips each segment, uses half-open cell boundaries, excludes
corner-only touches, and suppresses consecutive duplicate cells. Regular-axis
spacing is estimated across the full axis to tolerate float32 rounding.

Each parameter's source files must share the same grid. Dimension batching and
NetCDF hyperslab reads reuse the existing reader. Hyperslab arrays follow actual
variable dimensions. The legacy header reader still requires spatial dimensions
in the final two positions (either x/y order); a selected nonspatial dimension
misidentified as an axis is rejected. File headers
and reads reuse the request-owned object store. Row runs are not HDF5 chunk
scheduling, and multiple reads may still access a single file.

## Vertical selection and Section output

`vertical_dimension()` is shared by selection, discovery and serialization. It
prefers explicitly typed vertical dimensions, with known height names as a
fallback. Selection only forwards visible coordinates; hidden coordinates retain
the configured native selection.

Thus a layer fixed at a hidden height of 2 m can receive `z=3` and return
`z: [2]`. The response reports the selected coordinate, not the requested value.
Omitted z selects all visible levels. A Section always contains z; parameters
without a recognizable vertical coordinate or time are rejected. Collection
links are advertised only when at least one parameter has those dimensions.
Actual grid compatibility remains a native runtime check.

Native serialization explicitly retains hidden dimensions for Sections,
independently of the optional sample coordinates. Other hidden dimensions remain
available for grouping, but are not exposed as `custom:*` properties.

A Section contains a numeric z axis and a composite `[time, longitude, latitude]`
axis, ordered by time then path. Ranges use `[z, composite]`, flattened level
first. Missing values become null. Multiple parameter/custom-dimension results
produce a CoverageCollection. Local Section models extend the pinned
CoverageJSON library.

## Shared code and intentional differences

Position and trajectory share the JSON reader and native response validation.
Position, trajectory and cube share vertical discovery and query encoding.
Position and trajectory also share data-header propagation. Cube retains its
existing aggregation of trace headers across per-parameter calls.

Cube uses WCS and an intermediate NetCDF response. Position and trajectory use
native JSON. Omitted datetime follows the configured default for position and
trajectory; cube explicitly selects all times. These endpoint policies remain
visible in their handlers.

Expected unsupported trajectory inputs/grids/limits use a dedicated native
exception and become HTTP 422. Missing data is 404, invalid endpoint options are
400, and execution or malformed native-response failures are 500. Native failure
logs are not copied into public error messages.

## Regression coverage

Tests cover cell boundaries, clipping, revisits, float32 axes, rotated grids,
multiple files and parameters, hidden heights including an unmatched requested
height, custom dimensions, scalar processing, missing values, dimension order,
query encoding, discovery and failure categories. Position/cube tests protect
shared helper behavior. Geometry tests run in `testadagucserver`; API tests are
in `python/python_fastapi_server/test_ogc_api_edr.py`.
