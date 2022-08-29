FeatureInterval (match, matchid, label, bgcolor, fillcolor)
===========================================================

-   match - Required, The regular expression to match with the attribute
    value
-   matchid - Required, The attribute name to match with. All attributes
    per feature can be queried with GetFeatureInfo (click on the map in
    ADAGUCViewer).
-   label - Recommended, the label to display inside the legend
-   bgcolor - Optional, the background color for the map, can only be
    configured in the first FeatureInterval
-   fillcolor - Required, the color to shade.

```
<Style name="countries_nlmask">
<Legend fixed="true">bluewhitered</Legend>
<FeatureInterval match=".\*" matchid="abbrev" bgcolor="\#CCCCFF"
fillcolor="\#CCFFCCFF" label="Other"/>
<FeatureInterval match="NLD.\*" matchid="adm0_a3"
fillcolor="\#DFFFDF00" label="The Netherlands"/>
<FeatureInterval match="\^Luxembourg\$" matchid="brk_name"
fillcolor="\#0000FF" label="Luxembourg"/>
<FeatureInterval match="\^Asia\$" matchid="continent"
fillcolor="\#808080" label="Asia"/>
<FeatureInterval match="\^India\$" matchid="abbrev"
fillcolor="\#80FF80" label="India"/>
<NameMapping name="nearest" title="Mask NL"/>
<RenderMethod>nearest</RenderMethod>
</Style>

<Layer type="database">
<Title>Countries</Title>
<Name>countries</Name>
<!-- Data obtained from https://geojson-maps.kyd.com.au/ -->
<FilePath
filter="">{ADAGUC_PATH}data/datasets/countries.geojson</FilePath>
<Variable>features</Variable>
<Styles>countries_nlmask</Styles>
</Layer>
```

-   An example configuration is available here:
    -   https://github.com/KNMI/adaguc-server/blob/master/data/config/adaguc.geojson.xml

<!-- -->

-   Can be used with the following GeoJSON:
    -   https://github.com/KNMI/adaguc-server/blob/master/data/datasets/countries.geojson

![](ADAGUC_GeoJSON_MASKED.png)
In this image, the Netherlands is transparent and can be used as a
visual mask overlay.
