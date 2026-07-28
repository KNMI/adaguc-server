ShadeInterval (min,max,label,bgcolor,fillcolor)
===============================================

Back to [Configuration](./Configuration.md)

-   min - Value to shade from
-   max - Value to shade to
-   label - Optional, the label to display inside the legend
-   bgcolor - Optional, the background color for the map, can only be
    configured in the first Shadeinterval
-   fillcolor - Optional, the color to shade, the color picked from the
    corresponding [Legend](Legend.md). If the color does not occur in the
    legend, the nearest color is chosen. If not defined, the color is
    automatically picked from the legend.
-   fillcolor2 - Optional, a second color for this interval. When set, the
    interval is drawn in the legend as a smooth from fillcolor (at
    min) to fillcolor2 (at max), instead of a single flat color.

```xml
<ShadeInterval min="0.05" max="0.25" label="0.05-0.25" fillcolor="#E6E6FF"/>
<ShadeInterval min="0.25" max="0.50" label="0.25-0.5" fillcolor="#B3B3FF"/>
<ShadeInterval min="0.50" max="0.75" label="0.50-0.75" fillcolor="#8080FF"/>
<ShadeInterval min="0.75" max="1.00" label="0.75-1.00" fillcolor="#4C4CFF"/>
```

### Gradients and grouped legends

If one or more ShadeIntervals in a Style set fillcolor2, the legend for
that Style switches to a grouped layout: every ShadeInterval is drawn as
one equal-height band, regardless of how wide its min-max range is, and
only the boundary values are labelled. This is intended for a small set 
of intervals rather than the long, fine-grained lists sometimes used to
approximate a smooth color scale.

If no ShadeInterval in a Style sets fillcolor2, the interval is rendered
as a solid color, which is the default behaviour.


```xml
<ShadeInterval min="0.2"   max="0.4"   fillcolor="#4A4A4A"/>
<ShadeInterval min="0.4"   max="0.7"   fillcolor="#B8B8B8"/>
<ShadeInterval min="0.7"   max="1.5"   fillcolor="#23BA46" fillcolor2="#058501"/>
<ShadeInterval min="1.5"   max="10.0"  fillcolor="#FB2600" fillcolor2="#912B14"/>
<ShadeInterval min="10.0"  max="20.0"  fillcolor="#C198B3" fillcolor2="#C8106A"/>
<ShadeInterval min="20.0"  max="35.0"  fillcolor="#A502D7" fillcolor2="#05003E"/>
<ShadeInterval min="35.0"  max="70.0"  fillcolor="#87FFF9" fillcolor2="#245368"/>
<ShadeInterval min="70.0"  max="90.0"  fillcolor="#690004"/>
<ShadeInterval min="90.0"  max="120.0" fillcolor="#050000"/>
<ShadeInterval min="120.0" max="400.0" fillcolor="#ffffff"/>
```

In this example, the 0.7-1.5, 1.5-10.0, 10.0-20.0, 20.0-35.0 and 35.0-70.0
bands render as gradients; the rest render as flat colors, same as
before.