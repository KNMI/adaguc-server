#!/usr/bin/python

#***********************************************************************
#*                      All rights reserved                           **
#*                     Copyright (c) 2018 KNMI                        **
#*             Royal Netherlands Meteorological Institute             **
#***********************************************************************
#* Function   : TODO
#* Purpose    : TODO
#* Usage      : TODO
#*
#* Project    : EUNADICS-AV
#*
#* initial programmer :  S.E. Wagenaar
#* initial date       :  20180206
#**********************************************************************

import netCDF4
import argparse
import os.path
import numpy

x_dim_name = "x"
y_dim_name = "y"
time_dim_name = "time"

def main():

    netcdf_file = netCDF4.Dataset(os.path.join("grid_format_proj_example_1.nc"), mode="w", format="NETCDF4_CLASSIC")

    # Create the dimensions.
    netcdf_file.createDimension(x_dim_name, 4)
    netcdf_file.createDimension(y_dim_name, 3)
    netcdf_file.createDimension(time_dim_name, 2)\

    lon_var = netcdf_file.createVariable("x", "f8", (x_dim_name))
    lon_var.standard_name = "x coordinate of projection"
    lon_var.long_name = "projection_x_coordinate"
    lon_var.units = "km"

    # Create the coordinate variables.
    lat_var = netcdf_file.createVariable("y", "f8", (y_dim_name))
    lat_var.standard_name = "y coordinate of projection"
    lat_var.long_name = "projection_y_coordinate"
    lat_var.units = "km"

    time_var = netcdf_file.createVariable("time", "f8", (time_dim_name))
    time_var.standard_name = "time"
    time_var.long_name = "time"
    time_var.units = "seconds since 1970-01-01 00:00:00"
    time_var.valid_range = 0,2170706400 ## Valid range 1970-01-01 00:00:00.000 to 2038-01-01T00:00:00.000

    # Create the measured value variables.
    data_var_1 = netcdf_file.createVariable("data_var_1", "f8", (time_dim_name, x_dim_name, y_dim_name))
    data_var_1.standard_name = "data_var_1"
    data_var_1.long_name = "data_var_1"
    data_var_1.units = "none"
    data_var_1.grid_mapping = "projection"

    project_var = netcdf_file.createVariable("projection", "c")
    project_var.EPSG_code = "none" ;
    project_var.grid_mapping_name = "polar_stereographic" ;
    project_var.latitude_of_projection_origin = 90. ;
    project_var.straight_vertical_longitude_from_pole = 0. ;
    project_var.scale_factor_at_projection_origin = 0.933012709177451 ;
    project_var.false_easting = 0. ;
    project_var.false_northing = 0. ;
    project_var.semi_major_axis = 6378140. ;
    project_var.semi_minor_axis = 6356750. ;
    project_var.proj4_params = "+proj=stere +lat_0=90 +lon_0=0 +lat_ts=60 +a=6378.14 +b=6356.75 +x_0=0 y_0=0" ;
    project_var.long_name = "projection" ;


    print ("NetCDF file initialized.")

    # Fill the data variables.
    data_var_1[:] = [[[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]],
                     [[11, 12, 13, 14], [15, 16, 17, 18], [19, 20 , 21, 22]]]
    lon_var[:] = [340, 370, 400, 430]
    lat_var[:] = [-3950, -3980, -4000]
    time_var[:] = [1508052712, 1508052713]
   
    netcdf_file.close()
    print("Done.")

if __name__ == "__main__":
    main()
