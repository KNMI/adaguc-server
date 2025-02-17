Create a WMS service on a series of files with a time dimension
===============================================================

[Back to readme](./Readme.md)

To serve a series of files with a time component in your WMS service, you need to add the directory where all files are located and you need to provide information about the time dimension.

For example, if you have monthly files located in /data/adaguc-data/mymonthlyfiles/, you have to point the [FilePath](../configuration/FilePath.md) element to that location. You can specify a filter to filter the files you would like to add. This filter uses posix regular expressions to make the selection.

Because the data has a time dimension, you need to configure a time dimension in the layer. See [Dimension](../configuration/Dimension.md) for details on the configuration.
We select a interval of "P1M", because we are dealing with montly data See [ISO8601 notation](../info/ISO8601.md) for details on this notation.

Part of mymonthlyconfig.xml, stored in the dataset folder (/data/adaguc-datasets/)

```xml
<?xml version="1.0" encoding="UTF-8" ?>
<Configuration>
  <Layer type="database">
    <FilePath filter="^.*\.nc$">/data/adaguc-data/mymonthlyfiles/</FilePath>
    <Variable>interpolatedObs</Variable>
    <Styles>rainbow</Styles>
    <Dimension name="time" interval="P1M" default="max">time</Dimension>
  </Layer>
</Configuration>
```

You could copy these files from [here - python/examples/rundataset/data/sequence](../../python/examples/rundataset/data/sequence) to /data/adaguc-data/mymonthlyfiles/.

(`"^.*\.nc$"` is a regular expression, see http://en.wikipedia.org/wiki/Regular_expression)

When everything is configured, you are ready to scan (or ingest) the files, this can be done with the following command:

```
docker exec -i -t my-adaguc-server /adaguc/scan.sh -d mymonthlyconfig
```

It should print something like:

```
$ docker exec -i -t my-adaguc-server /adaguc/scan.sh -d mymonthlyconfig
Using adagucserver from  /adaguc/adaguc-server-master
Using config from /adaguc/adaguc-server-config.xml

*** Starting update for /data/adaguc-datasets/mymonthlyconfig
[D:000:pid137: adagucserverEC/CRequest.cpp:109 CRequest]                        Dataset name based on passed configfile is [mymonthlyconfig]
[D:001:pid137: adagucserverEC/CAutoResource.cpp:82 CAutoResource]               Found dataset /data/adaguc-datasets/mymonthlyconfig.xml
[D:002:pid137: adagucserverEC/CDBFileScanner.cpp:920 CDBFileScanner]            *** Starting update layer 'interpolatedObs' ***
[D:003:pid137: adagucserverEC/CDBFileScanner.cpp:922 CDBFileScanner]            Using path [/data/adaguc-data/mymonthlyfiles], filter [^.*\.nc$] and tailpath []
[D:004:pid137: adagucserverEC/CDBFileScanner.cpp:1065 CDBFileScanner]           Reading directory /data/adaguc-data/mymonthlyfiles with filter ^.*\.nc$
[D:005:pid137: hclasses/CDirReader.cpp:60 CDirReader]                           Doing recursive directory scan for [/data/adaguc-data/mymonthlyfiles]
[D:006:pid137: adagucserverEC/CDBFileScanner.cpp:950 CDBFileScanner]            Found 1 files
[D:007:pid137: adagucserverEC/CDBFileScanner.cpp:124 CDBFileScanner]            Checking dim [time]
[D:008:pid137: adagucserverEC/CDataReader.cpp:1378 CDataReader]                 Warning no standard name given for dimension time, using variable name instead.
[D:009:pid137: adagucserverEC/CDBAdapterPostgreSQL.cpp:923 CDBAdapterPostgreSQL] New table created: Set indexes
[D:010:pid137: adagucserverEC/CDataReader.cpp:1378 CDataReader]                 Warning no standard name given for dimension time, using variable name instead.
[D:011:pid137: adagucserverEC/CDBFileScanner.cpp:367 CDBFileScanner]            Found dimension 0 with name time of type 2, istimedim: [1]
[D:012:pid137: adagucserverEC/CDBFileScanner.cpp:390 CDBFileScanner]            Marking table done for dim 'time' with table 't20220914t151513526_shrmzjounuczvyyplawg'.
[D:013:pid137: adagucserverEC/CDBFileScanner.cpp:1015 CDBFileScanner]           *** Finished update layer 'interpolatedObs' ***

[D:014:pid137: adagucserverEC/CRequest.cpp:3501 CRequest]                       ***** Finished DB Update *****
```

Your WMS is now available. You can find it via the `wms?dataset=mymonthlyconfig&` parameters in the Url, or by exploring with the autowms in the `adaguc::datasets` folder.


The scan is recursive, e.g. files may be nested in subdirectories:

```
/data/adaguc-data/mymonthlyfiles/2013/jfm/jan.nc
...
/data/adaguc-data/mymonthlyfiles/2013/amj/apr.nc
...
```

If you would like to add a single file in the configured data directory,
you can do:

```
docker exec -i -t my-adaguc-server /adaguc/scan.sh -d mymonthlyconfig apr.nc
```

If you would like to scan specific files in a certain subdirectory of
the configured data directory, you can do:
```
docker exec -i -t my-adaguc-server /adaguc/scan.sh -d mymonthlyconfig 2013
```

This adds the files which are located in the given subdirectory. If
files are already added in a previous command, nothing will change.

Removing files from the service
-------------------------------

If you would like to remove files from the service, because they do not
exist anymore on the filesystem or for any other reason, you can call
updatedb again with only the dataset:
```
docker exec -i -t my-adaguc-server /adaguc/scan.sh -d mymonthlyconfig
```

This checks if files are present in adaguc-server database but not on the filesystem. It will also check if files have been updated.
