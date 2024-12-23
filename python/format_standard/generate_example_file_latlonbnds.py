from netCDF4 import Dataset
import numpy as np


def createnc(fn="spens1.nc", prob_type="f8", fill_value=None):
    nc_file = Dataset(fn, "w")

    nc_file.createDimension("time", 3)
    time_var = nc_file.createVariable("time", "i8", ("time",))
    time_var.units = "hours since 2024-06-01 00:00:00"
    time_var.calendar = "proleptic_gregorian"
    time_var[:] = [0, 1, 2]

    nc_file.createDimension("threshold", 4)
    threshold_var = nc_file.createVariable("threshold", "i8", "threshold")
    threshold_var[:] = [10, 20, 30, 40]

    nc_file.createDimension("gridpoint", 4)
    gridpoint_var = nc_file.createVariable("gridpoint", "i8", ("gridpoint",))
    gridpoint_var[:] = [0, 1, 2, 3]

    nc_file.createDimension("bnds_loc", 4)
    bnds_loc_var = nc_file.createVariable("bnds_loc", str, "bnds_loc")
    bnds_loc_var[:] = np.array(["down_left", "down_right", "up_left", "up_right"])

    if fill_value:
        probability_var = nc_file.createVariable(
            "probability",
            prob_type,
            ("time", "threshold", "gridpoint"),
            fill_value=fill_value,
        )
    else:
        probability_var = nc_file.createVariable(
            "probability",
            prob_type,
            ("time", "threshold", "gridpoint"),
        )
    probability_var.coordinates = "lat lon"
    value = []
    for tim in range(3):
        for threshold in range(4):
            for gridpoint in range(4):
                value.append(tim * 100 + threshold * 10 + gridpoint)
    values = np.array(value)
    probability_var[:] = values.reshape((3, 4, 4))

    lat = 52
    lon = 5
    lon_bnds_list = []
    lat_bnds_list = []
    dist = 0.5
    lat_bnds_list.extend([lat, lat, lat + dist, lat + dist])
    lat_bnds_list.extend([lat, lat, lat + dist, lat + dist])
    lat_bnds_list.extend([lat - dist, lat - dist, lat, lat])
    lat_bnds_list.extend([lat - dist, lat - dist, lat, lat])
    lon_bnds_list.extend([lon - 2 * dist, lon, lon - dist, lon])
    lon_bnds_list.extend([lon, lon + 2 * dist, lon, lon + dist])
    lon_bnds_list.extend([lon - 3 * dist, lon, lon - 2 * dist, lon])
    lon_bnds_list.extend([lon, lon + 3 * dist, lon, lon + 2 * dist])

    lon_bnds_var = nc_file.createVariable("lon_bnds", "f8", ("gridpoint", "bnds_loc"))
    lon_bnds_var[:] = np.array(lon_bnds_list).reshape((4, 4))
    lat_bnds_var = nc_file.createVariable("lat_bnds", "f8", ("gridpoint", "bnds_loc"))
    lat_bnds_var[:] = np.array(lat_bnds_list).reshape((4, 4))

    nc_file.coordinates = "lat_bnds lon_bnds"
    nc_file.close()


if __name__ == "__main__":
    createnc("latlonbnds_double.nc", "f8")
