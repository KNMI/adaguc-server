"""
Convert a netcdf dataset to coverage json
"""

from __future__ import annotations
import logging
from datetime import timezone
from typing import List
import math

import netCDF4
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


def get_dim(metadata: dict, dimname: str):
    if "dims" in metadata and metadata["dims"] is not None:
        for dim in metadata["dims"]:
            if dimname == metadata["dims"][dim]["cdfName"]:
                return metadata["dims"][dim]
    return {}


def netcdf_to_covjson(
    metadata: dict,
    netcdfdataset,
    translate_names: dict[str, str],
    translate_dims: dict[str, str],
) -> Coverage:
    """Converts a netcdf dataset to a CoverageJson object

    2022-10-06: Currently this function only handles one variable in the netCDF

    Returns:
        Coverage: Representation of the NetCDF file as Coverage
    """

    axes: dict[str, ValuesAxis] = {}
    ranges: dict[str, NdArrayFloat | TiledNdArrayFloat | AnyUrl] = {}
    parameters: dict[str, Parameter] = {}

    # float ta(time, height, y, x) ;
    netcdfdimname_to_covdimname = {
        "x": "x",
        "y": "y",
        "time": "t",
    }
    netcdfdimname_to_covdimname.update(translate_dims)
    netcdfdimname_to_covdimname["time"] = "t"

    # Loop through the variables in the NetCDF file
    custom_dim_values = []
    custom_dim_name = None
    coverages = []
    collection_parameters = {}
    layer_names = []
    for variablename in netcdfdataset.variables:
        variable = netcdfdataset.variables[variablename]
        layer_name = translate_names.get(variablename, variablename)

        # Find a variable with a grid_mapping attribute, this is a grid
        if "grid_mapping" in variable.ncattrs():
            # Get the names of the variable
            axisnames = []
            shape = []
            for _, dimname in enumerate(variable.dimensions):
                layer_dim_name = get_dim(metadata[layer_name], dimname).get(
                    "serviceName"
                )
                if dimname not in ["x", "y", "time"]:
                    if (
                        layer_dim_name in metadata[layer_name]["dims"]
                        and "hidden" in metadata[layer_name]["dims"][layer_dim_name]
                        and metadata[layer_name]["dims"][layer_dim_name]["hidden"]
                    ):
                        continue
                ncvar = netcdfdataset.variables[dimname]
                if ncvar.size >= 1 and dimname not in ["x", "y", "time"]:
                    if (
                        not metadata[layer_name]["dims"][layer_dim_name]["type"]
                        == "dimtype_vertical"
                    ):
                        custom_dim_name = netcdfdimname_to_covdimname.get(
                            dimname, dimname
                        )
                        custom_dim_values = ncvar[:].tolist()
                        continue
                coverage_axis_name = netcdfdimname_to_covdimname.get(dimname, dimname)
                if (
                    dimname not in ["x", "y", "time"]
                    and metadata[layer_name]["dims"][layer_dim_name]["type"]
                    == "dimtype_vertical"
                ):
                    coverage_axis_name = "z"
                axisnames.append(coverage_axis_name)
                # Fill in the shape object for the NdArray
                shape.append(ncvar.size)

                # Fill in the dimension values for the NdArray
                if dimname == "time":
                    # Convert the date values to datetime objects.
                    # Optional arguments are not handled by netCDF4.num2date therefore we remove them and pass a dict
                    # as parameters.
                    not_none_parameters = {
                        keyword: value
                        for keyword, value in {
                            "times": ncvar[:],
                            "units": ncvar.units,
                            "only_use_cftime_datetimes": False,
                            "only_use_python_datetimes": True,
                            "calendar": getattr(ncvar, "calendar", None),
                        }.items()
                        if value is not None
                    }
                    values = netCDF4.num2date(**not_none_parameters)
                    # netcdf4-python has made the choice to always return timezone naive datetimes, but guarantees
                    # that the time is in UTC. So we now have to manually set UTC timezone. Quite inefficient! See
                    # https://github.com/Unidata/netcdf4-python/issues/357
                    values_tz = [
                        v.replace(tzinfo=timezone.utc) for v in values.tolist()
                    ]
                    axes[coverage_axis_name] = ValuesAxis[AwareDatetime](
                        values=values_tz
                    )
                elif "str" in str(ncvar.dtype):
                    # Assign str values
                    vals = [str(val) for val in ncvar[:]]
                    axes[coverage_axis_name] = ValuesAxis[str](values=vals)
                else:
                    # Assign float values
                    axes[coverage_axis_name] = ValuesAxis[float](
                        values=ncvar[:].data.tolist()
                    )

            if len(custom_dim_values) == 0:
                # Create the ndarray for the ranges object
                ndarray = NdArrayFloat(
                    axisNames=axisnames,
                    shape=shape,
                    values=ma.masked_invalid(variable[:].flatten(order="C")).tolist(),
                )

                # Make the ranges object
                ranges[layer_name] = ndarray

                unit_of_measurement = (
                    variable.getncattr("unit")
                    if "unit" in variable.ncattrs()
                    else "unknown"
                )

                # Add the parameter
                if "label" in metadata[layer_name]["layer"]:
                    parameter_name = metadata[layer_name]["layer"]["variables"][0][
                        "label"
                    ]
                    parameter_description = metadata[layer_name]["layer"]["variables"][
                        0
                    ]["label"]
                    parameter_standard_name = metadata[layer_name]["layer"][
                        "variables"
                    ][0]["standard_name"]
                elif "long_name" in variable.ncattrs():
                    parameter_name = getattr(variable, "long_name")
                    parameter_description = getattr(variable, "long_name")
                    parameter_standard_name = metadata[layer_name]["layer"][
                        "variables"
                    ][0].get("standard_name", getattr(variable, "long_name"))
                elif "standard_name" in variable.ncattrs():
                    parameter_name = getattr(variable, "standard_name")
                    parameter_description = getattr(variable, "standard_name")
                    parameter_standard_name = getattr(variable, "standard_name")
                else:
                    parameter_name = variablename
                    parameter_description = variablename
                    parameter_standard_name = variablename

                parameters[layer_name] = Parameter(
                    # TODO: KDP-1622 Fix the difference in the ObservedProperty between DescribeCoverage from the
                    #  Adaguc Config and the NetCDF values
                    id=parameter_name,
                    observedProperty=ObservedProperty(
                        id=f"{VOCAB_ENDPOINT_URL}{parameter_standard_name}",
                        label={"en": parameter_name},
                    ),
                    description={"en": parameter_description},
                    unit=Unit(
                        symbol=Symbol(
                            value=unit_of_measurement,
                            type="http://www.opengis.net/def/uom/UCUM",
                        )
                    ),
                )
                collection_parameters[layer_name] = parameters[layer_name]
                layer_names.append(layer_name)
                # Define the referencing system, defaulting to latlon
                georeferencesysteminfo = georeferenceinfos[0]

                # Try to detect the georeferencesysteminfo based on the proj string in the crs variable of the netcdf file.
                if "crs" in netcdfdataset.variables:
                    crsvar = netcdfdataset.variables["crs"]
                    georeferencesysteminfo = get_proj_info_from_proj_string(
                        crsvar.proj4_params
                    )

                georeferencesystem = ReferenceSystem(
                    type=georeferencesysteminfo.crstype, id=georeferencesysteminfo.crsid
                )

                georeferencing = ReferenceSystemConnectionObject(
                    system=georeferencesystem, coordinates=georeferencesysteminfo.axes
                )

                temporalreferencesystem = ReferenceSystem(
                    type="TemporalRS", calendar="Gregorian"
                )

                temporalreferencing = ReferenceSystemConnectionObject(
                    system=temporalreferencesystem, coordinates=["t"]
                )

                # Create the domain based on the axes object
                domain = Domain(
                    domainType=DomainType.grid,
                    axes=axes,
                    referencing=[georeferencing, temporalreferencing],
                )

                # Assemble and return the coveragejson based on the domain and the ranges
                coverages.append(
                    Coverage(domain=domain, ranges=ranges, parameters=parameters)
                )
            else:
                step_len = math.prod(shape)
                for cnt, custom_dim_value in enumerate(custom_dim_values):
                    # Create the ndarray for the ranges object
                    ndarray = NdArrayFloat(
                        axisNames=axisnames,
                        shape=shape,
                        values=ma.masked_invalid(
                            variable[:].flatten(order="C")[
                                cnt * step_len : (cnt + 1) * step_len
                            ]
                        ).tolist(),
                    )

                    cnt = cnt + 1

                    # Make the ranges object
                    ranges[layer_name] = ndarray

                    unit_of_measurement = (
                        variable.getncattr("unit")
                        if "unit" in variable.ncattrs()
                        else "unknown"
                    )

                    # Add the parameter
                    if "long_name" in variable.ncattrs():
                        parameter_name = getattr(variable, "long_name")
                        parameter_description = getattr(variable, "long_name")
                    elif "standard_name" in variable.ncattrs():
                        parameter_name = getattr(variable, "standard_name")
                        parameter_description = getattr(variable, "standard_name")
                    else:
                        parameter_name = variablename
                        parameter_description = variablename
                    parameters[layer_name] = Parameter(
                        # TODO: KDP-1622 Fix the difference in the ObservedProperty between DescribeCoverage from the
                        #  Adaguc Config and the NetCDF values
                        observedProperty=ObservedProperty(label={"en": parameter_name}),
                        description={"en": parameter_description},
                        unit=Unit(
                            symbol=Symbol(
                                value=unit_of_measurement,
                                type="http://www.opengis.net/def/uom/UCUM/",
                            )
                        ),
                    )
                    collection_parameters[layer_name] = parameters[layer_name]
                    layer_names.append(layer_name)

                    # Define the referencing system, defaulting to latlon
                    georeferencesysteminfo = georeferenceinfos[0]

                    # Try to detect the georeferencesysteminfo based on the proj string in the crs variable of the netcdf file.
                    if "crs" in netcdfdataset.variables:
                        crsvar = netcdfdataset.variables["crs"]
                        georeferencesysteminfo = get_proj_info_from_proj_string(
                            crsvar.proj4_params
                        )

                    georeferencesystem = ReferenceSystem(
                        type=georeferencesysteminfo.crstype,
                        id=georeferencesysteminfo.crsid,
                    )

                    georeferencing = ReferenceSystemConnectionObject(
                        system=georeferencesystem,
                        coordinates=georeferencesysteminfo.axes,
                    )

                    temporalreferencesystem = ReferenceSystem(
                        type="TemporalRS", calendar="Gregorian"
                    )

                    temporalreferencing = ReferenceSystemConnectionObject(
                        system=temporalreferencesystem, coordinates=["t"]
                    )

                    custom = {f"custom:{custom_dim_name}": custom_dim_value}
                    # Create the domain based on the axes object
                    domain = Domain(
                        domainType=DomainType.grid,
                        axes=axes,
                        referencing=[georeferencing, temporalreferencing],
                        **custom,
                    )

                    # Assemble and return the coveragejson based on the domain and the ranges
                    coverages.append(
                        Coverage(domain=domain, ranges=ranges, parameters=parameters)
                    )

    return coverages
