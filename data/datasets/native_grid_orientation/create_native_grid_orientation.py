"""
Writes the test files for EDR cube requests with crs=native on grids stored in every orientation: x and y ascending or
descending. They have the rotated pole grid of test_rotated_pole_RACMO.nc. Every cell holds the number of its cell in
that grid, row * 100 + column (row 0 is the most southern, column 0 the most western), whatever the order of storage, so
a returned value tells exactly which cell it came from.

Usage (writes next to this script): python3 create_native_grid_orientation.py
"""

import os

import netCDF4
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = os.path.join(HERE, "..", "test_rotated_pole_RACMO.nc")


def write(name: str, x_ascending: bool, y_ascending: bool):
    with netCDF4.Dataset(SOURCE) as source, netCDF4.Dataset(os.path.join(HERE, name), "w") as target:
        x = source.variables["x"][:]  # Ascending in the source file
        y = source.variables["y"][:]
        rows, columns = np.meshgrid(np.arange(len(y)), np.arange(len(x)), indexing="ij")
        cells = (rows * 100 + columns).astype("f4")
        if not x_ascending:
            x, cells = x[::-1], cells[:, ::-1]
        if not y_ascending:
            y, cells = y[::-1], cells[::-1, :]
        target.createDimension("x", len(x))
        target.createDimension("y", len(y))
        target.createDimension("time", 1)
        for name_, values in (("x", x), ("y", y)):
            variable = target.createVariable(name_, "f8", (name_,))
            variable.standard_name = f"projection_{name_}_coordinate"
            variable[:] = values
        time = target.createVariable("time", "f8", ("time",))
        time.units = source.variables["time"].units
        time.standard_name = "time"
        time[:] = source.variables["time"][:]
        crs = target.createVariable("crs", "S1")
        crs.grid_mapping_name = "rotated_latitude_longitude"
        crs.grid_north_pole_latitude = source.variables["crs"].grid_north_pole_latitude
        crs.grid_north_pole_longitude = source.variables["crs"].grid_north_pole_longitude
        cell = target.createVariable("cell", "f4", ("time", "y", "x"))
        cell.long_name = "row * 100 + column of the cell in the grid"
        cell.units = "1"
        cell.grid_mapping = "crs"
        cell[0] = cells


if __name__ == "__main__":
    for x_ascending in (True, False):
        for y_ascending in (True, False):
            write(f"x{'asc' if x_ascending else 'desc'}_y{'asc' if y_ascending else 'desc'}.nc", x_ascending, y_ascending)
