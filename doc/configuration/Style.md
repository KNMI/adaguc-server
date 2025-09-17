Style (name, abstract, title)
============

Back to [Configuration](./Configuration.md)

Configures a style configuration, which can be assigned to a layer by
using the <Styles> element in that Layer. Styles can also be
assigned automatically to a Layer by configuring one or more
StandardNames element(s). When the Layers standard_name matches with
one of the names configured in a Style, this Style will also be assigned
to that Layer.

When using remote resources with [AutoResource](AutoResource.md), a style with name
"auto" needs to be configured for automatic scaling of min/max values.
This style is assigned to all layers and can be used for previewing the
data with automatically detected min/max values.

-   name, The name of the style to use within the service.
-   abstract, The abstract of the style to use within the service. This overrides the NameMapping elements
-   title, The title of the style to use within the service. This overrides the NameMapping elements

```

<Style name="auto">
<Legend>Tg_woltnhoff</Legend>
<Scale>0</Scale>
<Offset>0</Offset>
<RenderMethod>nearest,bilinear,point</RenderMethod>
</Style>

<Style name="examplestyle1">
<Legend fixedclasses="true" tickinterval="2"
tickround="2">color</Legend>
<ContourLine width="0.8" linecolor="#000000" textcolor="#404040"
textformatting="%2.0f" interval="2"/>

<ShadeInterval min="-10000" max="0" label="<0"
fillcolor="#FFFFFF">0</ShadeInterval>
<ShadeInterval min="0" max="1" label="0 - 1" />
<ShadeInterval min="1" max="2" label="1 - 2" />
<ShadeInterval min="2" max="3" label="2 - 3" />
<ShadeInterval min="3" max="4" label="3 - 4" />
<ShadeInterval min="4" max="5" label="4 - 5" />
<ShadeInterval min="5" max="6" label="5 - 6" />
<ShadeInterval min="6" max="7" label="6 - 7" />
<ShadeInterval min="7" max="8" label="7 - 8" />
<ShadeInterval min="8" max="9" label="8 - 9" />
<ShadeInterval min="9" max="10" label="9 - 10" />
<ShadeInterval min="10" max="10000" label=">10"
fillcolor="#FF00AF"/>
<RenderMethod>nearest,nearestcontour,bilinearcontour,shadedcontour,bilinear</RenderMethod>
<SmoothingFilter>0</SmoothingFilter>
<NameMapping name="nearest" title="Temperature 0-10"
abstract="Nearest neighbour rendering"/>
<NameMapping name="shadedcontour" title="Temperature 0-10 shaded"
abstract="Shaded with contourlines"/>
<NameMapping name="nearestcontour" title="Temperature 0-10 contours"
abstract="Nearest neighbour with contourlines"/>
<NameMapping name="bilinearcontour" title="Temperature 0-10 bilinear"
abstract="Bilinear with contourlines"/>
<StandardNames standard_name="air_temperature"
units="Celsius"/>
<Min>0</Min>
<Max>10</Max>
</Style>

<Layer>
<Styles>examplestyle1,examplestyle2,...</Styles>
...
</Layer>

```

An example style for cloudcover:
```
<Style name="highcloudcover">
<Legend fixedclasses="true" tickround="0.1" tickinterval=".1"
>highcloudcover</Legend>
<Min>0</Min>
<Max>1</Max>
<ShadeInterval min="0.05" max="0.25" label="0.05-0.25"
fillcolor="#E6E6FF"/>
<ShadeInterval min="0.25" max="0.50" label="0.25-0.5"
fillcolor="#B3B3FF"/>
<ShadeInterval min="0.50" max="0.75" label="0.50-0.75"
fillcolor="#8080FF"/>
<ShadeInterval min="0.75" max="1.00" label="0.75-1.00"
fillcolor="#4C4CFF"/>
<RenderMethod>bilinear,nearest,shaded</RenderMethod>
<NameMapping name="nearest" title="High cloud cover"
abstract="Rendered with nearest neighbour interpolation"/>
<NameMapping name="bilinear" title="High cloud cover bilinear"
abstract="Rendered with bilinear interpolation"/>
<NameMapping name="contour" title="High cloud cover contour"
abstract="Contourlines on a transparent background"/>
<NameMapping name="shaded" title="High cloud cover shaded"
abstract="Shaded"/>
<NameMapping name="nearestcontour" title="High cloud cover nearest +
contours" abstract="Contours with nearest neighbour interpolated
background"/>
<NameMapping name="bilinearcontour" title="High cloud cover bilinear
+ contours" abstract="Contours with bilinear interpolated
background"/>
<NameMapping name="shadedcontour" title="High cloud cover shaded +
contours" abstract="Contours with shaded background"/>
<StandardNames standard_name="high_cloud_cover"/>
</Style>
```

See [Predefined Legends](Predefined Legends.md) for some precooked legends for several
physical quantities like temperature, pressure, precipitation, etc..
