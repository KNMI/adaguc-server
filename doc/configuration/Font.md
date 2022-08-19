Font (location,size)
====================

Configures the font for various elements in the service.

TitleFont is used when the &title="The title" string is added to the
GetMap request, SubTitleFont is used when the &subtitle="The subtitle"
is added to the GetMap request. DimensionFont is used to display
dimensions inside the GetMap image, by adding &showdims=true to the
GetMap request. ContourFont is used for display in contourlines,
GridFont is used when displaying a grid layer.

-   location - The location of the TTF font file to use
-   size - The size in pixels of the font

```
<TitleFont location="/data/fonts/verdana.ttf" size="19"/>
<SubTitleFont location="/data/fonts/verdana.ttf" size="10"/>
<DimensionFont location="/data/fonts/verdana.ttf" size="7"/>
<ContourFont location="/data/fonts/verdana.ttf" size="7"/>
<GridFont location="/data/fonts/verdana.ttf" size="5"/>
```
