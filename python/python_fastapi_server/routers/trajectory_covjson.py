"""CoverageJSON Section encoding for native trajectory samples.

covjson-pydantic 0.5 supports tuple axes but does not yet define Section.
Keep the additional domain model local to this endpoint.
"""

import math
from collections import defaultdict
from typing import Literal

from covjson_pydantic.base_models import CovJsonBaseModel
from covjson_pydantic.coverage import Coverage, CoverageCollection
from covjson_pydantic.domain import ValuesAxis
from covjson_pydantic.ndarray import NdArrayFloat
from covjson_pydantic.observed_property import ObservedProperty
from covjson_pydantic.parameter import Parameter
from covjson_pydantic.reference_system import ReferenceSystem, ReferenceSystemConnectionObject
from covjson_pydantic.unit import Symbol, Unit

from .utils.edr_exception import EdrException
from .utils.edr_utils import SYMBOL_TYPE_URL, get_param_metadata, numeric_vertical_coordinate, vertical_dimension


class SectionAxes(CovJsonBaseModel):
    z: ValuesAxis[float]
    composite: ValuesAxis[tuple[str, float, float]]


class SectionDomain(CovJsonBaseModel, extra="allow"):
    type: Literal["Domain"] = "Domain"
    domainType: Literal["Section"] = "Section"
    axes: SectionAxes
    referencing: list[ReferenceSystemConnectionObject]


class SectionCoverage(Coverage):
    domain: SectionDomain


class SectionCoverageCollection(CoverageCollection):
    coverages: list[SectionCoverage]


def leaves(data, dimensions, keys=()):
    if not dimensions:
        yield keys, data
        return
    for key, value in data.items():
        yield from leaves(value, dimensions[1:], (*keys, key))


def numeric_value(value):
    if value is None or value == "nodata":
        return None
    try:
        number = float(value)
    except (TypeError, ValueError) as exc:
        raise EdrException(400, "Trajectory currently supports numeric parameters") from exc
    return number if math.isfinite(number) else None


def custom_value(value):
    try:
        return float(value)
    except ValueError:
        return value


def sections_from_response(data, metadata, collection_name):
    by_layer = defaultdict(list)
    for sample in data:
        by_layer[sample["layername"]].append(sample)

    coverages = []
    for layer_name, samples in by_layer.items():
        layer = metadata[layer_name]
        vertical = vertical_dimension(layer)
        groups = {}
        coords = [tuple(float(v) for v in sample["point"]["coords"].split(",")) for sample in samples]
        for sample_index, sample in enumerate(samples):
            dims = sample["dims"]
            dims = [dims] if isinstance(dims, str) else dims
            if vertical not in dims or "time" not in dims:
                raise EdrException(400, f"Section requires a selected time and vertical coordinate for {layer_name}")
            other_dims = [dim for dim in dims if dim not in (vertical, "time")]
            for keys, value in leaves(sample["data"], dims):
                selected = dict(zip(dims, keys))
                key = tuple((dim, selected[dim]) for dim in other_dims)
                group = groups.setdefault(key, {"times": set(), "levels": set(), "values": {}})
                level = numeric_vertical_coordinate(selected[vertical])
                time = selected["time"]
                group["times"].add(time)
                group["levels"].add(level)
                group["values"][level, time, sample_index] = numeric_value(value)

        md = get_param_metadata(layer, collection_name.rsplit(".", 1)[0])
        parameter = Parameter(
            id=layer_name,
            label={"en": md["parameter_label"]},
            observedProperty=ObservedProperty(id=md["observed_property_id"], label={"en": md["observed_property_label"]}),
            unit=Unit(symbol=Symbol(value=md["parameter_unit"], type=SYMBOL_TYPE_URL)),
        )
        referencing = [
            ReferenceSystemConnectionObject(
                coordinates=["x", "y"],
                system=ReferenceSystem(type="GeographicCRS", id="http://www.opengis.net/def/crs/OGC/1.3/CRS84"),
            ),
            ReferenceSystemConnectionObject(coordinates=["t"], system=ReferenceSystem(type="TemporalRS", calendar="Gregorian")),
            ReferenceSystemConnectionObject(
                coordinates=["z"], system=ReferenceSystem(type="VerticalCRS", description={"en": layer["dims"][vertical]["serviceName"]})
            ),
        ]
        for custom_dims, group in groups.items():
            times, levels = sorted(group["times"]), sorted(group["levels"])
            composite = [(time, lon, lat) for time in times for lon, lat in coords]
            values = [group["values"].get((level, time, index)) for level in levels for time in times for index in range(len(coords))]
            custom = {
                f"custom:{dim}": custom_value(value) for dim, value in custom_dims if not layer["dims"].get(dim, {}).get("hidden", False)
            }
            coverages.append(
                SectionCoverage(
                    domain=SectionDomain(
                        axes=SectionAxes(
                            z=ValuesAxis[float](values=levels),
                            composite=ValuesAxis[tuple[str, float, float]](dataType="tuple", coordinates=["t", "x", "y"], values=composite),
                        ),
                        referencing=referencing,
                        **custom,
                    ),
                    parameters={layer_name: parameter},
                    ranges={layer_name: NdArrayFloat(axisNames=["z", "composite"], shape=[len(levels), len(composite)], values=values)},
                )
            )
    if not coverages:
        raise EdrException(404, "No data along trajectory")
    if len(coverages) == 1:
        return coverages[0]
    return SectionCoverageCollection(
        coverages=coverages, parameters={name: p for coverage in coverages for name, p in coverage.parameters.items()}
    )
