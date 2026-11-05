Migrating deprecated RenderMethods to `generic`
===============================================

[Back to readme](./Readme.md)

The keywords `bilinear`, `shaded`, `contour`, `vector`, `barb`, `thin` and their combinations
(`shadedcontour`, `barbshaded`, ...) are deprecated, as are comma-separated lists like `nearest,bilinear,contour`.
The old renderer `CImgWarpBilinear` is gone. The generic warper emulates the keywords, so old configs keep working.
New configs use `<RenderMethod>generic</RenderMethod>`, or omit it, and enable features by adding elements.

| Old keyword | New |
|---|---|
| `contour`, `nearestcontour`, `bilinearcontour` | `<ContourLine>` elements. Add `<RenderSettings drawgrid="false"/>` to not paint the field. |
| `shaded`, `shadedcontour` | `<ShadeInterval>` and `<ContourLine>` elements |
| `bilinear`, `nearest` | `<RenderSettings interpolationmethod="bilinear"/>` or `"nearest"` |
| `barb`, `vector`, `barbshaded`, ... | `<Vector>`, `<Thinning>` and `<DataPostProc algorithm="pointsfromgrid">`, see below |
| A list of keywords that gives several styles | Several styles with `<IncludeStyle>`, see below |
| `<NameMapping>` in a style | `title` and `abstract` attributes of `<Style>` |

Test configs and `TestsForLegacyBilinearRender` (dataset [adaguc.tests.legacybilinearrender.xml](../../data/config/datasets/adaguc.tests.legacybilinearrender.xml)) show every legacy keyword.

Scalar styles
-------------

Before:

```xml
<Style name="clt">
  <Legend fixedclasses="true" tickinterval="500" tickround="1">rainbow</Legend>
  <Min>0.0</Min>
  <Max>100</Max>
  <RenderMethod>nearest,bilinear,contour,contournearest,contourshaded</RenderMethod>
  <ContourLine width="1.0" linecolor="#444444" textcolor="#444444" textformatting="%2.0f" classes="0,25,50,75,100"/>
  <ShadeInterval min="90" max="100" label="90-100" fillcolor="#00E6FF"/>
</Style>
```

After:

```xml
<Style name="clt">
  <Legend fixedclasses="true" tickinterval="500" tickround="1">rainbow</Legend>
  <Min>0.0</Min>
  <Max>100</Max>
  <RenderSettings interpolationmethod="bilinear"/>
  <ContourLine width="1.0" linecolor="#444444" textcolor="#444444" textformatting="%2.0f" classes="0,25,50,75,100"/>
  <ShadeInterval min="90" max="100" label="90-100" fillcolor="#00E6FF"/>
</Style>
```

Contours and shading are drawn when the style has `<ContourLine>` and `<ShadeInterval>` elements.

Variants with `IncludeStyle`
----------------------------

A keyword list used to give one WMS style per keyword. Now write one style per variant and share the common part:

```xml
<Style name="elevation">
  <Legend fixed="true" tickinterval="5">multicolor</Legend>
  <Min>-5</Min>
  <Max>115</Max>
  <RenderSettings interpolationmethod="bilinear"/>
</Style>

<Style name="elevation_contours">
  <IncludeStyle name="elevation"/>
  <ContourLine width="1.8" linecolor="#000000" textformatting="" interval="10"/>
</Style>
```

Included styles are applied first, then the elements of the including style extend or override them.
Full example: [adaguc.tests.ahn_utrechtse_heuvelrug_500m.xml](../../data/config/datasets/adaguc.tests.ahn_utrechtse_heuvelrug_500m.xml).

Wind barbs and vectors
----------------------

Before:

```xml
<Style name="windbarbs_kts">
  <IncludeStyle name="wind_colors_from_kts"/>
  <ContourLine width="1" linecolor="#444444" textformatting="%2.0f" classes="7,11,16,21,27,34,41,48,56,64,74,84,100,200"/>
  <RenderMethod>barbshadedcontour</RenderMethod>
</Style>
```

After:

```xml
<Style name="windbarbs_kts">
  <IncludeStyle name="wind_colors_from_kts"/>
  <ContourLine width="1" linecolor="#444444" textformatting="%2.0f" classes="7,11,16,21,27,34,41,48,56,64,74,84,100,200"/>
  <Vector vectorstyle="barb" linecolor="#0000FF" outlinewidth="0"/>
  <Thinning radius="40"/>
  <DataPostProc algorithm="pointsfromgrid" select="speed_component,direction_component"/>
  <RenderMethod>generic</RenderMethod>
</Style>
```

The layer needs `<DataPostProc algorithm="convert_uv_components"/>` as before.

- `<Vector>` sets the look. Use several with `min` and `max` for a look per wind speed range, see [Vector.md](../configuration/Vector.md).
- `<Thinning radius="40"/>` drops points closer than 40 pixels, see [Thinning.md](../configuration/Thinning.md).
- `select` is the speed object first, then the direction object.
- Barbs without a field underneath: add `<RenderSettings drawgrid="false"/>`.

Examples: [adaguc.tests.harm_windbarbs.xml](../../data/config/datasets/adaguc.tests.harm_windbarbs.xml).

<img border="1" src="../../tests/expectedoutputs/TestWMS/test_WMSGetMapWithHarmShadedWindSpeed.png" width="400">

A legacy `barb` or `vector` style without `<Vector>`, `<Thinning>` or `pointsfromgrid` gets these defaults added at render time.

WMS style names change
----------------------

A style with only `generic`, or no `RenderMethod`, is advertised under its own name: `mystyle`.
A style with a keyword list is advertised as `mystyle/keyword` per keyword.
Update saved GetMap URLs and viewer configs when you migrate: `mystyle/shadedcontour` becomes `mystyle`.

Check for deprecated use
------------------------

```sh
adagucserver --lint --config yourdataset.xml
```

It reports keyword lists, deprecated keywords, several legends per style and `<NameMapping>`.

Known differences
-----------------

- Shading and contour edges are not pixel identical. Very dense contour levels can look slightly more faceted.
- Barb and vector density differs on coarse grids. The old renderer placed symbols per destination pixel, `pointsfromgrid` places them per source cell. Tune `Thinning` and the `a` attribute of `pointsfromgrid`.
- `SmoothingFilter` gives different values. The old filter weighted neighbours by distance, the generic filter averages them. Legend colour boundaries and contour positions can shift slightly.

See also [RenderMethod.md](../configuration/RenderMethod.md), [RenderSettings.md](../configuration/RenderSettings.md), [ShadeInterval.md](../configuration/ShadeInterval.md), [ContourLine.md](../configuration/ContourLine.md).
