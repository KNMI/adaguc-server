Styling of Layers with shading and contourlines
===============================================

The following configuration provides several ways for styling a layer
using shading and contourlines.

```
<Style name="clt">
<Legend fixedclasses="true" tickinterval="500"
tickround="1">rainbow</Legend>
<Min>0.0</Min>
<Max>100</Max>
<RenderMethod>nearest,bilinear,contour,contournearest,contourshaded</RenderMethod>
<NameMapping name="nearest" title="clt colors" abstract="Drawing
images with clt colors"/>
<NameMapping name="bilinear" title="clt colors smooth"
abstract="Drawing images with clt colors and bilinear
interpolation"/>
<NameMapping name="contour" title="clt contour line"
abstract="Drawing images with clt contour and bilinear
interpolation"/>
<ContourLine width="1.0" linecolor="\#444444" textcolor="\#444444"
textformatting="%2.0f" classes="0,25,50,75,100"/>
<ShadeInterval min="90" max="100" label="90-100" fillcolor="\#00E6FF"
/>
</Style>
```

Use the following settings in the WMS element to get antialiased smooth
contourlines:
```
<WMSFormat name="image/png" format="image/png32"/>
```

For details please see [Configuration](Configuration.md)
