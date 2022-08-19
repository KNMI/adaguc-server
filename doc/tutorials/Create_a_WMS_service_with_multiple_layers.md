Create a WMS service with multiple layers
=========================================

You can add new layers to your WMS service by adding more [Layer](Layer.md)
elements. For example:

```

<?xml version="1.0" encoding="UTF-8" ?>
<Configuration>

<!-- General settings -->
<Path value="/data/software/adagucserver/data"/>
<TempDir value="/data/tmp/"/>
<OnlineResource
value="http://localhost/cgi-bin/demo/pytroll.cgi?"/>
<DataBase parameters="dbname=adagucdemo host=127.0.0.1 user=adaguc
password=adaguc"/>

<!-- WMS settings -->
<WMS>
<Title>OMI\_Pytroll\_DEMO\_SERVICE</Title>
<Abstract>This service demonstrates how the ADAGUC server can be
used to create OGC services.</Abstract>
<RootLayer>
<Title>ADAGUC Pytroll Demo Service</Title>
<Abstract></Abstract>
</RootLayer>
<TitleFont
location="/data/software/adagucserver/data/fonts/FreeSans.ttf"
size="19"/>
<SubTitleFont
location="/data/software/adagucserver/data/fonts/FreeSans.ttf"
size="10"/>
<DimensionFont
location="/data/software/adagucserver/data/fonts/FreeSans.ttf"
size="7"/>
<ContourFont
location="/data/software/adagucserver/data/fonts/FreeSans.ttf"
size="7"/>
<GridFont
location="/data/software/adagucserver/data/fonts/FreeSans.ttf"
size="5"/>
<WMSFormat name="image/png" format="image/png"/>
</WMS>

<!-- WCS settings -->
<WCS>
<Title>DEMO\_SERVICE</Title>
<Label>wcsLabel</Label>
<WCSFormat name="NetCDF3" driver="ADAGUC"
mimetype="Content-Type:text/plain" options="FORCENC3=TRUE"/>
<WCSFormat name="NetCDF4" driver="ADAGUC"
mimetype="Content-Type:text/plain"/>
</WCS>

<!-- Projections -->
<Projection id="EPSG:3411" proj4="+proj=stere +lat\_0=90 +lat\_ts=70
+lon\_0=-45 +k=1 +x\_0=0 +y\_0=0 +a=6378273 +b=6356889.449 +units=m
+no\_defs"/>
<Projection id="EPSG:3412" proj4="+proj=stere +lat\_0=-90
+lat\_ts=-70 +lon\_0=0 +k=1 +x\_0=0 +y\_0=0 +a=6378273 +b=6356889.449
+units=m +no\_defs"/>
<Projection id="EPSG:3575" proj4="+proj=laea +lat\_0=90 +lon\_0=10
+x\_0=0 +y\_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no\_defs"/>
<Projection id="EPSG:3857" proj4="+proj=merc +a=6378137 +b=6378137
+lat\_ts=0.0 +lon\_0=0.0 +x\_0=0.0 +y\_0=0 +k=1.0 +units=m
+nadgrids=`null +wktext  +no_defs"/>
  <Projection id="EPSG:4258"   proj4="+proj=longlat +ellps=GRS80 +no_defs"/>
  <Projection id="EPSG:4326"   proj4="+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"/>
  <Projection id="EPSG:25831"  proj4="+proj=utm +zone=31 +ellps=GRS80 +units=m +no_defs"/>
  <Projection id="EPSG:25832"  proj4="+proj=utm +zone=32 +ellps=GRS80 +units=m +no_defs"/>
  <Projection id="EPSG:28992"  proj4="+proj=sterea +lat_0=52.15616055555555 +lon_0=5.38763888888889 +k=0.9999079 +x_0=155000 +y_0=463000 +ellps=bessel +units=m +no_defs"/>
  <Projection id="EPSG:32661"  proj4="+proj=stere +lat_0=90 +lat_ts=90 +lon_0=0 +k=0.994 +x_0=2000000 +y_0=2000000 +ellps=WGS84 +datum=WGS84 +units=m +no_defs"/>
  <Projection id="EPSG:40000"  proj4="+proj=stere +ellps=WGS84 +lat_0=90 +lon_0=0 +no_defs"/>
  <Projection id="EPSG:900913" proj4=" +proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=`null
+no\_defs"/>
<Projection id="EPSG:102100" proj4="+proj=merc +a=6378137 +b=6378137
+lat\_ts=0.0 +lon\_0=0.0 +x\_0=0.0 +y\_0=0 +k=1.0 +units=m
+nadgrids=@null +wktext +no\_defs"/>

<!-~~Legends -~~>

<Legend name="rainbow" type="colorRange">
<palette index="0" red="0" green="0" blue="255"/>
<palette index="80" red="0" green="255" blue="255"/>
<palette index="120" red="0" green="255" blue="0"/>
<palette index="160" red="255" green="255" blue="0"/>
<palette index="240" red="255" green="0" blue="0"/>
</Legend>
<!-- Auto style -->
<Style name="auto">
<Legend>rainbow</Legend>
<Scale>0</Scale>
<Offset>0</Offset>
<RenderMethod>nearest,bilinear,nearestpoint</RenderMethod>
</Style>

<!-- Layer configuration -->
<Layer type="database">
<Group value="MSG\_RGB\_EuropeCanary"/>
<FilePath
filter=".\*\\.nc\$">/data/services/data/pytroll/MSG\_RGB\_EuropeCanary/</FilePath>
<Variable>overview</Variable>
<RenderMethod>rgba</RenderMethod>
<Dimension name="time" interval="PT15M">time</Dimension>
</Layer>

<Layer type="database">
<Group value="MSG\_RGB\_EuropeCanary"/>
<FilePath
filter=".\*\\.nc\$">/data/services/data/pytroll/MSG\_RGB\_EuropeCanary/</FilePath>
<Variable>airmass</Variable>
<RenderMethod>rgba</RenderMethod>
<Dimension name="time" interval="PT15M">time</Dimension>
</Layer>

<Layer type="database">
<Group value="MSG\_RGB\_EuropeCanary"/>
<FilePath
filter=".\*\\.nc\$">/data/services/data/pytroll/MSG\_RGB\_EuropeCanary/</FilePath>
<Variable>ash</Variable>
<RenderMethod>rgba</RenderMethod>
<Dimension name="time" interval="PT15M">time</Dimension>
</Layer>

<Layer type="database">
<Group value="MSG\_RGB\_EuropeCanary"/>
<FilePath
filter=".\*\\.nc\$">/data/services/data/pytroll/MSG\_RGB\_EuropeCanary/</FilePath>
<Variable>cloudtop</Variable>
<RenderMethod>rgba</RenderMethod>
<Dimension name="time" interval="PT15M">time</Dimension>
</Layer>

<Layer type="database">
<Group value="MSG\_RGB\_EuropeCanary"/>
<FilePath
filter=".\*\\.nc\$">/data/services/data/pytroll/MSG\_RGB\_EuropeCanary/</FilePath>
<Variable>convection</Variable>
<RenderMethod>rgba</RenderMethod>
<Dimension name="time" interval="PT15M">time</Dimension>
</Layer>

<Layer type="database">
<Group value="MSG\_RGB\_EuropeCanary"/>
<FilePath
filter=".\*\\.nc\$">/data/services/data/pytroll/MSG\_RGB\_EuropeCanary/</FilePath>
<Variable>dust</Variable>
<RenderMethod>rgba</RenderMethod>
<Dimension name="time" interval="PT15M">time</Dimension>
</Layer>

<Layer type="database">
<Group value="MSG\_RGB\_EuropeCanary"/>
<FilePath
filter=".\*\\.nc\$">/data/services/data/pytroll/MSG\_RGB\_EuropeCanary/</FilePath>
<Variable>green\_snow</Variable>
<RenderMethod>rgba</RenderMethod>
<Dimension name="time" interval="PT15M">time</Dimension>
</Layer>

<Layer type="database">
<Group value="MSG\_RGB\_EuropeCanary"/>
<FilePath
filter=".\*\\.nc\$">/data/services/data/pytroll/MSG\_RGB\_EuropeCanary/</FilePath>
<Variable>natural</Variable>
<RenderMethod>rgba</RenderMethod>
<Dimension name="time" interval="PT15M">time</Dimension>
</Layer>

<Layer type="database">
<Group value="MSG\_RGB\_EuropeCanary"/>
<FilePath
filter=".\*\\.nc\$">/data/services/data/pytroll/MSG\_RGB\_EuropeCanary/</FilePath>
<Variable>night\_fog</Variable>
<RenderMethod>rgba</RenderMethod>
<Dimension name="time" interval="PT15M">time</Dimension>
</Layer>

<Layer type="database">
<Group value="MSG\_RGB\_EuropeCanary"/>
<FilePath
filter=".\*\\.nc\$">/data/services/data/pytroll/MSG\_RGB\_EuropeCanary/</FilePath>
<Variable>red\_snow</Variable>
<RenderMethod>rgba</RenderMethod>
<Dimension name="time" interval="PT15M">time</Dimension>
</Layer>

<Layer type="database">
<Group value="noaa"/>
<Name>overview\_neur</Name>
<Title>Overview Northern Europe</Title>
<FilePath>/data/services/data/pytroll/noaa/NOAA19\_RGB\_20121127\_0959\_NEur.nc</FilePath>
<Variable>overview</Variable>
<RenderMethod>rgba</RenderMethod>
</Layer>

<Layer type="database">
<Group value="noaa"/>
<Name>overview\_moll</Name>
<Title>Overview Mollweide</Title>
<FilePath>/data/services/data/pytroll/noaa/NOAA19\_RGB\_20121127\_0959\_moll.nc</FilePath>
<Variable>overview</Variable>
<RenderMethod>rgba</RenderMethod>
</Layer>

<Layer type="database">
<Group value="npp"/>
<Name>overview\_moll</Name>
<Title>Overview Mollweide</Title>
<FilePath>/data/services/data/pytroll/npp/NPP\_RGB\_20121125\_1046\_moll.nc</FilePath>
<Variable>overview</Variable>
<RenderMethod>rgba</RenderMethod>
</Layer>

<!-- End of configuration /-->

</Configuration>

```
