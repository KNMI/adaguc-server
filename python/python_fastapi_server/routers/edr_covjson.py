"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import itertools
from datetime import datetime, timezone

from covjson_pydantic.coverage import Coverage, CoverageCollection
from covjson_pydantic.domain import Domain, ValuesAxis
from covjson_pydantic.observed_property import (
    ObservedProperty as CovJsonObservedProperty,
)
from covjson_pydantic.parameter import Parameter as CovJsonParameter
from covjson_pydantic.reference_system import (
    ReferenceSystem,
    ReferenceSystemConnectionObject,
)
from covjson_pydantic.unit import Symbol as CovJsonSymbol
from covjson_pydantic.unit import Unit as CovJsonUnit
from pydantic import AwareDatetime

from .edr_exception import EdrException
from .edr_utils import get_param_metadata

SYMBOL_TYPE_URL = "http://www.opengis.net/def/uom/UCUM"


def covjson_from_resp(dats, vertical_name, custom_name, collection_name):
    """
    Returns a coverage json from a Adaguc WMS GetFeatureInfo request
    """
    covjson_list = []
    for dat in dats:
        if len(dat["data"]):
            (lon, lat) = dat["point"]["coords"].split(",")
            lat = float(lat)
            lon = float(lon)
            dims = makedims(dat["dims"], dat["data"])
            time_steps = getdimvals(dims, "time")
            if vertical_name is not None:
                vertical_steps = getdimvals(dims, vertical_name)
            else:
                vertical_steps = getdimvals(dims, "elevation")

            custom_dim_values = []
            if custom_name is not None:
                custom_dim_values = getdimvals(dims, custom_name)

            valstack = []
            # Translate the adaguc GFI object in something we can handle in Python
            for dim_name in dims:
                vals = getdimvals(dims, dim_name)
                valstack.append(vals)
            tuples = list(itertools.product(*valstack))
            values = []
            for mytuple in tuples:
                value = multi_get(dat["data"], mytuple)
                if value:
                    try:
                        values.append(float(value))
                    except ValueError:
                        if value == "nodata":
                            values.append(None)
                        else:
                            values.append(value)

            parameters: dict[str, CovJsonParameter] = {}
            ranges = {}
            param_metadata = get_param_metadata(dat["name"], collection_name)
            symbol = CovJsonSymbol(
                value=param_metadata["parameter_unit"], type=SYMBOL_TYPE_URL
            )
            unit = CovJsonUnit(symbol=symbol)
            observed_property = CovJsonObservedProperty(
                id=param_metadata["observed_property_id"],
                label={"en": param_metadata["observed_property_label"]},
            )

            param = CovJsonParameter(
                id=dat["name"],
                observedProperty=observed_property,
                # description={"en":param_metadata["wms_layer_title"]}, # TODO in follow up
                unit=unit,
                label={"en:": param_metadata["parameter_label"]},
            )

            parameters[dat["name"]] = param
            axis_names = ["x", "y", "t"]
            shape = [1, 1, len(time_steps)]
            if vertical_steps:
                axis_names = ["x", "y", "z", "t"]
                shape = [1, 1, len(vertical_steps), len(time_steps)]
            if len(custom_dim_values) > 0:
                axis_names.insert(-1, custom_name)
                shape.insert(
                    -1,
                    len(custom_dim_values),
                )

            _range = {
                "axisNames": axis_names,
                "shape": shape,
                "values": values,
            }
            ranges[dat["name"]] = _range

            axes: dict[str, ValuesAxis] = {
                "x": ValuesAxis[float](values=[lon]),
                "y": ValuesAxis[float](values=[lat]),
                "t": ValuesAxis[AwareDatetime](
                    values=[
                        datetime.strptime(t, "%Y-%m-%dT%H:%M:%SZ").replace(
                            tzinfo=timezone.utc
                        )
                        for t in time_steps
                    ]
                ),
            }
            domain_type = "PointSeries"
            if vertical_steps:
                axes["z"] = ValuesAxis[float](values=vertical_steps)
                if len(vertical_steps) > 1:
                    domain_type = "VerticalProfile"
            if time_steps:
                axes["t"] = ValuesAxis[AwareDatetime](
                    values=[
                        datetime.strptime(t, "%Y-%m-%dT%H:%M:%SZ").replace(
                            tzinfo=timezone.utc
                        )
                        for t in time_steps
                    ]
                )
                if len(time_steps) > 1 and vertical_steps and len(vertical_steps) > 1:
                    domain_type = "Grid"
            if len(custom_dim_values) > 0:
                axes[custom_name] = ValuesAxis[str](values=custom_dim_values)
                if vertical_steps and len(vertical_steps) > 1:
                    domain_type = "Grid"

            referencing = [
                (
                    ReferenceSystemConnectionObject(
                        system=ReferenceSystem(
                            type="GeographicCRS",
                            id="http://www.opengis.net/def/crs/OGC/1.3/CRS84",
                        ),
                        coordinates=["x", "y", "z"],
                    )
                    if vertical_steps
                    else ReferenceSystemConnectionObject(
                        system=ReferenceSystem(
                            type="GeographicCRS",
                            id="http://www.opengis.net/def/crs/OGC/1.3/CRS84",
                        ),
                        coordinates=["x", "y"],
                    )
                ),
                ReferenceSystemConnectionObject(
                    system=ReferenceSystem(type="TemporalRS", calendar="Gregorian"),
                    coordinates=["t"],
                ),
            ]
            domain = Domain(domainType=domain_type, axes=axes, referencing=referencing)
            covjson = Coverage(
                id=f"coverage_{(len(covjson_list)+1)}",
                domain=domain,
                ranges=ranges,
                parameters=parameters,
            )
            covjson_list.append(covjson)

    if len(covjson_list) == 1:
        return covjson_list[0]
    if len(covjson_list) == 0:
        raise EdrException(code=400, description="No data")

    coverage_collection = CoverageCollection(coverages=covjson_list)

    return coverage_collection


def multi_get(dict_obj, attrs, default=None):
    """
    multi_get
    """
    result = dict_obj
    for attr in attrs:
        if attr not in result:
            return default
        result = result[attr]
    return result


def getdimvals(dims, name):
    """
    getdimvals
    """
    if name in dims:
        return dims[name]
    return None


def makedims(dims, data):
    """
    Makedims
    """
    dimlist = {}
    if isinstance(dims, str) and dims == "time":
        times = list(data.keys())
        dimlist["time"] = times
        return dimlist

    data1 = list(data.keys())
    if len(data1) == 0:
        return []
    dimlist[dims[0]] = data1

    if len(dims) >= 2:
        data2 = list(data[data1[0]].keys())
        dimlist[dims[1]] = data2

    if len(dims) >= 3:
        data3 = list(data[data1[0]][data2[0]].keys())
        dimlist[dims[2]] = data3

    if len(dims) >= 4:
        data4 = list(data[data1[0]][data2[0]][data3[0]].keys())
        dimlist[dims[3]] = data4

    if len(dims) >= 5:
        data5 = list(data[data1[0]][data2[0]][data3[0]][data4[0]].keys())
        dimlist[dims[4]] = data5

    return dimlist
