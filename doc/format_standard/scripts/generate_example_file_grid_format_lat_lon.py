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

lon_dim_name = "lon"
lat_dim_name = "lat"
time_dim_name = "time"

def main():

    netcdf_file = netCDF4.Dataset(os.path.join("grid_format_lat_lon_example_1.nc"), mode="w", format="NETCDF4_CLASSIC")

    # Create the dimensions.
    netcdf_file.createDimension(lon_dim_name, 4)
    netcdf_file.createDimension(lat_dim_name, 3)
    netcdf_file.createDimension(time_dim_name, 2)\

    lon_var = netcdf_file.createVariable("lon", "f8", (lon_dim_name))
    lon_var.standard_name = "longitude"
    lon_var.long_name = "longitude"
    lon_var.units = "degrees_east"

    # Create the coordinate variables.
    lat_var = netcdf_file.createVariable("lat", "f8", (lat_dim_name))
    lat_var.standard_name = "latitude"
    lat_var.long_name = "latitude"
    lat_var.units = "degrees_north"

    time_var = netcdf_file.createVariable("time", "f8", (time_dim_name))
    time_var.standard_name = "time"
    time_var.long_name = "time"
    time_var.units = "seconds since 1970-01-01 00:00:00"
    time_var.valid_range = 0,2170706400 ## Valid range 1970-01-01 00:00:00.000 to 2038-01-01T00:00:00.000

    # Create the measured value variables.
    data_var_1 = netcdf_file.createVariable("data_var_1", "f8", (time_dim_name, lat_dim_name, lon_dim_name))
    data_var_1.standard_name = "data_var_1"
    data_var_1.long_name = "data_var_1"
    data_var_1.units = "none"

    print ("NetCDF file initialized.")

    # Fill the data variables.
    data_var_1[:] = [[[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]],
                     [[11, 12, 13, 14], [15, 16, 17, 18], [19, 20 , 21, 22]]]
    lon_var[:] = [5.05, 6.05, 7.05, 8.05]
    lat_var[:] = [53.5, 52.5, 51.5]
    time_var[:] = [1508052712, 1508052713]
   
    netcdf_file.close()
    print("Done.")

if __name__ == "__main__":
    main()
