"""
File to generate a file thresholds example
Purpose: Test wind barb visualisation in adaguc-server
"""
import datetime
import os.path
import netCDF4
import numpy as np


X_DIMNAME = "x"
Y_DIMNAME = "y"
TIME_DIMNAME = "time"
THRESHOLD_DIMNAME = "threshold"
TIME_DIMUNITS = "seconds since 1970-01-01 00:00:00"

def normalize_array(data):
    """Normalizes array"""
    return (data - np.min(data)) / (np.max(data) - np.min(data))

def main():
    """Main routine"""
    netcdf_file = netCDF4.Dataset(os.path.join(
        "/data/adaguc-autowms/demo.nc"),
                                  mode="w",
                                  format="NETCDF4_CLASSIC")

    x_dimdata = np.arange(-1500000, 2000000, 50000)
    y_dimdata = np.arange(-1000000, 2000000, 50000)
    t_dimdata = [netCDF4.date2num(datetime.datetime(2025,6,10,10,0,0), TIME_DIMUNITS)]
    threshold_dimdata = np.arange(0, 100, 2)

    np_array = np.empty((threshold_dimdata.size, len(t_dimdata), y_dimdata.size, x_dimdata.size))

    # Create the dimensions.
    netcdf_file.createDimension(X_DIMNAME, x_dimdata.size)
    netcdf_file.createDimension(Y_DIMNAME, y_dimdata.size)
    netcdf_file.createDimension(TIME_DIMNAME, len(t_dimdata))
    netcdf_file.createDimension(THRESHOLD_DIMNAME, threshold_dimdata.size)


    lon_var = netcdf_file.createVariable(X_DIMNAME, "f8", (X_DIMNAME))
    lon_var.standard_name = "x coordinate of projection"
    lon_var.long_name = "projection_x_coordinate"
    lon_var.units = "km"

    # Create the coordinate variables.
    lat_var = netcdf_file.createVariable(Y_DIMNAME, "f8", (Y_DIMNAME))
    lat_var.standard_name = "y coordinate of projection"
    lat_var.long_name = "projection_y_coordinate"
    lat_var.units = "km"

    time_var = netcdf_file.createVariable(TIME_DIMNAME, "f8", (TIME_DIMNAME))
    time_var.standard_name = "time"
    time_var.long_name = "time"
    time_var.units = TIME_DIMUNITS

    threshold_var = netcdf_file.createVariable(THRESHOLD_DIMNAME, "f8", (THRESHOLD_DIMNAME))
    threshold_var.standard_name = "threshold"
    threshold_var.long_name = "threshold"
    threshold_var.units = "%"

    # Geographic information
    project_var = netcdf_file.createVariable("projection", "c")
    project_var.proj4_params = "+proj=sterea +lat_0=52.1561605555556 +lon_0=5.38763888888889 +k=0.9999079 +x_0=155000 +y_0=463000 +ellps=bessel +towgs84=565.4171,50.3319,465.5524,1.9342,-1.6677,9.1019,4.0725 +units=m +no_defs +type=crs"
    project_var.long_name = "projection"

    print("NetCDF file initialized.")

    # Create the measured value variables.
    data_var = netcdf_file.createVariable("data_var",
                                            "f8",
                                            (THRESHOLD_DIMNAME, TIME_DIMNAME, Y_DIMNAME, X_DIMNAME),
                                            complevel=2,
                                            compression='zlib')
    data_var.standard_name = "data_var"
    data_var.long_name = "data_var"
    data_var.units = "just a value"
    data_var.grid_mapping = "projection"

    # Fill the data variables.
    data_var[:] = np.full(shape=np_array.shape, fill_value=0)

    for threshold in range(0,threshold_dimdata.size):
        frac_threshold = threshold / threshold_dimdata.size
        percentage = frac_threshold*100
        print (f"{percentage:0.0f}%")

        for it_x in range(0,x_dimdata.size):
            for it_y in range(0,y_dimdata.size):
                frac_x = (it_x / x_dimdata.size)-0.5
                frac_y = (it_y / y_dimdata.size)-0.5
                frac_threshold_multip = (frac_threshold-0.5)*5
                grid_cell_value = frac_x*frac_x+(frac_y*frac_x*frac_threshold_multip*frac_threshold_multip*frac_threshold_multip)+frac_y*frac_y
                data_var[threshold,0,it_y,it_x] = grid_cell_value

    data_var[:] = normalize_array(data_var[:])

    lon_var[:] = x_dimdata
    lat_var[:] = y_dimdata
    time_var[:] = t_dimdata
    threshold_var[:] = threshold_dimdata

    netcdf_file.close()
    print("Done.")


if __name__ == "__main__":
    main()
