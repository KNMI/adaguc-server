Faster WMS by enabling tiling for large datasets like MeteoSat Third Generation
===============================================

In this case, tiling does not mean a tiled WMS. Instead it means cutting the input data in smaller chunks at different resolutions. This makes it possible for the WMS server to quickly generate images for the GetMap requests.

[Back to readme](./Readme.md).

Add:

```xml
<TileSettings debug="true" autotile="file" maxtilesinimage="32"/>
```
to your layer.

See TileSettings for details. [TileSettings](../configuration/TileSettings.md) - Configuration settings for tiling high    resolution layers.

`mtg-fci.xml`:
```xml
<?xml version="1.0" encoding="UTF-8" ?>
<Configuration>
  <WMS>
    <WMSFormat name="image/png" format="image/webp"/>
  </WMS>

  <Style name="mtg-fci-0-60" name="0 - 60" abstract="MTG FCI between 0 and 60">
    <Legend tickinterval="10">gray</Legend>
    <Min>0</Min>
    <Max>60</Max>
    <RenderMethod>nearest</RenderMethod>
  </Style>

  <Layer type="database">
    <Group value="MTG FCI channels" />
    <Name force="true">vis_06</Name>
    <Title>MTG Channel 3 VIS 0.6 NH</Title>
    <Variable units="Reflectance">data</Variable>
    <FilePath
      filter="^MTG-FCI-FD_NH_1km_vis_06_.*\.nc$" retentionperiod="{ADAGUCENV_RETENTIONPERIOD}"
      retentiontype="datatime">/data/adaguc-data/mtgnc/</FilePath>
    <Dimension name="time" interval="PT10M">time</Dimension>
    <TileSettings debug="true" autotile="file" maxtilesinimage="32"/>
    <Styles>mtg-fci-0-60</Styles>
  </Layer>

</Configuration>
```


After this you can scan new data as normal. Tiles will be generated automatically and the WMS service will be significantly faster.

```sh
 ./scripts/scan.sh -t -d mtg-fci 
```

```
[D:001:pid128672: adagucserverEC/CDBFileScanner.cpp:877]                          ==> *** Starting update layer [vis_06] ***
[D:002:pid128672: adagucserverEC/CDBFileScanner.cpp:1070]                       Reading directory /data/adaguc-data/mtgnc with filter ^MTG-FCI-FD_NH_1km_vis_06_.*\.nc$
[D:003:pid128672: adagucserverEC/CDBFileScanner.cpp:181]                        Recreating table: Now dropping table t20260205t140146460_hti51dl6kdw7myyzkxvx
[D:004:pid128672: adagucserverEC/CDBAdapterPostgreSQL.cpp:971]                  New table created: Set indexes
[D:005:pid128672: adagucserverEC/CDBFileScanner.cpp:486]                        Scan /data/adaguc-data/mtgnc/MTG-FCI-FD_NH_1km_vis_06_202602030930.nc
[W:006:pid128672: adagucserverEC/CDataSource.cpp:667]                           Deprecated to have RenderMethod configs in the style.
[W:007:pid128672: adagucserverEC/CDBFileScannerCleanFiles.cpp:38]               Layer wants to autocleanup, but attribute enablecleanupsystem in Settings is not set to true or dryrun but to false
[D:008:pid128672: adagucserverEC/CCreateTiles.cpp:118]                          Opening input file for tiles: /data/adaguc-data/mtgnc/MTG-FCI-FD_NH_1km_vis_06_202602030930.nc
[D:009:pid128672: adagucserverEC/CCreateTiles.cpp:182]                          Generating  MTG-FCI-FD_NH_1km_vis_06_202602030930-001_000_000tile.nc 1.1 done
[D:010:pid128672: CCDFDataModel/cdfVariableCache.cpp:45]                        Made Cache [[file:/data/adaguc-data/mtgnc/MTG-FCI-FD_NH_1km_vis_06_202602030930.nc][var:data][id:3][type:6][dims:[0: time 0 1 1][1: y 0 5568 1][2: x 0 11136 1]][pointer:0x781d720ee010]]
[D:011:pid128672: adagucserverEC/CDBFileScanner.cpp:486]                        Scan /data/adaguc-data/mtgnc/MTG-FCI-FD_NH_1km_vis_06_202602030930-001_000_000tile.nc
[D:012:pid128672: adagucserverEC/CCreateTiles.cpp:182]                          Generating  MTG-FCI-FD_NH_1km_vis_06_202602030930-001_000_001tile.nc 2.2 done
[D:013:pid128672: adagucserverEC/CDBFileScanner.cpp:486]                        Scan /data/adaguc-data/mtgnc/MTG-FCI-FD_NH_1km_vis_06_202602030930-001_000_001tile.nc
[D:014:pid128672: adagucserverEC/CCreateTiles.cpp:182]                          Generating  MTG-FCI-FD_NH_1km_vis_06_202602030930-001_000_002tile.nc 3.3 done
[D:015:pid128672: adagucserverEC/CDBFileScanner.cpp:486]                        Scan /data/adaguc-data/mtgnc/MTG-FCI-FD_NH_1km_vis_06_202602030930-001_000_002tile.nc
[D:016:pid128672: adagucserverEC/CCreateTiles.cpp:182]                          Generating  MTG-FCI-FD_NH_1km_vis_06_202602030930-001_000_003tile.nc 4.4 done
[D:017:pid128672: adagucserverEC/CDBFileScanner.cpp:486]                        Scan /data/adaguc-data/mtgnc/MTG-FCI-FD_NH_1km_vis_06_202602030930-001_000_003tile.nc
[D:018:pid128672: adagucserverEC/CCreateTiles.cpp:182]                          Generating  MTG-FCI-FD_NH_1km_vis_06_202602030930-001_000_004tile.nc 5.6 done
[D:019:pid128672: adagucserverEC/CDBFileScanner.cpp:486]                        Scan /data/adaguc-data/mtgnc/MTG-FCI-FD_NH_1km_vis_06_202602030930-001_000_004tile.nc
[D:020:pid128672: adagucserverEC/CCreateTiles.cpp:182]                          Generating  MTG-FCI-FD_NH_1km_vis_06_202602030930-001_000_005tile.nc 6.7 done
[D:021:pid128672: adagucserverEC/CDBFileScanner.cpp:486]                        Scan /data/adaguc-data/mtgnc/MTG-FCI-FD_NH_1km_vis_06_202602030930-001_000_005tile.nc
[D:022:pid128672: adagucserverEC/CCreateTiles.cpp:182]                          Generating  MTG-FCI-FD_NH_1km_vis_06_202602030930-001_000_006tile.nc 7.8 done
```


## More info

For more details on datapostproc usage, see [DataPostProc.md](../configuration/DataPostProc.md).