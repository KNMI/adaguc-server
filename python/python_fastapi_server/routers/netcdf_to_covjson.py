"""
Convert a netcdf dataset to coverage json
"""

import logging
from datetime import timezone
from typing import Dict
from typing import List

import netCDF4
import numpy.ma as ma
from covjson_pydantic.coverage import Coverage
from covjson_pydantic.coverage import Union
from covjson_pydantic.domain import Domain
from covjson_pydantic.domain import DomainType
from covjson_pydantic.domain import ValuesAxis
from covjson_pydantic.ndarray import NdArray
from covjson_pydantic.ndarray import TiledNdArray
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


class GeoReferenceInfo(BaseModel):
    """GeoReference object"""

    proj: str
    crsid: str
    crstype: str
    axes: List[str]


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


def netcdf_to_covjson(
    netcdfdataset, translated_names: dict[str, str], vertical_dim: dict[str, list[str]]
) -> Coverage:
    """Converts a netcdf dataset to a CoverageJson object

    2022-10-06: Currently this function only handles one variable in the netCDF

    Returns:
        Coverage: Representation of the NetCDF file as Coverage
    """

    axes: Dict[str, ValuesAxis] = {}
    ranges: Dict[str, Union[NdArray, TiledNdArray, AnyUrl]] = {}
    parameters: Dict[str, Parameter] = {}

    # float ta(time, height, y, x) ;
    netcdfdimname_to_covdimname = {
        "temp_at_hagl": "z",
        "height": "z",
        "x": "x",
        "y": "y",
        "time": "t",
    }
    print("vertical_dim: ", vertical_dim)
    if vertical_dim:
        netcdfdimname_to_covdimname[vertical_dim] = "z"

    # Loop through the variables in the NetCDF file
    for variablename in netcdfdataset.variables:
        variable = netcdfdataset.variables[variablename]
        translated_variablename = translated_names.get(variablename, variablename)

        # Find a variable with a grid_mapping attribute, this is a grid
        if "grid_mapping" in variable.ncattrs():
            # Get the names of the variable
            axesnames: List = [None] * len(variable.dimensions)
            shape = [None] * len(variable.dimensions)
            for index, dimname in enumerate(variable.dimensions):
                coverage_axis_name = netcdfdimname_to_covdimname.get(dimname, dimname)
                print(
                    "TRANSLATED ",
                    dimname,
                    "to",
                    coverage_axis_name,
                    netcdfdimname_to_covdimname,
                )
                axesnames[index] = coverage_axis_name
                ncvar = netcdfdataset.variables[dimname]
                # Fill in the shape object for the NdArray
                shape[index] = ncvar.size

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
                    # pylint: disable=no-member
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
                else:
                    # Assign float values
                    axes[coverage_axis_name] = ValuesAxis[float](
                        values=ncvar[:].data.tolist()
                    )

            # Create the ndarray for the ranges object
            ndarray = NdArray(
                axisNames=axesnames,
                shape=shape,
                values=ma.masked_invalid(variable[:].flatten(order="C")).tolist(),
            )

            # Make the ranges object
            ranges[translated_variablename] = ndarray

            unit_of_measurement = variable.units if variable.units else "unknown"

            # Add the parameter
            parameters[translated_variablename] = Parameter(
                # TODO: KDP-1622 Fix the difference in the ObservedProperty between DescribeCoverage from the
                #  Adaguc Config and the NetCDF values
                observedProperty=ObservedProperty(label={"en": variable.long_name}),
                description={"en": variable.long_name},
                unit=Unit(
                    symbol=Symbol(
                        value=unit_of_measurement,
                        type="http://www.opengis.net/def/uom/UCUM/",
                    )
                ),
            )

    # Define the referencing system, defaulting to latlon
    georeferencesysteminfo = georeferenceinfos[0]

    # Try to detect the georeferencesysteminfo based on the proj string in the crs variable of the netcdf file.
    if "crs" in netcdfdataset.variables:
        crsvar = netcdfdataset.variables["crs"]
        georeferencesysteminfo = get_proj_info_from_proj_string(crsvar.proj4_params)

    georeferencesystem = ReferenceSystem(
        type=georeferencesysteminfo.crstype, id=georeferencesysteminfo.crsid
    )

    georeferencing = ReferenceSystemConnectionObject(
        system=georeferencesystem, coordinates=georeferencesysteminfo.axes
    )

    temporalreferencesystem = ReferenceSystem(type="TemporalRS", calendar="Gregorian")

    temporalreferencing = ReferenceSystemConnectionObject(
        system=temporalreferencesystem, coordinates=["t"]
    )

    logger.info("DUMP: %s", axes.keys())

    # Create the domain based on the axes object
    domain = Domain(
        domainType=DomainType.grid,
        axes=axes,
        referencing=[georeferencing, temporalreferencing],
    )

    # Assemble and return the coveragejson based on the domain and the ranges
    return Coverage(domain=domain, ranges=ranges, parameters=parameters)


if __name__ == "__main__":
    # Run with python3.10 api/application/app/netcdf_to_covjson.py
    import requests

    SERVICE = "https://geoservices.knmi.nl/adagucserver?"
    SERVICE = "http://localhost:8080/adagucserver?"

    # Rijksdriehoek stelsel
    # QUERYSTRING = (
    #     "dataset=Tg_1_oper&"
    #     "SERVICE=WCS&"
    #     "REQUEST=GetCoverage&"
    #     "COVERAGE=daily_temperature/INTER_OPER_R___TAVGD___L3__0005_prediction&"
    #     "CRS=EPSG%3A28992&"
    #     "FORMAT=NetCDF3&"
    #     "BBOX=0,290000,300000,640000&"
    #     "RESX=10000&"
    #     "RESY=10000&"
    #     "TIME=2022-10-05T00:00:00Z"
    # )

    # Native grid:
    # QUERYSTRING = (
    #     "dataset=Tg_1_oper&"
    #     "SERVICE=WCS&"
    #     "REQUEST=GetCoverage&"
    #     "COVERAGE=daily_temperature/INTER_OPER_R___TAVGD___L3__0005_prediction&"
    #     "FORMAT=NetCDF3&"
    #     "TIME=2022-10-05T00:00:00Z"
    # )

    # WGS84 Lat/Lon projection
    QUERYSTRING = (
        "dataset=Tg_1_oper&"
        "SERVICE=WCS&"
        "REQUEST=GetCoverage&"
        "COVERAGE=daily_temperature/INTER_OPER_R___TAVGD___L3__0005_prediction&"
        "CRS=EPSG%3A4326&"
        "FORMAT=NetCDF3&"
        "BBOX=3.039095,50.580161,7.584775,53.746892&"
        "RESX=0.25&"
        "RESY=0.25&"
        "TIME=2022-10-05T00:00:00Z"
    )

    # QUERYSTRING = (
    #     "source=DEMO%2FWINS50%2FWINS50_43h21_fERA5_CTL_ptA_NETHERLANDS.NL_20190101.nc&"
    #     "SERVICE=WCS&"
    #     "REQUEST=GetCoverage&"
    #     "COVERAGE=ta&"
    #     "CRS=EPSG%3A4326&"
    #     "FORMAT=NetCDF4&"
    #     "BBOX=1.293392,50.065788,10.033752,55.667788&"
    #     "WIDTH=20&"
    #     "HEIGHT=20&"
    #     "TIME=2019-01-01T23:00:00Z&"
    #     "ELEVATION=600"
    # )
    QUERYSTRING = (
        "dataset=HARM_N25&"
        "SERVICE=WCS&"
        "REQUEST=GetCoverage&"
        "COVERAGE=air_temperature__at_2m&"
        "CRS=EPSG%3A4326&"
        "FORMAT=NetCDF3&"
        "BBOX=3.039095,50.580161,7.584775,53.746892&"
        # "RESX=0.25&"
        # "RESY=0.25&"
        "TIME=2023-03-23T09:00:00Z"
    )

    WCSGETCOVERAGEURL = SERVICE + QUERYSTRING

    # Get a NetCDF file as dataset
    response = requests.get(WCSGETCOVERAGEURL, timeout=60)

    print(response.status_code)

    ds = netCDF4.Dataset("filename.nc", memory=response.content)

    coveragejson = netcdf_to_covjson(ds, {})

    print(
        coveragejson.model_dump_json(
            exclude_none=True,
        )
    )
