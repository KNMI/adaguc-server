Style a Layer with alpha transparency for cloudcover
====================================================

[Back to readme](./Readme.md)

-   Adjust the WMSFormat in WMS to output 32bit PNG images (use image/webp or image/png32), as this will result in high quality opacity.

```xml
<WMSFormat name="image/png" format="image/webp"/>
```

This is the legend for cloudcover with alpha transparency

```xml
<Legend name="cloudcover" type="colorRange">
  <palette index="0" red="255" green="255" blue="255" alpha="0"/>
  <palette index="240" red="255" green="255" blue="255" alpha="255"/>
</Legend>
```

This is the style for cloudcover

```xml
<Style name="cloudcover" title="Alpha transparency" abstract="Visualize radar images with alpha transparency on a log scale">
  <Legend fixedclasses="true" tickround=".1">cloudcover</Legend>
  <Min>0.0</Min>
  <Max>8</Max>
  <ContourLine width="1" linecolor="#444444" textcolor="#444444" textformatting="%2.1f" classes="3"/>
  <ContourLine width="3" linecolor="#0000FF" textcolor="#444444" textformatting="%2.1f" classes="4"/>
  <ShadeInterval min="-1" max="2" label="<2" fillcolor="#E6E6FF"/>
  <ShadeInterval min="2" max="4" label="2-4" fillcolor="#B3B3FF"/>
  <ShadeInterval min="4" max="6" label="4-6" fillcolor="#8080FF"/>
  <ShadeInterval min="6" max="8" label=">6" fillcolor="#4C4CFF"/>
  <RenderMethod>nearest,bilinear,contour,contourbilinear,shaded</RenderMethod>
</Style>
```

This is is the layer definition with cloudcover

```xml  
<Layer type="database">
  <FilePath filter=".*\.nc$">http://opendap.nmdc.eu/knmi/thredds/dodsC/essence/run021/2D_daymean/hih_cld_021_2026-2100.nc</FilePath>
  <Variable>hih_cld</Variable>
  <Styles>cloudcover</Styles>
  <Dimension name="time" interval="P1D">time</Dimension>
  <DataPostProc algorithm="ax+b" a="8" b="0" units="octa"/>
</Layer>
```

TODO: Correct non-working link