DataFormats
===========

The Adaguc server can visualise a number of dataypes from data in NetCDF
datafiles (NetCDF3 and NetCDF4). Data can also be read from some types
of HDF5 files, like HDF5 files according to the KNMI HDF5 specification.
When NetCDF data is following the CF conventions, simple automated
visualisation is possible by using this extra information. The CF
Conventions are described in detail on the
[CF-Metadata](http://cf-pcmdi.llnl.gov) site.

Data types
----------

### Structured Grid or field information - Model/satellite/radar etc. (NetCDF)

NetCDF files using structured grids contain variables with gridded data,
this type of NetCDF file occur most frequently. There are two types of
structured grids, unprojected grids in standard lat/lon projection and
projected grids described according a geographical projection. When
projected data is used, parameter variables need to link to projection
variables to describe the geographical projection and projection
coordinates need to be described. Usually the geographical projection is
also described by giving all longitudes and latitudes for all grid
points. These files are the "classic" examples of CF Convention
following files. ADAGUC only uses the projection variable and the
projection coordinates to calculated the coordinates for each grid
point, it does not read all latitudes and longitudes for every grid
point and are not required to plot the data correctly.

![](https://dev.knmi.nl/attachments/681/regular_and_projected_grids.jpg)

Example of a regular lat/lon NetCDF file (topleft image):
```
netcdf latlon {
dimensions:
lon = 360 ;
lat = 180 ;
time = 12 ;
variables:
double lon(lon) ;
lon:long\_name = "longitude" ;
lon:standard\_name = "longitude" ;
lon:units = "degrees\_east" ;
double lat(lat) ;
lat:long\_name = "latitude" ;
lat:standard\_name = "latitude" ;
lat:units = "degrees\_north" ;
double time(time) ;
time:long\_name = "time" ;
time:units = "days since 2010-01-01 00:00:00" ;
double NOx(time, lat, lon) ;
NOx:Units = "Gg N/km2" ;

// global attributes:
:Conventions = "CF-1.4" ;
}
```

Example of a projected coordinate system, uses a polar stereographic
projection (topright image):
```
netcdf projected {
dimensions:
x = 700 ;
y = 765 ;
time = 1 ;
variables:
double x(x) ;
x:long\_name = "x coordinate of projection" ;
x:standard\_name = "projection\_x\_coordinate" ;
x:units = "km" ;
double y(y) ;
y:long\_name = "y coordinate of projection" ;
y:standard\_name = "projection\_y\_coordinate" ;
y:units = "km" ;
double time(time) ;
time:units = "minutes since 1950-01-01 0:0:0" ;
time:long\_name = "time" ;
time:standard\_name = "time" ;
float precipitation(time, y, x) ;
precipitation:grid\_mapping = "projection" ;
precipitation:\_FillValue = -1.f ;
precipitation:long\_name = "precipitation flux" ;
precipitation:units = "kg/m2/h" ;
precipitation:standard\_name = "precipitation\_flux" ;
char projection ;
projection:EPSG\_code = "none" ;
projection:grid\_mapping\_name = "polar\_stereographic" ;
projection:latitude\_of\_projection\_origin = 90. ;
projection:straight\_vertical\_longitude\_from\_pole = 0. ;
projection:scale\_factor\_at\_projection\_origin = 0.933012709177451 ;
projection:false\_easting = 0. ;
projection:false\_northing = 0. ;
projection:semi\_major\_axis = 6378140. ;
projection:semi\_minor\_axis = 6356750. ;
projection:proj4\_params = "+proj=stere +lat\_0=90 +lon\_0=0 +lat\_ts=60
+a=6378.14 +b=6356.75 +x\_0=0 y\_0=0" ;
projection:long\_name = "projection" ;

// global attributes:
:Conventions = "CF-1.4" ;
}
```

### RGBA grid information / True color images (NetCDF)

A special case are geographical grids which do not contain values of a
parameter, but contains the RGBA value of the colour that should be
presented at that grid point. This kind of information is usually found
in satellite products, where the values of different bands are combined
(composited) into one RGB or RGBA value. The value of the parameter at a
grid point is the value that should be plotted on the visualisation. You
could see these fields as color photos, where each pixel is
georeferenced.
![](https://dev.knmi.nl/attachments/678/truecolor_rgba_netcdf_adaguc.jpg)
The CF-conventions currently have no way to describe these parameters,
so these files are generated as fields of 32 bit unsigned integers, each
containing an RGBA value. The configuration of the layer tells the
service that the parameter values should be interpreted as pixel color
values, this can be done by choosing [RenderMethod](RenderMethod.md) rgba. The
service can then reproject the image from the source projection, if
needed, by doing a nearest neighbor "interpolation". When the
standard\_name "rgba" and units "rgba" is given for the corresponding
variable, the system detects that this is a RGBA/True color variable.
The variable type should be unsigned integer (NC\_UINT) and represents 4
bytes for R,G,B and A respectively.

```
netcdf butterfly\_fromjpg\_truecolor {
dimensions:
y = 1080 ;
x = 1920 ;
time = 1 ;
variables:
double y(y) ;
y:units = "m" ;
y:standard\_name = "projection\_y\_coordinate" ;
double x(x) ;
x:units = "m" ;
x:standard\_name = "projection\_x\_coordinate" ;
double time(time) ;
time:units = "seconds since 1970-01-01 00:00:00" ;
time:standard\_name = "time" ;
byte projection ;
projection:proj4 = "+proj=sterea +lat\_0=52.15616055555555
+lon\_0=5.38763888888889 +k=0.9999079 +x\_0=155000 +y\_0=463000
+ellps=bessel +units=m +no\_defs" ;
uint butterfly(time, y, x) ;
butterfly:units = "rgba" ;
butterfly:standard\_name = "rgba" ;
butterfly:long\_name = "butterfly" ;
butterfly:grid\_mapping = "projection" ;

// global attributes:
:Conventions = "CF-1.4" ;
}
```

### Point data (NetCDF)

Point data is new in CF Conventions is characterised by a global
attribute **featureType = "timeSeries"** in the NetCDF file.
![](https://dev.knmi.nl/attachments/679/point_timeseries_adaguc_automated_weatherobservations.jpg)
A detailed example is described here: \[\[Pointtimeseries\_example\]\]

### Forecast reference times from model runs

Adaguc can display model data with forecasts. Multiple files with
different forecast\_reference\_time's and overlapping times can be
aggregated in a single layer.

A variable with standard\_name forecast\_reference\_time should be
present in the netcdf file(s):
```
double forecast\_reference\_time ;
forecast\_reference\_time:units = "seconds since 1970-01-01 00:00:00
+00:00" ;
forecast\_reference\_time:standard\_name = "forecast\_reference\_time"
```

This variable can be either a scalar or a variable with multiple dates.

### Ugrid data

Experimental support for UGRID data (meshes and polygons) is built in.
When a NetCDF file with UGRID convention is configured in the service,
the service will render a GetMap image with the UGRID mesh displayed in
it.
![](https://dev.knmi.nl/attachments/680/ugrid_mesh_adaguc.jpg)
For details on the UGRID format you can visit
https://github.com/ugrid-conventions/ugrid-conventions.

### Curvilinear data

![](https://dev.knmi.nl/attachments/675/curvilinear_adaguc.jpg)

### Vector data - Swath information (NetCDF)

The ADAGUC service can handle swath data from orbiting satellites in the
so called ADAGUC-format, described at
http://adaguc.knmi.nl/contents/documents/ADAGUC\_Standard.html (called
vector data there). This data is organized in time related rows of
geographical "tiles" or "pixels" each representing one measured value.

### Swath data - ASCAT data (NetCDF)

The ASCAT NetCDF data format (see http:www.knmi.nl/scatterometer)
describes measured wind direction and wind speed (and some other
parameters) for polar orbiting satellites. This file format is
recognised by the ADAGUC server.
Data is organised in a grid of values for a certain parameter. Wind
direction and wind speed (which are the main scatterometer products)can
be depicted by the service as wind barbs or wind vectors.
The ADAGUC standard distinguishes between **geographic raster data** and
**projected raster data**. The grid of geographic raster data can be
described by two axes (two series of points). The grid of projected
raster data also has two axes, but these do not contain geographic
coordinates. The NetCDF files contain parameters (two-dimensional) for
the latitude and longitude of each grid-point. This kind of raster is
for example found in numerical model data with a rotated lat-lon grid
like HIRLAM.

### Vector components in gridded fields, e.g. U and V for wind (NetCDF)

In certain data sets, in particular in the case of numerical weather
model output, wind is available as two vector components. For displaying
wind vectors or wind barbs these two components have to be combined into
a wind direction parameter and a wind speed parameter. These u and v
vectors can be oriented along the model grid or can be oriented along
true north/east directions. The former case is often found in models
which have a rotated grid; these rotations are sometimes used to get rid
of erratic behaviour of models around the poles. For example HIRLAM data
is expressed in a rotated lat lon grid, with grid-oriented u and v
vectors.
When calculating (wind) speed and direction from grid-oriented u and v
vectors for display of barbs or vectors, the service takes into account
the rotated grid if needed by inspection of the standard\_name attribute
of the u-component.

### HDF5 data

The ADAGUC service can read HDF5 files in the KNMI HDF5 format. These
files for satellite and radar measurements contain metadata for the
projection and the time of the measurement. The ADAGUC service can not
handle HDF5 lightning files currently, nor can it handle radar elevation
data.
**Update since 2015-07-13**
The server is able to read KNMI HDF5 precipitation forecast data. These
files contain several image groups with a forecast date timestamp. These
image groups are aggregated to a single variable with time dimension,
this new variable is called "forecast". An example file is attached. See
https://dev.knmi.nl/attachments/712/RAD\_NL25\_PCP\_FM\_201506180815.h5

Data organisation
-----------------

Tools to handle data
--------------------

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

- **grib2netcdf** from ECMWF's grib\_api package (in development)

- **cdo** from the Max Planck Institute for Meteorology
(https://code.zmaw.de/projects/cdo)
- **fimex** from the Norwegian Meteorological Institute met.no
(http://fimex.met.no) which is a nice tool for generating NetCDF data
from GRIB files.

### Manipulating NetCDF data with Python

Python has some very good possibilities for manipulating large fields of
data and writing data into NetCDF4 files is fairly easy with the
[netcdf4-python](http://code.google.com/p/netcdf4-python) library. In
combination with for example the grib\_api library and/or the
[pygrib](https://code.google.com/p/pygrib) library converters can be
built from GRIB to NetCDF.
Python also has [Numpy](http://www.numpy.org) modules which make
handling of large arrays easy.

### Satellite data with PyTroll

Satellite data from polar orbiting and geostationary satellites can be
turned into products with the [pytroll](http://www.pytroll.org)
software, based on Python. A module to generate RGBA or normal grid
products as part of Pytroll is almost finished.
