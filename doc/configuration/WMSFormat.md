WMSFormat (name, format)
========================

Back to [Configuration](./Configuration.md)

Can be used to translate incoming requests for image/png to image/png32
internally for higher quality images. Name is the incoming name, format
is the internally used format.

Multiple WMSFormats can be listed.

For example:
```xml
<WMSFormat name="image/png" format="image/png32"/>
```
