AutoResource (enableautoopendap, enablelocalfile)
==============================================================

Back to [Configuration](./Configuration.md)

AutoResource enables the creation of WMS services on the fly from
opendap resources and local files. This can be useful to quicklook an
opendap data resource, or to create WMS services on the fly on a bunch
of files residing in a directory.

-   enableautoopendap - Set to "true" to allow external opendap
    connections
-   enablelocalfile - Set to true to allow the server to read local
    files, in this case additional [Dir](Dir.md) elements must be
    configured.

  
```xml
    <AutoResource enableautoopendap="false" enablelocalfile="false" >
      <Dir basedir="/data/sdpkdc/" prefix="/data/sdpkdc/"/>
      <Dir basedir="/data/omi/" prefix="/data/omi/"/>
    </AutoResource>
```

Files can now be accessed by adding the source=/data/myfile.nc to the
WMS and WCS urls.

-   See [Dir](Dir.md) for configuration of the Dir element
-   When using AutoResource, a style with name "auto" needs to be
    configured, see [Style](Style.md)

Using autoresource and serving OpenDAP
--------------------------------------

http://yourhost.com/adaguc.dataset.cgi/opendap/data/myfile.nc/testdata

1.  data/myfile.nc is the file identifier
2.  testdata is the variable in the netcdf file

-   [Dataset](Dataset.md) must be disabled in this case
-   See [OpenDAP](OpenDAP.md) for serving data with opendap.

