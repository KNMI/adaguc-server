#!/usr/bin/python

# ***********************************************************************
# *                      All rights reserved                           **
# *                     Copyright (c) 2024 KNMI                        **
# *             Royal Netherlands Meteorological Institute             **
# ***********************************************************************
# * Purpose   : This script will create a file where lon and lat variables have irregular spacing. These variables are still 1 dimensional.
# *
# * Project    : ADAGUC
# *
# * initial programmer :  Maarten Plieger
# * initial date       :  20240202
# **********************************************************************

import netCDF4
import numpy
import datetime
import os.path
import random

TIME_DIM_NAME = "time"
EXTRA_DIM_NAME = "extra"
LATITUDE_1D_DIM_NAME = "lat"
LONGITUDE_1D_DIM_NAME = "lon"
filename = os.path.join("example_file_irregular_1Dlat1Dlon_grid.nc")

"""

This script will create a file where lon and lat variables have irregular spacing. These variables are still 1 dimensional.

See the NetCDF structure below:

netcdf irreglatlongrid {
dimensions:
        time = 2 ;
        lat = 16 ;
        lon = 20 ;
        extra = 10 ;
variables:
        double time(time) ;
                time:standard_name = "time" ;
                time:long_name = "time" ;
                time:units = "seconds since 1970-01-01 00:00:00" ;
        double lon(lon) ;
                lon:standard_name = "longitude" ;
                lon:long_name = "longitude" ;
                lon:units = "degrees_east" ;
        double lat(lat) ;
                lat:standard_name = "latitude" ;
                lat:long_name = "latitude" ;
                lat:units = "degrees_north" ;
        double extra(extra) ;
                extra:standard_name = "extra" ;
                extra:long_name = "extra" ;
                extra:units = "extra" ;
        double data_var_1(extra, time, lat, lon) ;
                data_var_1:_FillValue = -9999. ;
                data_var_1:standard_name = "data_var_1" ;
                data_var_1:long_name = "data_var_1" ;
                data_var_1:units = "none" ;


"""


def main():
    netcdf_file = netCDF4.Dataset(filename, mode="w", format="NETCDF4")

    # Create the dimensions.
    netcdf_file.createDimension(TIME_DIM_NAME, 2)
    netcdf_file.createDimension(LATITUDE_1D_DIM_NAME, 16)
    netcdf_file.createDimension(LONGITUDE_1D_DIM_NAME, 20)
    netcdf_file.createDimension(EXTRA_DIM_NAME, 10)

    # Create the coordinate variables.
    time_var = netcdf_file.createVariable("time", "f8", (TIME_DIM_NAME))
    time_var.standard_name = "time"
    time_var.long_name = "time"
    time_var.units = "seconds since 1970-01-01 00:00:00"

    lon_var = netcdf_file.createVariable(
        "lon",
        "f8",
        (LONGITUDE_1D_DIM_NAME),
    )
    lon_var.standard_name = "longitude"
    lon_var.long_name = "longitude"
    lon_var.units = "degrees_east"

    lat_var = netcdf_file.createVariable(
        "lat",
        "f8",
        (LATITUDE_1D_DIM_NAME),
    )
    lat_var.standard_name = "latitude"
    lat_var.long_name = "latitude"
    lat_var.units = "degrees_north"

    # Create extra dim variable
    extra_var = netcdf_file.createVariable("extra", "f8", (EXTRA_DIM_NAME))
    extra_var.standard_name = "extra"
    extra_var.long_name = "extra"
    extra_var.units = "extra"

    extra_var[:] = numpy.arange(0, 10, 1) / 10.0

    # Create the measured value variables.
    data_var_1 = netcdf_file.createVariable(
        "data_var_1",
        "f4",
        (EXTRA_DIM_NAME, TIME_DIM_NAME, LATITUDE_1D_DIM_NAME, LONGITUDE_1D_DIM_NAME),
        fill_value=-9999,
        compression="zlib",
        least_significant_digit=3,
    )
    data_var_1.standard_name = "data_var_1"
    data_var_1.long_name = "data_var_1"
    data_var_1.units = "none"

    print("NetCDF file initialized.")

    # Fill the data variables.
    # data_var_1[:] = [1, 2]

    lon_var[:] = [
        -12,
        -8,
        -4,
        -2,
        -1.5,
        -0.5,
        -0.4,
        -0.3,
        -0.2,
        -0.1,
        0,
        0.1,
        0.2,
        0.3,
        0.5,
        1.5,
        2,
        4,
        8,
        12,
    ]
    latoffset = 52
    lat_var[:] = [
        -4 + latoffset,
        -2 + latoffset,
        -1.5 + latoffset,
        -0.5 + latoffset,
        -0.4 + latoffset,
        -0.3 + latoffset,
        -0.2 + latoffset,
        -0.1 + latoffset,
        0 + latoffset,
        0.1 + latoffset,
        0.2 + latoffset,
        0.3 + latoffset,
        0.5 + latoffset,
        1.5 + latoffset,
        2 + latoffset,
        4 + latoffset,
    ]
    time_var[:] = [
        netCDF4.date2num(
            datetime.datetime(2017, 1, 1, 0, 10, 0), "seconds since 1970-01-01 00:00:00"
        ),
        netCDF4.date2num(
            datetime.datetime(2017, 1, 1, 0, 11, 0), "seconds since 1970-01-01 00:00:00"
        ),
    ]

    for iextra, extra in enumerate(extra_var[:]):
        for it, time in enumerate(time_var[:]):
            for iy, lat in enumerate(lat_var[:]):
                for ix, lon in enumerate(lon_var[:]):
                    na = numpy.cos((iy / 15) * 3.141592654 + extra) + 0.5 * it
                    nb = numpy.cos((ix / 19) * 3.141592654 + extra) + 0.5 * it
                    data_var_1[iextra, it, iy, ix] = na * na + nb * nb
    # print(data_var_1[:])
    netcdf_file.close()
    print("Done.")


if __name__ == "__main__":
    main()
