WMSFormat (name, format, quality)
========================

Back to [Configuration](./Configuration.md)

Can be used to translate incoming requests for image/png to image/png32
internally for higher quality images. 

WMSFormat can be used in the WMS element or within a [Layer](./Layer.md)
 element.

- name: the incoming name in the WMS GetMap request
- format: the internally used format and used as image output. (image/png, image/png32, image/webp)
- quality: The compression quality. Currently webp supports the quality flag. 0 is low, 100 is high.

Multiple WMSFormats can be listed.

For example:
```xml
<WMSFormat name="image/png" format="image/png32"/>

<WMSFormat name="image/webp" quality="70"/>
```
