"""
Native grid output for EDR: data on the grid of the source data instead of resampled to EPSG:4326
"""

from __future__ import annotations
import math
from functools import lru_cache

from pyproj import CRS
from pyproj.exceptions import CRSError

from .edr_exception import EdrException, exc_failed_call

# Value of the crs parameter that requests the native grid
NATIVE_CRS = "native"
# The largest EDR resolution-x/-y: adaguc-server refuses a WCS width or height above 20000
MAX_POSITIONS = 20000

# Values of the crs parameter that request the default EPSG:4326 output
DEFAULT_CRS_VALUES = (
    "EPSG:4326",
    "CRS84",
    "OGC:CRS84",
    "http://www.opengis.net/def/crs/EPSG/0/4326",
    "http://www.opengis.net/def/crs/OGC/1.3/CRS84",
)

# The crs_details entry of EPSG:4326, for the collection metadata
EPSG_4326_CRS_DETAILS = {
    "crs": "EPSG:4326",
    "wkt": 'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"]]',
}


def check_crs(crs: str | None, allow_native: bool, query_type: str) -> None:
    """Raises a 400 for a crs parameter the query does not support. Only cube queries support crs=native."""
    if crs is None or crs in DEFAULT_CRS_VALUES or (allow_native and crs == NATIVE_CRS):
        return
    supported = f"{NATIVE_CRS} or EPSG:4326" if allow_native else "EPSG:4326"
    raise EdrException(code=400, description=f"Unsupported crs {crs} for {query_type} queries, use {supported}")


# Parameters that PROJ defaults to 0 when they are left out, but that pyproj's to_cf needs for a rotated pole (ob_tran)
OB_TRAN_DEFAULTS = ("o_lon_p", "lon_0")


def add_missing_proj_defaults(projstring: str) -> str:
    """
    Adds the parameters that PROJ defaults, but that pyproj needs to describe the CRS. A rotated pole (ob_tran) without
    +o_lon_p or +lon_0, like "+proj=ob_tran +o_proj=longlat +o_lat_p=35.0 +lon_0=-8.0", is valid for PROJ, which uses 0,
    but pyproj's to_cf fails on it with a KeyError.
    """
    params = projstring.split()
    if "+proj=ob_tran" not in params:
        return projstring
    for key in OB_TRAN_DEFAULTS:
        if not any(param.startswith(f"+{key}=") for param in params):
            projstring += f" +{key}=0"
    return projstring


@lru_cache(maxsize=64)
def get_crs(projstring: str) -> CRS | None:
    """The pyproj CRS for a proj string, or None when it cannot be parsed"""
    try:
        return CRS.from_proj4(add_missing_proj_defaults(projstring))
    except CRSError:
        return None


@lru_cache(maxsize=64)
def get_epsg(projstring: str) -> int | None:
    """The EPSG code of a proj string, or None. Cached, because identifying a projected CRS takes PROJ about 10ms."""
    crs = get_crs(projstring)
    return None if crs is None else crs.to_epsg()


@lru_cache(maxsize=64)
def get_crs_wkt(projstring: str) -> str | None:
    """
    WKT of the CRS of a proj string, or None when it cannot be parsed.

    A rotated pole grid (ob_tran) is written as a CF rotated pole ("Pole rotation (netCDF CF convention)"), which is
    understood by more than PROJ alone. The WKT that PROJ writes for ob_tran itself has the PROJ-only method
    "PROJ ob_tran o_proj=longlat", and turns a "+e=0" in the proj string into a parameter with an angle unit.
    """
    crs = get_crs(projstring)
    if crs is None:
        return None
    try:
        cf = crs.to_cf()
    except (KeyError, CRSError):
        # pyproj cannot write every CRS as CF, for example when it misses a parameter that PROJ defaults
        return crs.to_wkt()
    if cf.get("grid_mapping_name") == "rotated_latitude_longitude":
        cf.pop("crs_wkt", None)  # Otherwise from_cf returns the ob_tran CRS again
        # ob_tran gives the pole longitude as lon_0 - 180, which can be below -180
        cf["grid_north_pole_longitude"] = (cf["grid_north_pole_longitude"] + 180) % 360 - 180
        try:
            return CRS.from_cf(cf).to_wkt()
        except CRSError:
            pass
    return crs.to_wkt()


def get_native_crs_wkt(gridspec: dict | None) -> str | None:
    """WKT of the native grid of a layer, or None when the layer has no grid or its grid is in EPSG:4326 already"""
    if not gridspec or not gridspec.get("projstring"):
        return None
    if get_crs(gridspec["projstring"]) is None or get_epsg(gridspec["projstring"]) == 4326:
        return None
    return get_crs_wkt(gridspec["projstring"])


def get_collection_native_crs_wkt(gridspecs: list[dict | None]) -> str | None:
    """
    WKT of the native projection that all parameters of a collection share, or None. crs=native is only advertised when
    every parameter has the same projection (their grids can differ in extent or cell size): a cube request for several
    parameters must work, and one WKT must describe them all.
    """
    projstrings = {(gridspec or {}).get("projstring") for gridspec in gridspecs}
    if len(gridspecs) == 0 or len(projstrings) != 1:
        return None
    return get_native_crs_wkt(gridspecs[0])


def get_collection_native_crs_details(gridspecs: list[dict | None]) -> dict | None:
    """
    The crs_details entry of crs=native for the collection metadata, or None when the collection has no native grid.
    Next to the WKT it gives the extent of the native grid, so a client knows which bbox to use without converting
    coordinates itself:
    - bbox: minx, miny, maxx, maxy of the outer cell edges, in native coordinates. When the parameters have different
      grids in the same projection, this covers all of them.
    - cellsize and size: the cell size and the number of cells in x and y, only when all parameters have the same grid.
    """
    wkt = get_collection_native_crs_wkt(gridspecs)
    if wkt is None:
        return None
    details = {"crs": NATIVE_CRS, "wkt": wkt}
    try:
        # The gridspec bbox can be ordered top to bottom
        bboxes = [
            (min(bbox[0], bbox[2]), min(bbox[1], bbox[3]), max(bbox[0], bbox[2]), max(bbox[1], bbox[3]))
            for bbox in (gridspec["bbox"] for gridspec in gridspecs)
        ]
    except (KeyError, IndexError, TypeError):
        return details
    details["bbox"] = [min(b[0] for b in bboxes), min(b[1] for b in bboxes), max(b[2] for b in bboxes), max(b[3] for b in bboxes)]
    grids = {(tuple(gridspec["bbox"]), gridspec.get("width"), gridspec.get("height"), gridspec.get("cellsizex"), gridspec.get("cellsizey")) for gridspec in gridspecs}
    if len(grids) == 1 and None not in next(iter(grids)):
        details["cellsize"] = [abs(gridspecs[0]["cellsizex"]), abs(gridspecs[0]["cellsizey"])]
        details["size"] = [gridspecs[0]["width"], gridspecs[0]["height"]]
    return details


def subsample_axis(start: int, end: int, positions: int | None) -> tuple[float, int, int]:
    """
    Every n-th cell of the cells start..end-1 of a grid axis, for at most `positions` values (EDR resolution-x/-y: the
    number of positions; None, 0 or at least the number of cells: every cell).

    Returns (first_edge, count, step): the output grid starts at cell edge `first_edge` (in cells, can be half a cell
    before `start`), has `count` cells of `step` native cells each, and the centre of every output cell is the centre of
    a native cell: the middle one of each block of `step` cells (the lower middle one for an even step). So nearest
    neighbour resampling takes exactly those native cells, without interpolation.
    """
    cells = end - start
    if not positions or positions >= cells:
        return float(start), cells, 1
    return subsample_axis_by_step(start, end, math.ceil(cells / positions))


def subsample_axis_by_step(start: int, end: int, step: int) -> tuple[float, int, int]:
    """
    Every `step`-th cell of the cells start..end-1 of a grid axis, as described in subsample_axis. A step larger than
    the number of cells gives one cell: the middle one.
    """
    step = max(1, min(step, end - start))
    middle = (step - 1) // 2  # offset of the cell taken from each block
    count = (end - start - 1 - middle) // step + 1  # the last taken cell must be inside the range
    # Output cell k has its centre at first_edge + (k + 0.5) * step, which is the centre start + k * step + middle + 0.5
    first_edge = start + middle + 0.5 - 0.5 * step
    return first_edge, count, step


def cellsize_to_step(cellsize: float | None, native_cellsize: float) -> int | None:
    """
    The number of native cells per output cell for a requested cell size in native units (resolution_x/resolution_y
    with crs=native): rounded to a whole number of native cells, at least 1. None when no cell size is requested.
    """
    if cellsize is None:
        return None
    return max(1, math.floor(cellsize / native_cellsize + 0.5))


def get_native_grid_query(
    gridspec: dict | None,
    bbox: str,
    positions_x: int | None = None,
    positions_y: int | None = None,
    cellsize_x: float | None = None,
    cellsize_y: float | None = None,
) -> dict:
    """
    WCS parameters for the cells of the native grid that intersect a bbox. As the EDR standard says, the bbox values are
    in the coordinate system of the crs parameter, so here in native grid coordinates (like rotated degrees). The bbox is
    snapped to whole cells and width and height are the number of cells, so every value is a cell of the native grid,
    without resampling.

    positions_x, positions_y: EDR resolution-x and resolution-y, the number of positions in x and y. When smaller than
    the number of cells, every n-th cell is returned (see subsample_axis): still cells of the native grid, without
    interpolation, but fewer of them. 0 or None: every cell.

    cellsize_x, cellsize_y: resolution_x and resolution_y, the cell size in native units (like metres or rotated
    degrees), rounded to a whole number of native cells (see cellsize_to_step). Every n-th cell is returned, as above.
    An axis takes either a number of positions or a cell size.
    """
    if get_native_crs_wkt(gridspec) is None:
        raise exc_failed_call("crs=native is not available for this collection")
    try:
        x_min, y_min, x_max, y_max = (float(value) for value in bbox.split(","))
    except ValueError as exc:
        raise exc_failed_call(f"Invalid bbox {bbox}") from exc
    if not (x_min < x_max and y_min < y_max):
        raise exc_failed_call(f"Invalid bbox {bbox}, expected minx,miny,maxx,maxy")

    # The gridspec bbox can be ordered top to bottom
    grid_left = min(gridspec["bbox"][0], gridspec["bbox"][2])
    grid_bottom = min(gridspec["bbox"][1], gridspec["bbox"][3])
    native_cellsize_x = abs(gridspec["cellsizex"])
    native_cellsize_y = abs(gridspec["cellsizey"])
    # A bbox edge on a cell edge must not take in the neighbouring cell because of rounding, like 293.0000001 cells.
    # 1e-4 cell also covers coordinates stored as float32, which are off by about 1e-6 cell.
    eps = 1e-4
    col_start = max(0, math.floor((x_min - grid_left) / native_cellsize_x + eps))
    col_end = min(gridspec["width"], math.ceil((x_max - grid_left) / native_cellsize_x - eps))
    row_start = max(0, math.floor((y_min - grid_bottom) / native_cellsize_y + eps))
    row_end = min(gridspec["height"], math.ceil((y_max - grid_bottom) / native_cellsize_y - eps))
    if col_end <= col_start or row_end <= row_start:
        raise exc_failed_call(f"bbox {bbox} is outside the native grid")

    if cellsize_x is not None:
        first_col, width, step_x = subsample_axis_by_step(col_start, col_end, cellsize_to_step(cellsize_x, native_cellsize_x))
    else:
        first_col, width, step_x = subsample_axis(col_start, col_end, positions_x)
    if cellsize_y is not None:
        first_row, height, step_y = subsample_axis_by_step(row_start, row_end, cellsize_to_step(cellsize_y, native_cellsize_y))
    else:
        first_row, height, step_y = subsample_axis(row_start, row_end, positions_y)
    native_bbox = (
        grid_left + first_col * native_cellsize_x,
        grid_bottom + first_row * native_cellsize_y,
        grid_left + (first_col + width * step_x) * native_cellsize_x,
        grid_bottom + (first_row + height * step_y) * native_cellsize_y,
    )
    return {
        "crs": gridspec["projstring"],
        "bbox": ",".join(repr(value) for value in native_bbox),
        "width": width,
        "height": height,
    }
