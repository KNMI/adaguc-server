"""
Creates point timeseries NetCDF files with a forecast reference time, in two layouts:
- dimension/: reference_time is a dimension of the point variables, ta(station, reference_time, time)
- scalar/: forecast_reference_time is a scalar variable, ta(station, time)
- scalar_frt/: like scalar/, but the scalar variable is named frt. Only its standard_name tells it is a reference time.

Each file has one constant temperature, so the selected file can be recognized in the rendered image.
Usage: python3 create_point_reference_time.py <outputdir> [layout ...]
"""

import datetime
import os
import sys

import netCDF4
import numpy as np

STATIONS = [
    ("06212", "HOORN-A", 52.9181, 4.1503),
    ("06215", "VOORSCHOTEN AWS", 52.1397, 4.4364),
    ("06225", "IJMUIDEN WP", 52.4622, 4.5549),
    ("06229", "TEXELHORS WP", 52.9950, 4.7199),
    ("06235", "DE KOOY VK", 52.9269, 4.7811),
    ("06240", "AMSTERDAM/SCHIPHOL AP", 52.3172, 4.7897),
    ("06248", "WIJDENES WP", 52.6324, 5.1735),
    ("06249", "BERKHOUT AWS", 52.6427, 4.9788),
]

# (reference time, time, temperature)
FILES = [
    ("2020-12-20T06:00:00", "2020-12-20T12:00:00", 11),
    ("2020-12-20T06:00:00", "2020-12-20T12:10:00", 22),
    ("2020-12-20T09:00:00", "2020-12-20T12:00:00", 33),
]

TIME_UNITS = "seconds since 1970-01-01 00:00:00"


def seconds(isodate):
    return (datetime.datetime.fromisoformat(isodate) - datetime.datetime(1970, 1, 1)).total_seconds()


def create_file(filename, layout, reftime, time, value):
    ncfile = netCDF4.Dataset(filename, "w")
    ncfile.featureType = "timeSeries"
    ncfile.Conventions = "CF-1.6"
    ncfile.createDimension("station", len(STATIONS))
    ncfile.createDimension("time", 1)

    station = ncfile.createVariable("station", str, ("station",))
    station.long_name = "Station id"
    stationname = ncfile.createVariable("stationname", str, ("station",))
    stationname.long_name = "Station name"
    lat = ncfile.createVariable("lat", "f8", ("station",))
    lat.standard_name = "latitude"
    lat.units = "degrees_north"
    lon = ncfile.createVariable("lon", "f8", ("station",))
    lon.standard_name = "longitude"
    lon.units = "degrees_east"
    for i, (stationid, name, stationlat, stationlon) in enumerate(STATIONS):
        station[i] = stationid
        stationname[i] = name
        lat[i] = stationlat
        lon[i] = stationlon

    timevar = ncfile.createVariable("time", "f8", ("time",))
    timevar.standard_name = "time"
    timevar.units = TIME_UNITS
    timevar[:] = [seconds(time)]

    if layout == "dimension":
        ncfile.createDimension("reference_time", 1)
        reftimevar = ncfile.createVariable("reference_time", "f8", ("reference_time",))
        reftimevar[:] = [seconds(reftime)]
        ta = ncfile.createVariable("ta", "f8", ("station", "reference_time", "time"))
        ta[:] = np.full((len(STATIONS), 1, 1), value)
    else:
        reftimevar = ncfile.createVariable("frt" if layout == "scalar_frt" else "forecast_reference_time", "f8", ())
        reftimevar.assignValue(seconds(reftime))
        ta = ncfile.createVariable("ta", "f8", ("station", "time"))
        ta[:] = np.full((len(STATIONS), 1), value)
    reftimevar.standard_name = "forecast_reference_time"
    reftimevar.units = TIME_UNITS
    ta.standard_name = "air_temperature"
    ta.units = "degrees Celsius"
    ncfile.close()


def main():
    outputdir = sys.argv[1] if len(sys.argv) > 1 else "."
    layouts = sys.argv[2:] if len(sys.argv) > 2 else ("dimension", "scalar", "scalar_frt")
    for layout in layouts:
        os.makedirs(os.path.join(outputdir, layout), exist_ok=True)
        for reftime, time, value in FILES:
            filename = f"points_ref{reftime[11:13]}_t{time[11:13]}{time[14:16]}.nc"
            create_file(os.path.join(outputdir, layout, filename), layout, reftime, time, value)


if __name__ == "__main__":
    main()
