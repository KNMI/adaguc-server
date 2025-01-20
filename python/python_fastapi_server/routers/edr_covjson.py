"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import itertools
import math
from datetime import datetime, timezone
from copy import deepcopy

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


def try_numeric(values):
    try:
        new_values = [float(i) for i in values]
        return new_values
    except ValueError:
        return values


def covjson_from_resp(dats, metadata):
    """
    Returns a coverage json from a Adaguc WMS GetFeatureInfo request
    """
    covjson_list = []
    dataset_name = list(metadata.keys())[0].rsplit(".")[0]
    for dat in dats:
        if len(dat["data"]):
            (lon, lat) = dat["point"]["coords"].split(",")
            custom_name = None
            vertical_name = None
            for param_dim in metadata[dat["name"]]["dims"].values():
                if param_dim["cdfName"] not in ["x", "y", "reference_time", "time"]:
                    if not param_dim["hidden"]:
                        if "isvertical" in param_dim and param_dim["isvertical"]:
                            # vertical_cdf_name = param_dim["cdfName"]
                            vertical_name = param_dim["serviceName"]
                        elif "iscustom" in param_dim and param_dim["iscustom"]:
                            # custom_cdf_name = param_dim["cdfName"]
                            custom_name = param_dim["serviceName"]
            lat = float(lat)
            lon = float(lon)
            dims = makedims(dat["dims"], dat["data"])
            time_steps = getdimvals(dims, "time")
            if vertical_name is not None:
                vertical_steps = getdimvals(dims, vertical_name)
            else:
                vertical_steps = getdimvals(dims, "elevation")

            custom_dim_values = []
            if custom_name is not None and len(custom_name) > 0:
                custom_dim_values = getdimvals(dims, custom_name)
                custom_dim_values = try_numeric(custom_dim_values)

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
            param_metadata = get_param_metadata(metadata[dat["name"]], dataset_name)
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
            axis_names = ["t"]
            shape = [len(time_steps)]
            if vertical_steps:
                axis_names = ["z", "t"]
                shape = [len(vertical_steps), len(time_steps)]

            _range = {
                "dataType": "float",
                "axisNames": axis_names,
                "shape": shape,
                "values": values,
            }
            ranges[dat["name"]] = _range

            axes: dict[str, ValuesAxis] = {
                "x": ValuesAxis[float](values=[lon]),
                "y": ValuesAxis[float](values=[lat]),
            }
            domain_type = "Point"
            separate_timesteps = False
            if vertical_steps:
                axes["z"] = ValuesAxis[float](values=vertical_steps)
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
                    domain_type = "VerticalProfile"
                    separate_timesteps = True
                elif (
                    len(time_steps) == 1 and vertical_steps and len(vertical_steps) > 1
                ):
                    domain_type = "VerticalProfile"
                elif (
                    len(time_steps) == 1 and vertical_steps and len(vertical_steps) == 1
                ):
                    domain_type = "Point"
                elif len(time_steps) > 1:
                    domain_type = "PointSeries"

            # if len(custom_dim_values) > 1:  # TODO 1 or 0?
            #     axes[custom_name] = ValuesAxis[str](values=custom_dim_values)
            #     if vertical_steps and len(vertical_steps) > 1:
            #         domain_type = "Grid"

            referencing = [
                (
                    ReferenceSystemConnectionObject(
                        system=ReferenceSystem(
                            type="GeographicCRS",
                            id="http://www.opengis.net/def/crs/OGC/1.3/CRS84",
                        ),
                        coordinates=["x", "y", "z"],
                    )
                    if vertical_steps and len(vertical_steps) > 0
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

            if separate_timesteps:
                index = 0
                for time_step in time_steps:
                    time_step_axes: dict[str, ValuesAxis] = {
                        "x": ValuesAxis[float](values=[lon]),
                        "y": ValuesAxis[float](values=[lat]),
                        "z": axes["z"] if vertical_steps else None,
                        "t": ValuesAxis[AwareDatetime](
                            values=[
                                datetime.strptime(
                                    time_step, "%Y-%m-%dT%H:%M:%SZ"
                                ).replace(tzinfo=timezone.utc)
                            ]
                        ),
                    }
                    time_step_domain = Domain(
                        domainType=domain_type,
                        axes=time_step_axes,
                        referencing=referencing,
                    )
                    step_ranges = deepcopy(ranges)
                    step_ranges[dat["name"]]["values"] = ranges[dat["name"]]["values"][
                        index :: len(time_steps)
                    ]
                    step_ranges[dat["name"]]["shape"][1] = 1

                    covjson = Coverage(
                        id=f"coverage_{(len(covjson_list)+1)}-{time_step}",
                        domain=time_step_domain,
                        ranges=step_ranges,
                        parameters=parameters,
                    )
                    covjson_list.append(covjson)
                    index += 1
            else:
                if len(custom_dim_values) > 0:
                    step_len = math.prod(shape)
                    for custom_dim_value_idx, custom_dim_value in enumerate(
                        custom_dim_values
                    ):
                        custom = {f"custom:{custom_name}": custom_dim_value}
                        domain = Domain(
                            domainType=domain_type,
                            axes=axes,
                            referencing=referencing,
                            **custom,
                        )
                        partial_ranges = {
                            dat["name"]: {
                                "values": ranges[dat["name"]]["values"][
                                    custom_dim_value_idx
                                    * step_len : (custom_dim_value_idx + 1)
                                    * step_len
                                ],
                                "dataType": "float",
                                "axisNames": axis_names,
                                "shape": shape,
                            }
                        }
                        covjson = Coverage(
                            id=f"coverage_{(len(covjson_list)+1)}",
                            domain=domain,
                            ranges=partial_ranges,
                            parameters=parameters,
                        )
                        covjson_list.append(covjson)
                else:
                    domain = Domain(
                        domainType=domain_type,
                        axes=axes,
                        referencing=referencing,
                    )
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
