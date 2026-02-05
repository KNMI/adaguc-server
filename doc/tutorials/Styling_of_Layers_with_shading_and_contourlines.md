Styling of Layers with shading and contourlines
===============================================

[Back to readme](./Readme.md)

The following configuration provides several ways for styling a layer
using shading and contourlines.

```xml
<Style name="clt" title="clt colors" abstract="Drawing images with clt colors">
  <Legend fixedclasses="true" tickinterval="500" tickround="1">rainbow</Legend>
  <Min>0.0</Min>
  <Max>100</Max>
  <RenderMethod>nearest,bilinear,contour,contournearest,contourshaded</RenderMethod>
  <ContourLine width="1.0" linecolor="#444444" textcolor="#444444" textformatting="%2.0f" classes="0,25,50,75,100"/>
  <ShadeInterval min="90" max="100" label="90-100" fillcolor="#00E6FF"/>
</Style>
```

Use the following settings in the WMS element to get antialiased smooth contourlines:

```xml 
<WMSFormat name="image/png" format="image/png32"/>
```

This enables higher quality opacity settings via the alpha channel, which is needed for high quality isolines.

For details please see [Configuration](../configuration/Configuration.md) and [WMSFormat](../configuration/WMSFormat.md)
