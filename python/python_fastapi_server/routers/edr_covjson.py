"""CoverageJSON conversion for native ADAGUC GetPointValue responses.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import itertools
from collections import Counter
import math
from datetime import datetime, timezone
from typing import Any

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

from .utils.edr_exception import EdrException
from .utils.edr_utils import DATETIME_ISO8601_FMT, SYMBOL_TYPE_URL, VOCAB_ENDPOINT_URL, get_param_metadata, numeric_vertical_coordinate


def point_coordinate_axes(
    lon: float,
    lat: float,
    vertical_steps: list[float] | None,
    time_steps: list[str] | None,
) -> dict[str, ValuesAxis[float] | ValuesAxis[AwareDatetime]]:
    """Build point coordinates with optional vertical and UTC time axes."""
    axes: dict[str, ValuesAxis[float] | ValuesAxis[AwareDatetime]] = {
        "x": ValuesAxis[float](values=[lon]),
        "y": ValuesAxis[float](values=[lat]),
    }
    if vertical_steps:
        axes["z"] = ValuesAxis[float](values=vertical_steps)
    if time_steps:
        axes["t"] = ValuesAxis[AwareDatetime](
            values=[datetime.strptime(t, DATETIME_ISO8601_FMT).replace(tzinfo=timezone.utc) for t in time_steps]
        )
    return axes


def point_coverage_referencing(has_vertical: bool) -> list[ReferenceSystemConnectionObject]:
    """Build the point response's CRS84 and temporal references."""
    return [
        ReferenceSystemConnectionObject(
            system=ReferenceSystem(type="GeographicCRS", id="http://www.opengis.net/def/crs/OGC/1.3/CRS84"),
            coordinates=["x", "y", "z"] if has_vertical else ["x", "y"],
        ),
        ReferenceSystemConnectionObject(
            system=ReferenceSystem(type="TemporalRS", calendar="Gregorian"),
            coordinates=["t"],
        ),
    ]


def try_numeric(values):
    try:
        new_values = [float(i) for i in values]
        return new_values
    except ValueError:
        return values


def get_entry_param_metadata(dat: dict, param_metadata: dict) -> tuple[str, dict]:
    """
    Returns the parameter id and metadata for one GetFeatureInfo entry.

    A layer can return several entries, for example the `convert_uv_components`
    post-processor returns speed, direction and four wind components. The layer
    metadata only describes the first variable, so for those entries the id,
    unit and observed property are taken from the entry itself.
    """
    variable_name = dat.get("variablename", "")
    param_id = f"{dat['layername']}.{variable_name}"
    entry_metadata = dict(param_metadata)
    if dat.get("units"):
        entry_metadata["parameter_unit"] = dat["units"]
    standard_name = dat.get("standard_name", "")
    if standard_name:
        entry_metadata["observed_property_id"] = VOCAB_ENDPOINT_URL + standard_name
        entry_metadata["observed_property_label"] = standard_name.replace("_", " ").capitalize()
    entry_metadata["parameter_label"] = f"{param_metadata['parameter_label']} ({variable_name})"
    return param_id, entry_metadata


def point_response_to_covjson(dats: list[dict[str, Any]], metadata: dict[str, Any], collection_name: str) -> Coverage | CoverageCollection:
    """Convert decoded native GetPointValue JSON into point/profile CoverageJSON."""
    covjson_list = []
    dataset_name = collection_name.rsplit(".")[0]
    entries_per_layer = Counter(dat["layername"] for dat in dats)
    for dat in dats:
        layer_name = dat["layername"]
        if layer_name not in metadata:
            print("SKIPPING layername named ", layer_name)
            break
        if not dat["data"]:
            continue

        layer_metadata = metadata[layer_name]
        custom_name = None
        vertical_name = None
        reference_time_name = None
        for param_dim in layer_metadata["dims"].values():
            if param_dim["cdfName"] in ("x", "y", "time") or param_dim["hidden"]:
                continue
            if param_dim["type"] == "dimtype_vertical":
                vertical_name = param_dim["serviceName"]
            elif param_dim["type"] == "dimtype_custom" or param_dim["serviceName"] != "reference_time":
                custom_name = param_dim["serviceName"]
            else:
                reference_time_name = param_dim["serviceName"]

        lon, lat = dat["point"]["coords"].split(",")
        lat = float(lat)
        lon = float(lon)

        dims = makedims(dat["dims"], dat["data"])
        time_steps = dims.get("time")
        vertical_steps = dims.get(vertical_name if vertical_name is not None else "elevation")
        if vertical_steps:
            vertical_steps = [numeric_vertical_coordinate(level) for level in vertical_steps]

        custom_dim_values = []
        if custom_name:
            custom_dim_values = dims.get(custom_name)
            custom_dim_values = try_numeric(custom_dim_values)

        if reference_time_name:
            reference_time_value = dims.get(reference_time_name)[0]

        values = []
        for coordinates in itertools.product(*dims.values()):
            value = multi_get(dat["data"], coordinates)
            if not value:
                continue
            try:
                values.append(float(value))
            except ValueError:
                values.append(None if value == "nodata" else value)

        parameters: dict[str, CovJsonParameter] = {}
        ranges = {}
        param_metadata = get_param_metadata(layer_metadata, dataset_name)
        param_id = layer_name
        if entries_per_layer[layer_name] > 1:
            param_id, param_metadata = get_entry_param_metadata(dat, param_metadata)
        symbol = CovJsonSymbol(value=param_metadata["parameter_unit"], type=SYMBOL_TYPE_URL)
        unit = CovJsonUnit(symbol=symbol)
        observed_property = CovJsonObservedProperty(
            id=param_metadata["observed_property_id"],
            label={"en": param_metadata["observed_property_label"]},
        )

        param = CovJsonParameter(
            id=param_id,
            observedProperty=observed_property,
            # description={"en":param_metadata["wms_layer_title"]}, # TODO in follow up
            unit=unit,
            label={"en:": param_metadata["parameter_label"]},
        )

        parameters[param_id] = param
        axis_names = ["t"]
        shape = [len(time_steps)]
        if vertical_steps:
            axis_names = ["z", "t"]
            shape = [len(vertical_steps), len(time_steps)]

        ranges[param_id] = {"dataType": "float", "axisNames": axis_names, "shape": shape, "values": values}

        axes = point_coordinate_axes(lon, lat, vertical_steps, time_steps)
        domain_type = "Point"
        separate_timesteps = False

        if time_steps:
            if vertical_steps and len(vertical_steps) > 1:
                domain_type = "VerticalProfile"
                separate_timesteps = len(time_steps) > 1
            elif len(time_steps) > 1:
                domain_type = "PointSeries"

        referencing = point_coverage_referencing(bool(vertical_steps))

        if separate_timesteps:
            for index, time_step in enumerate(time_steps):
                time_step_axes = point_coordinate_axes(lon, lat, vertical_steps, [time_step])
                time_step_domain = Domain(
                    domainType=domain_type,
                    axes=time_step_axes,
                    referencing=referencing,
                )
                step_ranges = {
                    param_id: {
                        **ranges[param_id],
                        "values": ranges[param_id]["values"][index :: len(time_steps)],
                        "shape": [len(vertical_steps), 1],
                    }
                }

                covjson = Coverage(
                    id=f"coverage_{(len(covjson_list)+1)}-{time_step}",
                    domain=time_step_domain,
                    ranges=step_ranges,
                    parameters=parameters,
                )
                covjson_list.append(covjson)
            continue

        step_len = math.prod(shape)
        for index, custom_dim_value in enumerate(custom_dim_values or [None]):
            custom = {}
            step_ranges = ranges
            if custom_dim_values:
                custom = {f"custom:{custom_name}": custom_dim_value}
                if reference_time_name is not None:
                    custom[f"custom:{reference_time_name}"] = reference_time_value
                step_ranges = {
                    param_id: {
                        **ranges[param_id],
                        "values": values[index * step_len : (index + 1) * step_len],
                    }
                }
            domain = Domain(domainType=domain_type, axes=axes, referencing=referencing, **custom)
            covjson_list.append(Coverage(id=f"coverage_{len(covjson_list) + 1}", domain=domain, ranges=step_ranges, parameters=parameters))

    if len(covjson_list) == 1:
        return covjson_list[0]
    if not covjson_list:
        raise EdrException(code=400, description="No data")

    return CoverageCollection(coverages=covjson_list)


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
