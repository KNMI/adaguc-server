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


@lru_cache(maxsize=64)
def get_crs(projstring: str) -> CRS | None:
    """The pyproj CRS for a proj string, or None when it cannot be parsed"""
    try:
        return CRS.from_proj4(projstring)
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
    cf = crs.to_cf()
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
    WKT of the native grid that all parameters of a collection share, or None. crs=native is only advertised when every
    parameter has the same native grid: a cube request for several parameters must work, and one WKT must describe them all.
    """
    projstrings = {(gridspec or {}).get("projstring") for gridspec in gridspecs}
    if len(gridspecs) == 0 or len(projstrings) != 1:
        return None
    return get_native_crs_wkt(gridspecs[0])


def get_native_grid_query(gridspec: dict | None, bbox: str) -> dict:
    """
    WCS parameters for the cells of the native grid that intersect a bbox. As the EDR standard says, the bbox values are
    in the coordinate system of the crs parameter, so here in native grid coordinates (like rotated degrees). The bbox is
    snapped to whole cells and width and height are the number of cells, so every value is a cell of the native grid,
    without resampling.
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
    cellsize_x = abs(gridspec["cellsizex"])
    cellsize_y = abs(gridspec["cellsizey"])
    # A bbox edge on a cell edge must not take in the neighbouring cell because of rounding, like 293.0000001 cells.
    # 1e-4 cell also covers coordinates stored as float32, which are off by about 1e-6 cell.
    eps = 1e-4
    col_start = max(0, math.floor((x_min - grid_left) / cellsize_x + eps))
    col_end = min(gridspec["width"], math.ceil((x_max - grid_left) / cellsize_x - eps))
    row_start = max(0, math.floor((y_min - grid_bottom) / cellsize_y + eps))
    row_end = min(gridspec["height"], math.ceil((y_max - grid_bottom) / cellsize_y - eps))
    if col_end <= col_start or row_end <= row_start:
        raise exc_failed_call(f"bbox {bbox} is outside the native grid")

    native_bbox = (
        grid_left + col_start * cellsize_x,
        grid_bottom + row_start * cellsize_y,
        grid_left + col_end * cellsize_x,
        grid_bottom + row_end * cellsize_y,
    )
    return {
        "crs": gridspec["projstring"],
        "bbox": ",".join(repr(value) for value in native_bbox),
        "width": col_end - col_start,
        "height": row_end - row_start,
    }
