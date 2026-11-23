"""
Convert a netcdf dataset to coverage json
"""

from __future__ import annotations
import logging
from datetime import timezone
from typing import Any, List
from functools import lru_cache

import netCDF4
import numpy as np
from numpy.typing import ArrayLike
from numpy import ma
from covjson_pydantic.coverage import Coverage
from covjson_pydantic.coverage import CoverageCollection
from covjson_pydantic.domain import Domain
from covjson_pydantic.domain import DomainType
from covjson_pydantic.domain import ValuesAxis
from covjson_pydantic.ndarray import NdArrayFloat
from covjson_pydantic.ndarray import TiledNdArrayFloat
from covjson_pydantic.observed_property import ObservedProperty
from covjson_pydantic.parameter import Parameter
from covjson_pydantic.reference_system import ReferenceSystem
from covjson_pydantic.reference_system import ReferenceSystemConnectionObject
from covjson_pydantic.unit import Symbol
from covjson_pydantic.unit import Unit
from pydantic import AnyUrl
from pydantic import AwareDatetime
from pydantic import BaseModel

from pyproj import CRS
from pyproj.exceptions import CRSError

from .utils.edr_utils import SYMBOL_TYPE_URL
from .utils.native_grid import get_crs, get_epsg

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

VOCAB_ENDPOINT_URL = "https://vocab.nerc.ac.uk/standard_name/"


class GeoReferenceInfo(BaseModel):
    """GeoReference object"""

    proj: str
    crsid: str | None = None
    crstype: str
    axes: list[str]


GeoReferenceInfos = List[GeoReferenceInfo]


georeferenceinfos: GeoReferenceInfos = [
    GeoReferenceInfo(
        proj="+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs",
        crsid="http://www.opengis.net/def/crs/EPSG/0/4326",
        crstype="GeographicCRS",
        axes=["y", "x"],
    ),
    GeoReferenceInfo(
        proj=(
            "+proj=sterea +lat_0=52.15616055555555 +lon_0=5.38763888888889 +k=0.9999079"
            " +x_0=155000 +y_0=463000 +ellps=bessel"
            " +towgs84=565.4171,50.3319,465.5524,-0.398957388243134,0.343987817378283,-1.87740163998045,4.0725"
            " +units=m +no_defs"
        ),
        crsid="http://www.opengis.net/def/crs/EPSG/0/28992",
        crstype="ProjectedCRS",
        axes=["x", "y"],
    ),
    GeoReferenceInfo(
        proj=(
            "+proj=sterea +lat_0=52.15616055555555 +lon_0=5.38763888888889"
            " +k=0.9999079 +x_0=155000 +y_0=463000 +ellps=bessel +units=m +no_defs"
        ),
        crsid="http://www.opengis.net/def/crs/EPSG/0/28992",
        crstype="ProjectedCRS",
        axes=["x", "y"],
    ),
]


def get_proj_info_from_proj_string(projstring: str) -> GeoReferenceInfo:
    """Find the GeoReferenceInfo based on a proj4 string

    Args:
        projstring (str): _description_

    Returns:
        GeoReferenceInfo: _description_
    """
    for georeferenceinfo in georeferenceinfos:
        if georeferenceinfo.proj == projstring:
            return georeferenceinfo
    crs = get_crs(projstring)
    epsg = get_epsg(projstring)
    if crs is None or epsg == 4326:
        return georeferenceinfos[0]
    # Other projections, like the rotated pole grid of crs=native: with the EPSG id when there is one. Without one the id is
    # left out, as CoverageJSON prescribes for an undefined CRS. The WKT is in the crs_details of the collection.
    return GeoReferenceInfo(
        proj=projstring,
        crsid=f"http://www.opengis.net/def/crs/EPSG/0/{epsg}" if epsg is not None else None,
        crstype="GeographicCRS" if crs.is_geographic else "ProjectedCRS",
        axes=get_epsg_axis_order(epsg),
    )


@lru_cache(maxsize=64)
def get_epsg_axis_order(epsg: int | None) -> list[str]:
    """The axis order of an EPSG CRS, like ["y", "x"] for geographic CRSs with latitude first. ["x", "y"] without EPSG code."""
    if epsg is not None:
        try:
            axis_info = CRS.from_epsg(epsg).axis_info
            if axis_info and axis_info[0].direction in ("north", "south"):
                return ["y", "x"]
        except CRSError:
            pass
    return ["x", "y"]


def netcdf_coordinate_axis(ncvar: netCDF4.Variable, dimname: str) -> ValuesAxis[AwareDatetime] | ValuesAxis[str] | ValuesAxis[float]:
    """Convert a NetCDF coordinate variable to a CoverageJSON axis."""
    if dimname == "time":
        # num2date does not accept None for optional arguments such as calendar.
        date_options = {
            key: value
            for key, value in {
                "times": ncvar[:],
                "units": ncvar.units,
                "only_use_cftime_datetimes": False,
                "only_use_python_datetimes": True,
                "calendar": getattr(ncvar, "calendar", None),
            }.items()
            if value is not None
        }
        values = netCDF4.num2date(**date_options)
        # NetCDF dates are UTC but timezone-naive; CoverageJSON needs aware dates.
        return ValuesAxis[AwareDatetime](values=[value.replace(tzinfo=timezone.utc) for value in values.tolist()])

    if "str" in str(ncvar.dtype):
        return ValuesAxis[str](values=[str(value) for value in ncvar[:]])

    # Coordinates are written like the values, with the digits they have: -13.35, not -13.350000000000001
    return ValuesAxis[float](values=round_to_data_precision(ncvar[:].data).tolist())


def netcdf_coverage_referencing(dataset: netCDF4.Dataset) -> list[ReferenceSystemConnectionObject]:
    """Build spatial and temporal references, defaulting to latitude/longitude."""
    info = georeferenceinfos[0]
    if "crs" in dataset.variables:
        info = get_proj_info_from_proj_string(dataset.variables["crs"].proj4_params)
    return [
        ReferenceSystemConnectionObject(
            system=ReferenceSystem(type=info.crstype, id=info.crsid),
            coordinates=info.axes,
        ),
        ReferenceSystemConnectionObject(
            system=ReferenceSystem(type="TemporalRS", calendar="Gregorian"),
            coordinates=["t"],
        ),
    ]


# Significant digits that values are written with. A 32 bit float is told apart from every other by 9 digits, and
# often by 7 or 8. 12 digits are far more than packed data and measurements have; a 64 bit float can hold 15 to 17.
FLOAT32_SIGNIFICANT_DIGITS = (7, 8, 9)
FLOAT64_SIGNIFICANT_DIGITS = 12
# Whole numbers below this are written as they are: they are exact, and a count, an identifier or a time in milliseconds
# can have more than 12 digits. Noise of a calculation cannot turn one whole number of this size into another.
FLOAT64_EXACT_WHOLE_NUMBERS_BELOW = 1e15


# The powers of ten that a 64 bit float holds exactly: 10^0 to 10^22. They are written out and not calculated: 10^23 is
# not exact, and numpy calculates it as 1e23 or as 1.0000000000000001e23, depending on the processor it runs on.
EXACT_POWERS_OF_TEN = np.array([float(f"1e{power}") for power in range(23)])


def round_significant(values: np.ndarray, digits: int) -> np.ndarray:
    """
    64 bit floats rounded to `digits` significant digits: scaled by a power of ten to a whole number and back. The whole
    number and the power of ten are exact, and dividing or multiplying two exact numbers gives the 64 bit float nearest
    to the decimal, which is written as that decimal. This needs a power of ten that is exact, so of at most 10^22
    (see EXACT_POWERS_OF_TEN); the few values that need a larger one, like 1e-12 for 12 digits, are rounded as text.
    Zero, infinite and NaN are returned as they are.
    """
    rounded = values.copy()
    with np.errstate(divide="ignore", invalid="ignore"):
        power = digits - 1 - np.floor(np.log10(np.abs(values)))
    finite = np.isfinite(power)
    largest = len(EXACT_POWERS_OF_TEN) - 1

    # Up to a whole number and back: 3.8000000000000003 * 10^11 = 380000000000, / 10^11 = 3.8
    multiply = finite & (power >= 0) & (power <= largest)
    scale = EXACT_POWERS_OF_TEN[power[multiply].astype(np.intp)]
    rounded[multiply] = np.round(values[multiply] * scale) / scale

    # Values with more than `digits` digits before the decimal point, down to a whole number and back
    divide = finite & (power < 0) & (power >= -largest)
    scale = EXACT_POWERS_OF_TEN[(-power[divide]).astype(np.intp)]
    rounded[divide] = np.round(values[divide] / scale) * scale

    # Too small or too large for an exact power of ten
    other = finite & (np.abs(power) > largest)
    if np.any(other):
        rounded[other] = [float(f"{value:.{digits - 1}e}") for value in values[other].tolist()]
    return rounded


def round_to_data_precision(values: np.ndarray) -> np.ndarray:
    """
    Values as 64 bit floats that are written in JSON with as many digits as the data has, instead of with the noise of
    how they are stored:
    - A 32 bit float written as it is gives 17 digits, like -21.036102294921875 or 3.799999952316284. It is rounded to
      the fewest of 7, 8 or 9 significant digits that still give the same 32 bit float: -21.036102 and 3.8.
    - Packed data unpacked to 64 bit, like 38 * 0.1 = 3.8000000000000003: rounded to 12 significant digits (3.8).
    This makes the JSON about half as long, and about a fifth smaller after compression.

    For 32 bit floats nothing is lost: the value written gives the same 32 bit float. A 64 bit float changes by at most
    5 parts in 10^12, far less than the precision of packed data or of a measurement; a 64 bit value that really has more
    than 12 significant digits loses the digits after the 12th. Whole numbers below 10^15 are not rounded, see
    FLOAT64_EXACT_WHOLE_NUMBERS_BELOW.

    Integers are returned as they are. Values that are not finite are left for the caller.
    """
    if not np.issubdtype(values.dtype, np.floating):
        return values
    doubles = values.astype(np.float64)
    if values.dtype.itemsize > 4:
        rounded = round_significant(doubles, FLOAT64_SIGNIFICANT_DIGITS)
        with np.errstate(invalid="ignore"):
            whole = (np.abs(doubles) < FLOAT64_EXACT_WHOLE_NUMBERS_BELOW) & (doubles == np.floor(doubles))
        rounded[whole] = doubles[whole]
        return rounded
    # 32 bit floats: more digits for the values that fewer digits do not give back
    rounded = doubles.copy()
    todo = np.arange(values.size)
    for digits in FLOAT32_SIGNIFICANT_DIGITS:
        candidates = round_significant(doubles[todo], digits)
        rounded[todo] = candidates
        todo = todo[candidates.astype(np.float32) != values[todo]]
        if todo.size == 0:
            break
    return rounded


def coverage_values(values: ArrayLike) -> list[float | None]:
    """
    Flatten in C order, replacing masked or non-finite values with None. The values are rounded to the precision of
    the data, see round_to_data_precision.
    """
    flat = ma.asarray(values).ravel(order="C")

    # Find invalid/masked values
    missing = ~np.isfinite(flat.data) | ma.getmask(flat)
    missing_count = np.count_nonzero(missing)

    # All invalid/missing values, shortcut to return all None.
    if missing_count == flat.size:
        return [None] * flat.size

    data = round_to_data_precision(flat.data)

    # All values are valid, shortcut to immediately return data
    if missing_count == 0:
        return data.tolist()

    return ma.array(data, mask=missing, copy=False).tolist()


def netcdf_to_covjson(
    metadata: dict[str, Any],
    netcdfdataset: netCDF4.Dataset,
    translate_names: dict[str, str],
    translate_dims: dict[str, str],
) -> list[Coverage]:
    """Converts a netcdf dataset to a CoverageJson object

    2022-10-06: Currently this function only handles one variable in the netCDF

    Returns:
        list[Coverage]: CoverageJSON objects for the variables and custom-dimension values
    """

    axes: dict[str, ValuesAxis[AwareDatetime] | ValuesAxis[str] | ValuesAxis[float]] = {}
    ranges: dict[str, NdArrayFloat | TiledNdArrayFloat | AnyUrl] = {}
    parameters: dict[str, Parameter] = {}

    # float ta(time, height, y, x) ;
    netcdfdimname_to_covdimname = {"x": "x", "y": "y", "time": "t"}
    netcdfdimname_to_covdimname.update(translate_dims)
    netcdfdimname_to_covdimname["time"] = "t"

    # Loop through the variables in the NetCDF file
    coverages = []
    referencing: list[ReferenceSystemConnectionObject] | None = None

    for variablename in netcdfdataset.variables:
        variable = netcdfdataset.variables[variablename]
        layer_name = translate_names.get(variablename, variablename)

        # Skip non-grid variables.
        if "grid_mapping" not in variable.ncattrs():
            continue

        if referencing is None:
            referencing = netcdf_coverage_referencing(netcdfdataset)
        layer_metadata = metadata[layer_name]
        layer_dimensions = layer_metadata.get("dims") or {}
        dimensions_by_cdf_name = {dim["cdfName"]: dim for dim in layer_dimensions.values()}

        # Get the names of the variable
        axisnames = []
        shape = []
        custom_dim_values = []
        custom_dim_name = None
        custom_dim_axis = None
        for dim_index, dimname in enumerate(variable.dimensions):
            dimension_metadata = dimensions_by_cdf_name.get(dimname, {})
            is_spatial_or_time = dimname in ("x", "y", "time")
            if not is_spatial_or_time and dimension_metadata.get("hidden", False):
                continue

            ncvar = netcdfdataset.variables[dimname]
            is_vertical = not is_spatial_or_time and dimension_metadata["type"] == "dimtype_vertical"
            if ncvar.size >= 1 and not is_spatial_or_time and not is_vertical:
                custom_dim_name = netcdfdimname_to_covdimname.get(dimname, dimname)
                custom_dim_axis = dim_index
                custom_dim_values = ncvar[:].tolist()
                continue

            coverage_axis_name = "z" if is_vertical else netcdfdimname_to_covdimname.get(dimname, dimname)
            axisnames.append(coverage_axis_name)
            # Fill in the shape object for the NdArray
            shape.append(ncvar.size)

            axes[coverage_axis_name] = netcdf_coordinate_axis(ncvar, dimname)

        unit_of_measurement = variable.getncattr("unit") if "unit" in variable.ncattrs() else "unknown"
        unit = Unit(symbol=Symbol(value=unit_of_measurement, type=SYMBOL_TYPE_URL))
        parameter_name = getattr(variable, "long_name", getattr(variable, "standard_name", variablename))

        if len(custom_dim_values) == 0:
            # Add the parameter
            layer = layer_metadata["layer"]
            if "label" in layer or "long_name" in variable.ncattrs():
                parameter_metadata = layer["variables"][0]
            if "label" in layer:
                parameter_name = parameter_metadata["label"]
                parameter_standard_name = parameter_metadata["standard_name"]
            elif "long_name" in variable.ncattrs():
                parameter_standard_name = parameter_metadata.get("standard_name", getattr(variable, "long_name"))
            elif "standard_name" in variable.ncattrs():
                parameter_standard_name = getattr(variable, "standard_name")
            else:
                parameter_standard_name = variablename

            parameters[layer_name] = Parameter(
                # TODO: KDP-1622 Fix the difference in the ObservedProperty between DescribeCoverage from the
                #  Adaguc Config and the NetCDF values
                id=parameter_name,
                observedProperty=ObservedProperty(
                    id=f"{VOCAB_ENDPOINT_URL}{parameter_standard_name}",
                    label={"en": parameter_name},
                ),
                description={"en": parameter_name},
                unit=unit,
            )
        else:
            parameters[layer_name] = Parameter(
                # TODO: KDP-1622 Fix the difference in the ObservedProperty between DescribeCoverage from the
                #  Adaguc Config and the NetCDF values
                observedProperty=ObservedProperty(label={"en": parameter_name}),
                description={"en": parameter_name},
                unit=unit,
            )

        variable_values = variable[:]
        selection: list[slice | int] = [slice(None)] * variable.ndim
        for index, custom_dim_value in enumerate(custom_dim_values or [None]):
            values = variable_values
            custom = {}
            if custom_dim_axis is not None:
                # A member doesn't have to be the outermost dimension (for example, time, member, y, x).
                # Select the custom dim value before flattening the other axes.
                selection[custom_dim_axis] = index
                values = variable_values[tuple(selection)]
                custom = {f"custom:{custom_dim_name}": custom_dim_value}

            ranges[layer_name] = NdArrayFloat(axisNames=axisnames, shape=shape, values=coverage_values(values))
            domain = Domain(domainType=DomainType.grid, axes=axes, referencing=referencing, **custom)
            coverages.append(Coverage(domain=domain, ranges=ranges, parameters=parameters))

    return coverages


class CubeParameterError(Exception):
    """The NetCDF of a parameter could not be read, for example because adaguc-server answered with an error message"""


def cube_responses_to_covjson(
    responses: list[tuple[str, bytes]],
    metadata: dict[str, Any],
    translate_names: dict[str, str],
    translate_dims: dict[str, str],
) -> bytes | None:
    """
    The CoverageJSON, serialized, of the NetCDF that adaguc-server returned for each parameter of a cube request:
    one Coverage, or a CoverageCollection. None when there is no coverage.

    Runs in a separate process (see routers/edr_cube.py): for a large cube, like millions of values, converting and
    serializing takes seconds of Python work that holds the GIL, which would hold up all other requests of the server.

    Raises:
        CubeParameterError: with the parameter name, when its NetCDF cannot be read
    """
    coverages: list[Coverage] = []
    parameters: dict[str, Parameter] = {}
    for parameter_name, netcdf in responses:
        try:
            with netCDF4.Dataset(f"{parameter_name}.nc", memory=netcdf) as dataset:
                parameter_coverages = netcdf_to_covjson(metadata, dataset, translate_names, translate_dims)
        except OSError as exc:
            raise CubeParameterError(parameter_name) from exc
        for coverage in parameter_coverages or []:
            coverages.append(coverage)
            parameters = parameters | coverage.parameters
    if not coverages:
        return None
    result: BaseModel = coverages[0] if len(coverages) == 1 else CoverageCollection(coverages=coverages, parameters=parameters)
    return result.model_dump_json(exclude_none=True, by_alias=True).encode("utf-8")
