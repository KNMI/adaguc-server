import json
from pathlib import Path

import netCDF4
import numpy as np
from numpy import ma
from numpy.typing import ArrayLike
import pytest

from routers.netcdf_to_covjson import coverage_values, netcdf_to_covjson


@pytest.mark.parametrize(
    "values, expected",
    [
        (np.array([[1, 2], [3, 4]]), [1, 2, 3, 4]),
        (ma.array([[1, np.nan], [np.inf, 4]], mask=[[True, False], [False, False]]), [None, None, None, 4]),
        (ma.array([1, 2], mask=True), [None, None]),
    ],
)
def test_coverage_values(values: ArrayLike, expected: list[float | None]) -> None:
    assert coverage_values(values) == expected


def test_netcdf_conversion_after_dataset_closes() -> None:
    path = Path(__file__).resolve().parents[2] / "data/datasets/testdata.nc"
    with netCDF4.Dataset(path) as dataset:
        variable = dataset.variables["testdata"]
        expected_values = variable[:].ravel().tolist()
        expected_shape = list(variable.shape)
        metadata = {"testdata": {"dims": {}, "layer": {"variables": [{"standard_name": variable.standard_name}]}}}
        coverages = netcdf_to_covjson(metadata, dataset, {}, {})

    # Serialization must work after the NetCDF dataset is closed.
    result = json.loads(coverages[0].model_dump_json())["ranges"]["testdata"]
    assert result["values"] == expected_values
    assert result["shape"] == expected_shape
