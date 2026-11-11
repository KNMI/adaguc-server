"""Database-independent conversion and resource-lifetime regression tests.

Run with PYTHONPATH=python/python_fastapi_server:python/lib
python -m pytest --noconftest python/python_fastapi_server/test_edr_conversion.py
"""

import json
from collections.abc import Iterator

import netCDF4
import numpy as np
from numpy import ma
import pytest
from numpy.typing import ArrayLike
from routers.edr_covjson import point_response_to_covjson
from routers.netcdf_to_covjson import coverage_values, netcdf_to_covjson


@pytest.mark.parametrize(
    "values, expected",
    [
        (np.array([[1, 2], [3, 4]]), [1, 2, 3, 4]),
        (ma.array(np.arange(16), mask=[True] + [False] * 15), [None, *range(1, 16)]),
        (ma.array([[1, np.nan], [np.inf, 4]], mask=[[True, False], [False, False]]), [None, None, None, 4]),
        (ma.array([1, 2], mask=True), [None, None]),
        (np.array([]), []),
    ],
)
def test_coverage_values(values: ArrayLike, expected: list[float | None]) -> None:
    assert coverage_values(values) == expected


@pytest.fixture
def cube_payload() -> Iterator[tuple[netCDF4.Dataset, dict]]:
    dataset = netCDF4.Dataset("fixture", mode="w", diskless=True)
    try:
        for name, size in (("time", 2), ("y", 2), ("x", 3)):
            dataset.createDimension(name, size)
            axis = dataset.createVariable(name, "f8", (name,))
            axis[:] = np.arange(size)
            if name == "time":
                axis.units = "hours since 2024-01-01 00:00:00"
        variable = dataset.createVariable("temperature", "f4", ("time", "y", "x"), fill_value=-9999)
        variable[:] = np.arange(12).reshape(2, 2, 3)
        variable[0, 0, 0] = -9999
        variable[0, 0, 1] = np.nan
        variable.grid_mapping = "crs"
        variable.unit = "K"
        variable.long_name = "Temperature"
        metadata = {
            "temperature": {
                "dims": {name: {"cdfName": name, "serviceName": name} for name in ("time", "y", "x")},
                "layer": {"variables": [{"standard_name": "air_temperature"}]},
            }
        }
        yield dataset, metadata
    finally:
        if dataset.isopen():
            dataset.close()


def test_coverage_is_serializable_after_dataset_closes(cube_payload: tuple[netCDF4.Dataset, dict]) -> None:
    dataset, metadata = cube_payload
    coverage = netcdf_to_covjson(metadata, dataset, {}, {})
    dataset.close()
    result = json.loads(coverage[0].model_dump_json())
    assert result["ranges"]["temperature"]["values"] == [None, None, *range(2, 12)]


def test_custom_dimension_preserves_slices() -> None:
    dataset = netCDF4.Dataset("custom", mode="w", diskless=True)
    try:
        for name, size in (("member", 3), ("time", 1), ("y", 2), ("x", 2)):
            dataset.createDimension(name, size)
            axis = dataset.createVariable(name, "f8", (name,))
            axis[:] = np.arange(size)
            if name == "time":
                axis.units = "hours since 2024-01-01"
        variable = dataset.createVariable("v", "f4", ("member", "time", "y", "x"), fill_value=-9999)
        values = np.arange(12, dtype="f4").reshape(3, 1, 2, 2)
        values[1, 0, 0, 0] = -9999
        values[2, 0, 1, 0] = np.inf
        variable[:] = values
        variable.grid_mapping = "crs"
        variable.unit = "K"
        variable.long_name = "Value"
        dims = {name: {"cdfName": name, "serviceName": name, "type": "custom"} for name in ("member", "time", "y", "x")}

        result = netcdf_to_covjson({"v": {"dims": dims}}, dataset, {}, {})
        assert len(result) == 3
        for coverage, expected in zip(result, ([0, 1, 2, 3], [None, 5, 6, 7], [8, 9, None, 11])):
            assert coverage.ranges["v"].values == expected
            assert coverage.ranges["v"].shape == [1, 2, 2]
    finally:
        dataset.close()


@pytest.fixture
def point_layer() -> dict:
    return {
        "dims": {"time": {"cdfName": "time"}},
        "layer": {
            "layername": "temperature",
            "title": "Temperature",
            "abstract": "",
            "variables": [{"units": "K", "label": "Temperature", "standard_name": "air_temperature"}],
        },
    }


@pytest.mark.parametrize("members", [False, True])
def test_point_response_time_series_and_members(point_layer: dict, members: bool) -> None:
    times = ["2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"]
    data = dict(zip(times, ["1", "2"]))
    if members:
        point_layer["dims"]["member"] = {
            "cdfName": "member", "serviceName": "member", "hidden": False, "type": "dimtype_custom"
        }
        data = {"1": data, "2": dict(zip(times, ["nodata", "4"]))}
    entry = {
        "layername": "temperature",
        "point": {"coords": "5,52"},
        "dims": ["member", "time"] if members else "time",
        "data": data,
    }
    result = point_response_to_covjson([entry], {"temperature": point_layer}, "dataset.collection")
    coverages = result.coverages if members else [result]
    assert len(coverages) == (2 if members else 1)
    for index, coverage in enumerate(coverages):
        assert coverage.domain.domainType == "PointSeries"
        assert coverage.ranges["temperature"].values == ([1, 2] if index == 0 else [None, 4])
        if members:
            assert coverage.domain.model_dump()["custom:member"] == index + 1


def test_point_response_splits_profiles_by_time(point_layer: dict) -> None:
    point_layer["dims"]["level"] = {
        "cdfName": "level", "serviceName": "level", "hidden": False, "type": "dimtype_vertical"
    }
    times = ["2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"]
    entry = {
        "layername": "temperature",
        "point": {"coords": "5,52"},
        "dims": ["level", "time"],
        "data": {"850": dict(zip(times, ["1", "2"])), "500": dict(zip(times, ["3", "nodata"]))},
    }
    result = point_response_to_covjson([entry], {"temperature": point_layer}, "dataset.collection")
    assert len(result.coverages) == 2
    for coverage, expected in zip(result.coverages, ([1, 3], [2, None])):
        assert coverage.domain.domainType == "VerticalProfile"
        assert coverage.domain.axes.z.values == [850, 500]
        assert len(coverage.domain.axes.t.values) == 1
        assert coverage.ranges["temperature"].shape == [2, 1]
        assert coverage.ranges["temperature"].values == expected
