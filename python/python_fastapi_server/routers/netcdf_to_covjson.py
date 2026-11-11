"""
Convert a netcdf dataset to coverage json
"""

from __future__ import annotations
import logging
from datetime import timezone
from typing import Any, List
import math

import netCDF4
import numpy as np
from numpy.typing import ArrayLike
from numpy import ma
from covjson_pydantic.coverage import Coverage
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

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

VOCAB_ENDPOINT_URL = "https://vocab.nerc.ac.uk/standard_name/"


class GeoReferenceInfo(BaseModel):
    """GeoReference object"""

    proj: str
    crsid: str
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
    # TODO Maybe indicate that the correct georeference info system was not found.
    return georeferenceinfos[0]


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

    return ValuesAxis[float](values=ncvar[:].data.tolist())


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


def coverage_values(values: ArrayLike) -> list[float | None]:
    """Flatten in C order, replacing masked or non-finite values with None."""
    flat = ma.asarray(values).ravel(order="C")

    # Find invalid/masked values
    missing = ~np.isfinite(flat.data) | ma.getmask(flat)
    missing_count = np.count_nonzero(missing)

    # All values are valid, shortcut to immediately return data
    if missing_count == 0:
        return flat.data.tolist()

    # All invalid/missing values, shortcut to return all None.
    if missing_count == flat.size:
        return [None] * flat.size

    return ma.array(flat.data, mask=missing, copy=False).tolist()


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
    custom_dim_values = []
    custom_dim_name = None
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
        for dimname in variable.dimensions:
            dimension_metadata = dimensions_by_cdf_name.get(dimname, {})
            is_spatial_or_time = dimname in ("x", "y", "time")
            if not is_spatial_or_time and dimension_metadata.get("hidden", False):
                continue

            ncvar = netcdfdataset.variables[dimname]
            is_vertical = not is_spatial_or_time and dimension_metadata["type"] == "dimtype_vertical"
            if ncvar.size >= 1 and not is_spatial_or_time and not is_vertical:
                custom_dim_name = netcdfdimname_to_covdimname.get(dimname, dimname)
                custom_dim_values = ncvar[:].tolist()
                continue

            coverage_axis_name = "z" if is_vertical else netcdfdimname_to_covdimname.get(dimname, dimname)
            axisnames.append(coverage_axis_name)
            # Fill in the shape object for the NdArray
            shape.append(ncvar.size)

            axes[coverage_axis_name] = netcdf_coordinate_axis(ncvar, dimname)

        unit_of_measurement = variable.getncattr("unit") if "unit" in variable.ncattrs() else "unknown"
        unit = Unit(symbol=Symbol(value=unit_of_measurement, type="http://www.opengis.net/def/uom/UCUM/"))
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

        flat_values = variable[:].ravel(order="C")
        step_len = math.prod(shape)
        for index, custom_dim_value in enumerate(custom_dim_values or [None]):
            values = flat_values
            custom = {}
            if custom_dim_values:
                values = flat_values[index * step_len : (index + 1) * step_len]
                custom = {f"custom:{custom_dim_name}": custom_dim_value}

            ranges[layer_name] = NdArrayFloat(axisNames=axisnames, shape=shape, values=coverage_values(values))
            domain = Domain(domainType=DomainType.grid, axes=axes, referencing=referencing, **custom)
            coverages.append(Coverage(domain=domain, ranges=ranges, parameters=parameters))

    return coverages
