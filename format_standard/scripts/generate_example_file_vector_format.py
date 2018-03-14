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

nv_dim_name = "nv"
time_dim_name = "time"

def main():

    netcdf_file = netCDF4.Dataset(os.path.join("vector_format_example_1.nc"), mode="w", format="NETCDF4_CLASSIC")

    # Create the dimensions.
    netcdf_file.createDimension(nv_dim_name, 4) # Number of vertices of measured pixel
    netcdf_file.createDimension(time_dim_name, 2)

    netcdf_file.createVariable("product", "c") ## necessary for old versions of adaguc detection.
    
    # Create the coordinate variables.
    time_var = netcdf_file.createVariable("time", "f8", (time_dim_name))
    time_var.standard_name = "time"
    time_var.long_name = "time"
    time_var.units = "seconds since 1970-01-01 00:00:00"
    time_var.valid_range = 0,2170706400 ## Valid range 1970-01-01 00:00:00.000 to 2038-01-01T00:00:00.000

    lon_var = netcdf_file.createVariable("lon", "f8", (time_dim_name))
    lon_var.standard_name = "longitude"
    lon_var.long_name = "longitude"
    lon_var.units = "degrees_east"
    lon_var.bounds = "lon_bnds"

    lat_var = netcdf_file.createVariable("lat", "f8", (time_dim_name))
    lat_var.standard_name = "latitude"
    lat_var.long_name = "latitude"
    lat_var.units = "degrees_north"
    lat_var.bounds = "lat_bnds"

    lon_bnds_var = netcdf_file.createVariable("lon_bnds", "f4", (time_dim_name, nv_dim_name)) # No attributes necessary.
    lat_bnds_var = netcdf_file.createVariable("lat_bnds", "f4", (time_dim_name, nv_dim_name)) # No attributes necessary.

    # Create the measured value variables.
    data_var_1 = netcdf_file.createVariable("data_var_1", "f8", (time_dim_name))
    data_var_1.standard_name = "data_var_1"
    data_var_1.long_name = "data_var_1"
    data_var_1.units = "none"

    print ("NetCDF file initialized.")

    # Fill the data variables.
    data_var_1[:] = [1, 2]
    lon_var[:] = [5.05, 6.05]
    lat_var[:] = [53.5, 53.5]
    lon_bnds_var[:] = [[5.1, 5.1, 5, 5], [6.1, 6.1, 6, 6]]
    lat_bnds_var[:] = [[53, 54, 54, 53], [53, 54, 54, 53]]
    time_var[:] = [1508052712, 1508052713]
   
    netcdf_file.close()
    print("Done.")

if __name__ == "__main__":
    main()
