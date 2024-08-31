Layer (type,hidden)
===================

Back to [Configuration](./Configuration.md)

-   type - The type of layer, can be either database,grid, liveupdate or cascaded
-   hidden - When set to true, the Layer is not advertised in the
    GetCapabilities document, but is accessible with GetMap requests.

Below an example of a minimum Layer configuration is given:
```xml
<Layer type="database">
  <Variable>T2m</Variable>
  <FilePath>/data/sdpkdc/har_nc/t2mtest.nc</FilePath>
  <Styles>temperature</Styles>
</Layer>
```

Grid Layers
-----------

This type of layer show a Graticule grid, displaying meridians and
parallels.
Here an example of a grid layer with a 10 degree interval is given:

```xml
<Layer type="grid">
  <Group value="baselayers"/>
  <Name force="true">grid10</Name>
  <Title>grid 10 degrees</Title>
  <Grid resolution="10"/>
  <WMSFormat name="image/png32"/>
</Layer>
```

Cascaded Layers
---------------

Cascaded layers are copies of Layers from other services. This mostly
used to add an external background or baselayer map to the service. The
cascaded layer can be called like any normal Layer in the GetMap
request. Combinations of Layers can be requested in the GetMap URL to
compose nice maps.

```xml
<Layer type="cascaded" hidden="false">
  <Group value="baselayers"/>
  <Name force="true">npsnaturalearth2</Name>
  <Title>NPS - Natural Earth II</Title>
  <WMSLayer service="http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?" layer="naturalearth2"/>
  <LatLonBox minx="-180" miny="-90" maxx="180" maxy="90"/>
</Layer>
```

Liveupdate layer
---------------

This layer type displays a GetMap image with the current time per second for the last hour.

```xml
<?xml version="1.0" encoding="UTF-8" ?>
<Configuration>
  <!--
  See https://dev.knmi.nl/projects/adagucserver/wiki/Dataset, for details
  This file can be included by using the adaguc.dataset.cgi?service=wms&DATASET=testdata& key value pair in the URL
  -->
  <Layer type="liveupdate">
    <Name>liveupdate</Name>
  </Layer>
</Configuration>
```

<img src='2024-02-14-liveupdate-layer.png' />
