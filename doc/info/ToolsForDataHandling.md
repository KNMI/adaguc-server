
# Tools to handle data

### Generic NetCDF tools - ncgen and ncdump

The NetCDF4 software contains two very useful tools for exploring and
manipulating NetCDF data files.

1.  **ncdump** can show the contents (headers only or headers and data)
    for any NetCDF file (or OpenDAP url). It has several command-line
    options (like -h for showing headers only). ncdump is very useful
    for inspection of NetCDF data when configuring an ADAGUC service
    layer.
2.  **ncgen** can generate a NetCDF file from an ASCII file in the CDL
    format which happens to be the format that ncdump dumps the data in.
    So for simple NetCDF manipulations one could ncdump the file, edit
    the resulting text and run that through ncgen to generate a new
    NetCDF file.

### GRIB data conversion tools

Weather model data is most often coded in GRIB-1 or GRIB-2 format. Some
tools which can convert GRIB files to NetCDF files are:

- **grib2netcdf** from ECMWF's grib_api package (in development)

- **cdo** from the Max Planck Institute for Meteorology
(https://code.zmaw.de/projects/cdo)
- **fimex** from the Norwegian Meteorological Institute met.no
(http://fimex.met.no) which is a nice tool for generating NetCDF data
from GRIB files.

### Manipulating NetCDF data with Python

Python has some very good possibilities for manipulating large fields of
data and writing data into NetCDF4 files is fairly easy with the
[netcdf4-python](http://code.google.com/p/netcdf4-python) library. In
combination with for example the grib_api library and/or the
[pygrib](https://code.google.com/p/pygrib) library converters can be
built from GRIB to NetCDF.
Python also has [Numpy](http://www.numpy.org) modules which make
handling of large arrays easy.

### Satellite data with PyTroll

Satellite data from polar orbiting and geostationary satellites can be
turned into products with the [pytroll](http://www.pytroll.org)
software, based on Python. A module to generate RGBA or normal grid
products as part of Pytroll is almost finished.
