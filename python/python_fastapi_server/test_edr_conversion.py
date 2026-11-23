import json
from pathlib import Path

import netCDF4
import numpy as np
from numpy import ma
from numpy.typing import ArrayLike
import pytest

from routers.netcdf_to_covjson import coverage_values, netcdf_to_covjson, round_to_data_precision


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


def test_values_have_the_precision_of_the_data() -> None:
    """Values are written with the digits the data has, not with the noise of how they are stored"""
    # Packed data unpacked to 64 bit: 38 * 0.1 is 3.8000000000000003
    packed = np.array([38, 0, -694, 1013], dtype=np.int16) * 0.1
    assert json.dumps(coverage_values(packed)) == "[3.8, 0.0, -69.4, 101.3]"

    # 32 bit floats: the fewest digits that still give the same 32 bit float, instead of 17
    floats = np.array([-21.036102294921875, 6.324615478515625, 3.8, 1.0e-7, 1013.25, 0.1, 3.4e38, 1.5e-40, 16777217.0], dtype=np.float32)
    written = coverage_values(floats)
    assert json.dumps(written[:3]) == "[-21.036102, 6.3246155, 3.8]"
    assert np.array_equal(np.array(written, dtype=np.float32), floats)
    random_floats = np.random.default_rng(1).normal(0, 1e3, 5000).astype(np.float32)
    assert np.array_equal(np.array(coverage_values(random_floats), dtype=np.float32), random_floats)

    # 64 bit floats keep 12 significant digits, whatever their size
    doubles = np.array([123456.789012345, 1.23456789012345e-12, -9.87654321098765e20, 0.0, 5e-324, 1.7e308])
    rounded = round_to_data_precision(doubles)
    assert json.dumps(rounded[:3].tolist()) == "[123456.789012, 1.23456789012e-12, -9.87654321099e+20]"
    assert np.allclose(rounded, doubles, rtol=1e-11, atol=0)
    # Too small or large for a power of ten: as they are
    assert rounded[4] == 5e-324 and rounded[5] == 1.7e308

    # Whole numbers are exact, also with more than 12 digits, like a count, an identifier or a time in milliseconds
    whole = np.array([1234567890123.0, 1716854400123.0, -98765432109876.0, 999999999999999.0, 38.0, -0.0, np.nan, np.inf])
    assert np.array_equal(round_to_data_precision(whole), whole, equal_nan=True)
    assert json.dumps(coverage_values(whole[:3])) == "[1234567890123.0, 1716854400123.0, -98765432109876.0]"
    # Other 64 bit values with more than 12 significant digits lose the digits after the 12th
    assert round_to_data_precision(np.array([0.1234567890123456, 1234567890123.5])).tolist() == [0.123456789012, 1234567890120.0]

    # Integers stay integers
    assert coverage_values(np.array([1, 2, 3], dtype=np.int32)) == [1, 2, 3]


def test_netcdf_conversion_after_dataset_closes() -> None:
    path = Path(__file__).resolve().parents[2] / "data/datasets/testdata.nc"
    with netCDF4.Dataset(path) as dataset:
        variable = dataset.variables["testdata"]
        expected_values = variable[:].ravel()
        expected_shape = list(variable.shape)
        metadata = {"testdata": {"dims": {}, "layer": {"variables": [{"standard_name": variable.standard_name}]}}}
        coverages = netcdf_to_covjson(metadata, dataset, {}, {})

    # Serialization must work after the NetCDF dataset is closed.
    result = json.loads(coverages[0].model_dump_json())["ranges"]["testdata"]
    # The values are written with the digits that give the same 32 bit floats
    assert expected_values.dtype == np.float32
    assert np.array_equal(np.array(result["values"], dtype=np.float32), expected_values)
    assert result["shape"] == expected_shape
