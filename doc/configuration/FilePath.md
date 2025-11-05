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
