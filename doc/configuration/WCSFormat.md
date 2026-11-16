WCSFormat (name, driver, mimetype, options)
===========================================

Back to [Configuration](./Configuration.md)

The formats the server can output data in with WCS GetCoverage.

-   name - The name represented in the DescribeCoverage document, used in the `FORMAT` parameter of GetCoverage.
-   driver - The writer to use:
    -   `ADAGUCNetCDF`: CF NetCDF, written by adaguc-server.
    -   `GTiff`: GeoTIFF, uncompressed, one band per time step. The CRS is written as EPSG code, or as user defined projection for
        proj strings. The nodata value and the dimension values are written in the same TIFF tags as GDAL uses.
    -   `AAIGrid`: ESRI ASCII grid, for a single band only. Non-square cells are written with `dx` and `dy`.
    -   `NetCDF`, the name of the GDAL NetCDF driver used by older configurations, is written by `ADAGUCNetCDF`.

    GDAL is not used anymore: other GDAL driver names give an error on GetCoverage.
-   mimetype - The mimetype used when providing data during the
    GetCoverage request
-   options - Not supported: these were GDAL creation options. When set, they are ignored and a warning is logged.

For example, the following configures the output formats of the default configuration (`data/config/includes/WCSFormat.include.xml`):

```xml
<WCSFormat name="netcdf" driver="ADAGUCNetCDF" mimetype="Content-Type:application/netcdf"/>

<WCSFormat name="NetCDF4" driver="ADAGUCNetCDF" mimetype="Content-Type:application/netcdf"/>

<WCSFormat name="aaigrid" driver="AAIGrid" mimetype="Content-Type:text/plain" />

<WCSFormat name="geotiff" driver="GTiff" mimetype="Content-Type:text/plain" />
```
