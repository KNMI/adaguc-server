FilePath (filter, maxquerylimit, ncml, retentionperiod, retentiontype) <value>
====================================================

Back to [Configuration](./Configuration.md)

-   filter - A regular expression, used to filter files inside a
    directory
-   maxquerylimit - The maximum amount of time values will be used in
    the single request
-   ncml - The ncmlfile to apply, see [NCML support](../info/ncml.md) for
    details
-   retentionperiod - Optional setting in [iso period](../info/ISO8601.md) format to schedule an automatic cleanup. 
    Note that `<Settings enablecleanupsystem="true"/>` ( See [Settings](./Settings.md) ) also needs to be set.
-   retentiontype - Optional setting, default=`datatime`.The value `retentiontype="datatime"` configures cleaning based
    on the time or forecast_reference_time of files. This setting is used to create rolling archives of streaming datasets. The value `retentiontype="filedate"`
    configures cleaning based on the modification time of files. This setting enables a "temporary archive mode", where data is kept on the server for a certain period.\
-   `<value>` - Directory, file or OpenDAP url

The following configuration adds all files in the specified directory
matching the regular expression ^KMDS*TEST_P1M_OBSL2*_.*\.nc$ to
the service:
```xml
<FilePath filter="^KMDS*TEST_P1M_OBSL2*_.*\.nc$">/data/sdpkdc/kmds/</FilePath>
```

filter
------

-   In case a file or OpenDAP URL is given, the filter can be set to ""
    or can be left blank.

<!-- -->

-   If you would like to filter specific files you can use regular
    expressions.
    For example, if you would like to filter the following files based
    on prefixes:
    -   CFD_index_0.25deg_reg_v8.0.nc
    -   FD_index_0.25deg_reg_v8.0.nc
        You can use the filter "^CFD_index.**\$" and
  `      "^FD_index.**\$" respectively.

<!-- -->

-   The default to scan all files with .nc extension is "^.*\.nc$"

maxquerylimit
-------------

-   Allows you to set the maximum query limit of the database, default
    is 512 entries. Overrides the settings set in the [DataBase](DataBase.md)
    configuration.



retentionperiod and retentiontype 
--------------

This feature is available since adaguc-server version 2.7.11

The following dataset configuration configures a dataset which is updated every minute. Without the retentionperiod setting, the archive would grow infinetly large. By setting the retionperiod to `PT1H`, we keep the last hour of data. The files are first removed from the database and then from the filesystem.

- `retentionperiod="PT1H"` means that we want to keep the last hour until now (UTC time). You can configure this according to [iso period](../info/ISO8601.md)
- `retentiontype="datatime"` means that the value of the time dimension in the file is read to determine if the file is ready to be deleted.
- `retentiontype="filedate"` means that the value of the file modification date is read to determine if the file is ready to be deleted.

The retention check is done when a new file is added to the system. 

Manual trigger can be executed with the `--cleanfiles` command:
```
${ADAGUC_PATH}/bin/adagucserver --cleanfiles --config ${ADAGUC_CONFIG},livetimestream
```
But this is normally not necessary, because the step above is done during ingestion of new data.


```xml
<?xml version="1.0" encoding="UTF-8" ?>
<Configuration>
  
  <Settings enablecleanupsystem="true"/>
  
  <Layer type="database">
    <FilePath filter=".*\.nc$" retentionperiod="PT1H" retentiontype="datatime">/data/adaguc-data/livetimestream/</FilePath>
    
    <Variable>data</Variable>
    <Dimension name="time" interval="PT1M" quantizeperiod="PT1M" quantizemethod="low" >time</Dimension>
    <RenderMethod>rgba</RenderMethod>
  </Layer>

</Configuration>
```


Zarr stores on S3
-----------------

A Zarr store on S3 is given as the URL the NetCDF library uses to open it. A URL of which the path ends in a slash is a
directory of stores: adaguc-server lists the bucket and scans every store below it whose name matches the filter, like
files in a directory.

```xml
<!-- A single store -->
<FilePath filter="">https://s3.example.com/radar/timesteps/RAD_202405280000.zarr#mode=zarr,s3</FilePath>

<!-- All stores below the prefix timesteps/ of the bucket radar -->
<FilePath filter="^RAD_[0-9]+\.zarr$">https://s3.example.com/radar/timesteps/#mode=zarr,s3</FilePath>
```

-   A store is a key prefix ending in `.zarr/`. Prefixes that do not end in `.zarr/` are subdirectories and are
    listed as well.
-   The filter applies to the name of the store, for example `RAD_202405280000.zarr`. An empty filter matches all stores.
-   Each store is kept in the database under its own URL, for example
    `https://s3.example.com/radar/timesteps/RAD_202405280000.zarr#mode=zarr,s3`.
-   A scan of the dataset (`scan.sh -d <dataset>`) adds the stores that are new and removes the ones that are gone from the
    bucket. If the bucket cannot be listed the scan stops and the database is left as it is.
-   A single store can be added without listing the bucket: `scan.sh -f '<URL of the store>'`. This is the way to add
    stores as they arrive when the bucket holds very many of them.
-   Stores are assumed not to change: a URL has no modification date, so a store that was scanned is not scanned again.
    Use `scan.sh -t` to index everything again.
-   The URL has to be https; the NetCDF library does not use plain http for S3.
-   Credentials and region are taken from the environment variables `AWS_ACCESS_KEY_ID`, `AWS_SECRET_ACCESS_KEY` and
    `AWS_REGION`, both for listing the bucket and by the NetCDF library for opening the stores.
-   adaguc-server must be built with a NetCDF library that has S3 support and the filter plugins for the compression used
    in the stores; see the Dockerfile.
