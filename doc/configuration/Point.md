Point(min,max,pointstyle,fillcolor,linecolor,textcolor,textformat,fontfile,fontsize,discradius,textradius,dot,anglestart,anglestop,plotstationid)
=================================================================================================================================================

Back to [Configuration](./Configuration.md)

Configuration of rendering point data
-------------------------------------

The Point element is defines the rendering of point data. A Layer with
point data can contain one or more NetCDF variables (identified by
Variable elements in the configuration file). If there are exactly two
Variable elements the layer is assumed to contain wind speed and
direction (see [Vector](Vector.md)).
There are many rendering possibilities for point data.

The **pointstyle** attribute defines the style of point rendering and
can have one of the values "point", "disc", "volume", "symbol" or
"zoomablepoint".
Point data can be rendered:

-   as a text on the map on or around the station's location (single and
    multiple Variables) "point"
-   as text on a coloured disc (one Variable): "disc"
-   as a kind of fuzzy cloud: "volume"
-   as a symbol determined by the point value and a set of
    SymbolInterval definitions: "symbol"

The **plotstationid** attribute (true/false) defines if the stationid is
plotted together with the data.

Appearance of a text is determined by the **textcolor**, **fontfile**
and **fontsize** attributes. The numeric value is formatted into a text
by the printf-style format string of the **textformat** attribute. Tip,
no text is rendered when textformat is set to a blank space (" ").

### Pointstyle point

```xml
  <Point fillcolor="" linecolor="" textcolor="" fontfile="" fontsize="" discradius="5" textradius="" textformat="%f" dot="false" anglestart="" anglestep="" plotstationid="true" pointstyle="point"></Point>
  <RenderMethod>point</RenderMethod>
```
This pointstyle draws a circle at the station location, coloured
according to the value of the first variable of the layer and the
Legend. If **discradius** is defined to be larger than 0 a circle is
drawn at the station's location in a color determined by the data value
and the Style's Legend.If **discradius** is 0 then the data's value is
plotted centered at the station's location.
The attribute **linecolor** specifies a color for the edge of the
circle. Making the **linecolor** transparent (linecolor="0x00000000")
draws no line around the circle.
The value can be plotted next to the point. When 2 or more Variables are
defined in a layer, the values get plotted in a circle around the
station's location. The values are plotted starting at the angle defined
by the **anglestart** attribute and are **anglestep** degrees apart. The
distance of the text from the station's location is defined by the
**textradius** attribute.
If a **fillcolor** attribute is specified the circle is drawn in the
specified fixed color. A circle will be drawn around the disc in the
**linecolor**.
The attribute **dot** plots a dot at the station's location; this can be
useful during testing of the configuration of a point data style.

Single variable layer example: ![](point1.png)

Multiple variable layer example: ![](point3.png)

Example:

### Pointstyle disc

Pointstyle disc can only handle 1 variable in a layer or 2 in case of
wind data (direction/speed).
The attribute **discradius** defines the size of the disc on which the
value text is drawn, **fillcolor** defines the color of the disc (this
color can contain transparency).

The attributes **anglestart**, **anglestep**, **linecolor**,
**plotstationid** and **textradius** have no meaning here.

Example with temperature data:
![](disc.png)

### Pointstyle volume

Pointstyle volume shows a fuzzy disc at the station's location. This
style is most useful for events that can occur often and overlap
(because the rendered discs are translucent).
The base color of the disc is defined by the **fillcolor** attribute.
A station id is plotted if the **plotstationid** attribute has the value
true.

The attributes **anglestart**, **anglestep**, **linecolor** and
**textradius** have no meaning here.

Example:
![](volume.png)

### Pointstyle symbol

When set to "symbol", it enables the [SymbolInterval](SymbolInterval.md) to draw
Symbols/Icons on the map

### Pointstyle zoomablepoint

When set to "zoomablepoint", the point keeps the same size across
zooming and reprojections. This is used to plot IASI satellite imagery:

```xml
  <Style name="IASI">
    <Legend fixed="true" tickinterval=".1">temperature</Legend>
    <Min>0</Min>
    <Max>1</Max> 
    <NameMapping name="point"        title="IASI" abstract="IASI"/>
    <Point plotstationid="false" pointstyle="zoomablepoint" textformat=" " discradius="10" textradius="0" dot="false" fontsize="8" textcolor="#000000" />
    <RenderMethod>point</RenderMethod>
  </Style>

```

![](iasi_adaguc.png)

### Pointstyle radiusandvalue

This can be used to color earthquakes based on age and magnitude. In the
example below the magnitude is the radius of the disc, while the color
depends on age. The radius can be adjusted with the discradius property,
it multiplies the magnitude variable with the discradius value.

```xml
<Style name="magnitude">
    <Legend fixed="true" tickinterval="100000">magnitude</Legend>
    <RenderMethod>point</RenderMethod>
    <Min>0</Min>
    <Max>2000000</Max>
    <NameMapping name="point" title="Richter magnitude scale" abstract="Wth continuous colors" />
    <Point min="2592000" max="1000000000000" pointstyle="radiusandvalue" textformat=" " plotstationid="false" fillcolor="#CCCCCC" discradius="2.2" textradius="0" dot="false" fontsize="14" textcolor="#FFFFFF" />
    <Point min="604800" max="2592000" pointstyle="radiusandvalue" textformat=" " plotstationid="false" fillcolor="#FFFFFFFF" discradius="2.2" textradius="0" dot="false" fontsize="14" textcolor="#FFFFFF" />
    <Point min="86400" max="604800" pointstyle="radiusandvalue" textformat=" " plotstationid="false" fillcolor="#FFFF00FF" discradius="2.2" textradius="0" dot="false" fontsize="14" textcolor="#FFFFFF" />
    <Point min="3600" max="86400" pointstyle="radiusandvalue" textformat=" " plotstationid="false" fillcolor="#FF9900FF" discradius="2.2" textradius="0" dot="false" fontsize="14" textcolor="#FFFFFF" />
    <Point min="0" max="3600" pointstyle="radiusandvalue" textformat=" " plotstationid="false" fillcolor="#FF0000" discradius="2.2" textradius="0" dot="false" fontsize="14" textcolor="#FFFFFF" />
    <LegendGraphic value="{ADAGUC_DATASET_DIR}/csv_radiusandvalue_legend.png" />
  </Style>
  <Layer type="database">
    <Name>radiusandvalue</Name>
    <Title>magnitude</Title>
    <FilePath filter="">{ADAGUC_PATH}/data/datasets/test/csv_radiusandvalue.csv</FilePath>
    <Variable>age</Variable>
    <Variable>magnitude</Variable>
    <Styles>magnitude</Styles>
    <DataBaseTable>ok</DataBaseTable>
  </Layer>
```

![](pointstyle_radiusandvalue.png)
