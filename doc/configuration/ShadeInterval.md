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

```xml
<ShadeInterval min="0.05" max="0.25" label="0.05-0.25" fillcolor="#E6E6FF"/>
<ShadeInterval min="0.25" max="0.50" label="0.25-0.5" fillcolor="#B3B3FF"/>
<ShadeInterval min="0.50" max="0.75" label="0.50-0.75" fillcolor="#8080FF"/>
<ShadeInterval min="0.75" max="1.00" label="0.75-1.00" fillcolor="#4C4CFF"/>
```
