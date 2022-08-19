WCSFormat (name, driver, mimetype, options)
===========================================

The formats the server can output data in. For this purpose GDAL is
used.

-   name - The name represented in the DescribeCoverage document.
-   driver - The GDAL driver to use, can be any capable of writing data
-   mimetype - The mimetype used when providing data during the
    GetCoverage request
-   options - Additional options to the GDAL driver

For example, the following configures two extra WCS output formats,
netcdf3 and netcdf4 by using the GDAL ADAGUC driver:
http://trac.osgeo.org/gdal/wiki/ADAGUC

```
<WCSFormat name="netcdf" driver="NetCDF"
mimetype="Content-Type:application/netcdf"
options="WRITE\_GDAL\_TAGS=FALSE,WRITE\_LONLAT=FALSE,WRITE\_BOTTOMUP=FALSE,ZLEVEL=2,FORMAT=NC4C"/>
<WCSFormat name="aaigrid" driver="AAIGrid"
mimetype="Content-Type:text/plain" />
<WCSFormat name="geotiff" driver="GTiff"
mimetype="Content-Type:text/plain" />

<WCSFormat name="NetCDF3" driver="ADAGUC"
mimetype="Content-Type:text/plain" options="FORCENC3=TRUE"/>
<WCSFormat name="NetCDF4" driver="ADAGUC"
mimetype="Content-Type:text/plain"/>
```
