NameMapping(name,title,abstract)
================================

NameMapping is used to assign human readable titles to the [Style](Style.md)
configured in the service. The title is displayed to the user while the
name is used as internal reference.

-   name - The existing style name in the service to attach the title
    and abstract to.
-   title - The title to give to the corresponding name
-   abstract - The abstract to give to the corresponding name

For example:
```
<NameMapping name="nearest" title="Temperature 0-10"
abstract="Nearest neighbour rendering"/>
<NameMapping name="shadedcontour" title="Temperature 0-10 shaded"
abstract="Shaded with contourlines"/>
<NameMapping name="nearestcontour" title="Temperature 0-10 contours"
abstract="Nearest neighbour with contourlines"/>
<NameMapping name="bilinearcontour" title="Temperature 0-10 bilinear"
abstract="Bilinear with contourlines"/>
```
