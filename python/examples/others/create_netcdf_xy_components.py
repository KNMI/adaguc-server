"""
File to generate a file with x and y components
Purpose: Test wind barb visualisation in adaguc-server
"""

import os.path
import netCDF4
import numpy as np

XDIMNAME = "x"
YDIMNAME = "y"
TIMEDIMNAME = "time"


def main():
    """Main routine"""
    netcdf_file = netCDF4.Dataset(os.path.join(
        "./data/datasets/netcdf_wind_components/xycomponents_all_north.nc"),
                                  mode="w",
                                  format="NETCDF4_CLASSIC")

    # Create the dimensions.
    netcdf_file.createDimension(XDIMNAME, 75)
    netcdf_file.createDimension(YDIMNAME, 80)
    netcdf_file.createDimension(TIMEDIMNAME, 1)
    np_array = np.empty((1, 75, 80))

    lon_var = netcdf_file.createVariable("x", "f8", (XDIMNAME))
    lon_var.standard_name = "x coordinate of projection"
    lon_var.long_name = "projection_x_coordinate"
    lon_var.units = "km"

    # Create the coordinate variables.
    lat_var = netcdf_file.createVariable("y", "f8", (YDIMNAME))
    lat_var.standard_name = "y coordinate of projection"
    lat_var.long_name = "projection_y_coordinate"
    lat_var.units = "km"

    time_var = netcdf_file.createVariable("time", "f8", (TIMEDIMNAME))
    time_var.standard_name = "time"
    time_var.long_name = "time"
    time_var.units = "seconds since 1970-01-01 00:00:00"

    # Create the measured value variables.
    xcomponent = netcdf_file.createVariable("xcomponent",
                                            "f8",
                                            (TIMEDIMNAME, XDIMNAME, YDIMNAME),
                                            complevel=2,
                                            compression='zlib')
    xcomponent.standard_name = "xcomponent"
    xcomponent.long_name = "xcomponent"
    xcomponent.units = "kts"
    xcomponent.grid_mapping = "projection"

    ycomponent = netcdf_file.createVariable("ycomponent",
                                            "f8",
                                            (TIMEDIMNAME, XDIMNAME, YDIMNAME),
                                            complevel=2,
                                            compression='zlib')
    ycomponent.standard_name = "ycomponent"
    ycomponent.long_name = "ycomponent"
    ycomponent.units = "kts"
    ycomponent.grid_mapping = "projection"

    project_var = netcdf_file.createVariable("projection", "c")
    project_var.EPSG_code = "none"
    project_var.grid_mapping_name = "polar_stereographic"
    project_var.latitude_of_projection_origin = 90.
    project_var.straight_vertical_longitude_from_pole = 0.
    project_var.scale_factor_at_projection_origin = 0.933012709177451
    project_var.false_easting = 0.
    project_var.false_northing = 0.
    project_var.semi_major_axis = 6378140.
    project_var.semi_minor_axis = 6356750.
    project_var.proj4_params = "+proj=stere +lat_0=90 +lon_0=0 +lat_ts=60 +a=6378.14 +b=6356.75 +x_0=0 y_0=0"
    project_var.long_name = "projection"

    print("NetCDF file initialized.")

    # Fill the data variables.
    xcomponent[:] = np.full(shape=np_array.shape, fill_value=0)
    ycomponent[:] = np.full(shape=np_array.shape, fill_value=-65)
    lon_var[:] = np.arange(-15000, 15000, 400)
    lat_var[:] = np.arange(-30, -8000, -100)
    time_var[:] = [1508052712]

    netcdf_file.close()
    print("Done.")


if __name__ == "__main__":
    main()
