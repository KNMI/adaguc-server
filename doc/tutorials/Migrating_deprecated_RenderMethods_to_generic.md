Migrating deprecated RenderMethods to `generic` and `IncludeStyle`
====================================================================

[Back to tutorials](./Readme.md)

Older adaguc-server configurations often set `<RenderMethod>` to one of
`contour`, `shaded`, `contourshaded` (or related combinations such as
`shadedcontour`, `bilinearcontour`, `barbshaded`), or to a comma-separated
list such as `nearest,bilinear,contour,contournearest,contourshaded`. All of
this is deprecated. The current renderer is `generic`, which is also the
default when `<RenderMethod>` is omitted entirely. What used to be selected
by keyword is now selected by which style elements you add (`<ContourLine>`,
`<ShadeInterval>`, `<Vector>`, ...), and styles that used to be duplicated per
render method are now composed with `<IncludeStyle>`.

This tutorial shows the old and new configuration side by side, using real
examples from this repository's test configs, and explains the one behaviour
change (WMS style naming) that you need to be aware of when migrating.

Why this changed
-----------------

With the old `bilinear`/bilinear-based renderer, drawing contour lines or
shading required explicitly telling the renderer to do so via the
`RenderMethod` keyword, because contour/shading support was bolted onto a
small set of fixed render paths. The `generic` renderer collapses nearest,
bilinear, contour and shaded rendering into one engine and turns features on
based on configuration rather than keywords:

-   Contour lines are drawn whenever the style has one or more
    `<ContourLine>` elements — the `contour` keyword is no longer needed.
-   Shading is drawn whenever the style has one or more `<ShadeInterval>`
    elements — the `shaded` keyword is no longer needed.
-   Interpolation is chosen with
    `<RenderSettings interpolationmethod="nearest|bilinear"/>` instead of the
    `nearest`/`bilinear` keywords.
-   Wind barbs/vectors are chosen with one or more `<Vector>` elements
    instead of the `barb`/`vector` keyword.

Because of this, a style's `<RenderMethod>` should simply be `generic`, or
omitted altogether (see [RenderMethod](../configuration/RenderMethod.md)).

1. Contour lines
----------------

Before (`contour` keyword, from
[`adaguc.tests.dashedcontourlines.xml`](../../data/config/datasets/adaguc.tests.dashedcontourlines.xml)):

```xml
<Style name="testdata_style_manycontours">
    <ContourLine width="2" linecolor="#0000EE" textcolor="#FFFFFF" textstrokecolor="#000000" textsize="20.0" textformatting="%2.2f" classes="0.15" dashing="21,7"/>
    <ContourLine width="3" linecolor="#FF0000" textcolor="#808080" textstrokecolor="#FF0000" textsize="30.0" textformatting="%2.1f" classes="0.10"/>
    <ContourLine width="5" linecolor="#00FF00" textcolor="#00FF00" textstrokecolor="#000000" textsize="40.0" textformatting="%2.2f" classes="0.20" dashing="5,5"/>
    <RenderMethod>contour</RenderMethod>
</Style>
```

After: drop `<RenderMethod>contour</RenderMethod>` (or replace it with
`generic`). The `<ContourLine>` elements alone are what draws the contours:

```xml
<Style name="testdata_style_manycontours">
    <ContourLine width="2" linecolor="#0000EE" textcolor="#FFFFFF" textstrokecolor="#000000" textsize="20.0" textformatting="%2.2f" classes="0.15" dashing="21,7"/>
    <ContourLine width="3" linecolor="#FF0000" textcolor="#808080" textstrokecolor="#FF0000" textsize="30.0" textformatting="%2.1f" classes="0.10"/>
    <ContourLine width="5" linecolor="#00FF00" textcolor="#00FF00" textstrokecolor="#000000" textsize="40.0" textformatting="%2.2f" classes="0.20" dashing="5,5"/>
</Style>
```

2. Shading, and shading + contours combined
--------------------------------------------

Before (comma-separated list including `contour` and `contourshaded`, from
[`Styling_of_Layers_with_shading_and_contourlines.md`](./Styling_of_Layers_with_shading_and_contourlines.md)):

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

After: one style, `generic` (or omitted), both `<ContourLine>` and
`<ShadeInterval>` are active automatically because they are present:

```xml
<Style name="clt" title="clt colors" abstract="Drawing images with clt colors">
  <Legend fixedclasses="true" tickinterval="500" tickround="1">rainbow</Legend>
  <Min>0.0</Min>
  <Max>100</Max>
  <ContourLine width="1.0" linecolor="#444444" textcolor="#444444" textformatting="%2.0f" classes="0,25,50,75,100"/>
  <ShadeInterval min="90" max="100" label="90-100" fillcolor="#00E6FF"/>
</Style>
```

3. Building variants with `IncludeStyle` instead of comma-separated lists
---------------------------------------------------------------------------

The old comma-separated `RenderMethod` list wasn't only about turning on
contour/shading — it also had a second job: fanning one `<Style>` block out
into several WMS styles, one per render method (e.g. a nearest-neighbour
version and a bilinear version of the same legend). The replacement for that
*fan-out* is `<IncludeStyle name="...">`: define one small base style and let
other styles pull it in and add or override elements.

Real example, from
[`adaguc.tests.ahn_utrechtse_heuvelrug_500m.xml`](../../data/config/datasets/adaguc.tests.ahn_utrechtse_heuvelrug_500m.xml).
A base style with only the color classes:

```xml
<Style name="style_shaded_elevation">
    <ShadeInterval min="-80" max="-7" fillcolor="#08387b" bgcolor="#FF000080" />
    <ShadeInterval min="-7" max="-6" fillcolor="#084584" />
    <!-- ... more ShadeInterval classes ... -->
</Style>
```

Two variants of it, differing only in interpolation method, built with
`IncludeStyle` instead of duplicating the `ShadeInterval` list per render
method:

```xml
<Style name="style_shaded_nearest_elevation" title="Elevation - discrete classes - nearest" abstract="Elevation with defined classes and colors with nearest neighbour interpolation">
    <IncludeStyle name="style_shaded_elevation"/>
    <RenderSettings interpolationmethod="nearest"/>
</Style>

<Style name="style_shaded_bilinear_elevation" title="Elevation - discrete classes - bilinear" abstract="Elevation with defined classes and colors with bilinear interpolation">
    <IncludeStyle name="style_shaded_elevation"/>
    <RenderSettings interpolationmethod="bilinear"/>
</Style>
```

And a variant that adds contour lines on top of an existing colormapped
style, again via `IncludeStyle` rather than adding `contour` to a
`RenderMethod` list:

```xml
<Style name="style_colormapped_bilinear_multicolor" title="Multicolor legend - bilinear" abstract="Plotted with a multicolor colormap and bilinear interpolation">
    <Legend fixed="true" tickinterval="5">multicolor</Legend>
    <Min>-5</Min>
    <Max>115</Max>
    <RenderSettings interpolationmethod="bilinear"/>
</Style>

<Style name="style_colormapped_bilinear_multicolor_contours" title="Multicolor legend - and contouring" abstract="Plotted with a multicolor colormap and bilinear interpolation with contour lines at every 10">
    <IncludeStyle name="style_colormapped_bilinear_multicolor"/>
    <SmoothingFilter>2</SmoothingFilter>
    <ContourLine width="1.8" linecolor="#000000" textcolor="#000000" textformatting="" interval="10"/>
</Style>
```

`IncludeStyle` is resolved recursively, and can itself be included further
(`style_colormapped_bilinear_multicolor_contours_extrasmooth_discretized` in
the same file includes `..._extrasmooth`, which includes
`style_colormapped_bilinear_multicolor`). Included elements are applied
first, then the including style's own elements extend or override them.
None of these styles need a `<RenderMethod>` at all — `generic` is the
default.

4. Wind barbs
-------------

Before (comma-separated `barbshaded,barbshadedcontour,vectorshaded,vectorshadedcontour,barb`,
from [`adaguc.tests.harm_windbarbs.xml`](../../data/config/datasets/adaguc.tests.harm_windbarbs.xml)):

```xml
<Style name="windbarbs_kts" title="Wind barbs + filled wind speed contours" abstract="Rendered with barbs">
    <IncludeStyle name="wind_colors_from_kts"/>
    <ContourLine width="1" linecolor="#444444" textcolor="#444444" textformatting="%2.0f" classes="7,11,16,21,27,34,41,48,56,64,74,84,100,200"/>
    <RenderMethod>barbshaded,barbshadedcontour,vectorshaded,vectorshadedcontour,barb</RenderMethod>
    <StandardNames standard_name="x_wind,y_wind"/>
</Style>
```

After: use `generic`, with one or more `<Vector>` elements to describe how
barbs should look — optionally with a different style per wind-speed range —
plus `<DataPostProc algorithm="pointsfromgrid">` to sample the grid into
points for the point/vector renderer, from the same file:

```xml
<Style name="windbarbs_kts_shaded_withbarbs_for_grids">
  <IncludeStyle name="wind_colors_from_ms"/>

  <Vector min="0" max="1"       vectorstyle="barb" linewidth="2" linecolor="#FFFFFF" plotvalue="false" outlinewidth="1.6" outlinecolor="#000000" />
  <Vector min="1" max="3.4"     vectorstyle="barb" linewidth="0" linecolor="#000000" plotvalue="true" />
  <Vector min="3.4" max="8"     vectorstyle="barb" linewidth="3" linecolor="#FFFF00" plotvalue="true" />
  <Vector min="8" max="10.8"    vectorstyle="barb" linewidth="1.5" outlinewidth="4" fontsize="15" outlinecolor="#000000" linecolor="#FFFFFF" plotvalue="true" textcolor="#FFFFFF" />
  <Vector min="10.8" max="13.9" vectorstyle="barb" linewidth="0" linecolor="#00FF00" plotvalue="true" textcolor="#111111"/>
  <Vector min="13.9"            vectorstyle="barb" linewidth="0" linecolor="#00FF00" plotvalue="true" textcolor="#111111"/>

  <RenderSettings renderhint="discreteclasses" />
  <!-- Samples points from the grid for the point/vector renderer; `a` thins the result so points are no closer than `a` pixels apart -->
  <DataPostProc algorithm="pointsfromgrid" select="speed_component,direction_component" a="50"/>
  <RenderMethod>generic</RenderMethod>
</Style>
```

Per-range `<Vector>` elements (`min`/`max`) are new functionality that the
old `barb` keyword did not have: the old style drew every barb the same way,
while the new style can vary color, line width and whether the numeric value
is plotted, by wind-speed bucket. See
[Vector](../configuration/Vector.md) for the full list of `<Vector>`
attributes.

5. Watch out for: WMS style names change
-----------------------------------------

This is the one behavioural difference to check for before migrating a
production layer. With the old, deprecated path, a style with more than one
render method (or a single non-whitelisted render method) is expanded into
one WMS style per render method, named `"<style>/<rendermethod>"` — for
example `examplestyle1/shadedcontour`. You can see this convention used
deliberately elsewhere, e.g.
`<AdditionalLayer ... style="style_hillshaded_transparent/hillshaded">` in
the AHN example above.

Once a style only uses `generic` (or no `RenderMethod` at all), it is no
longer expanded: the WMS style name is simply the style's own `name`
attribute, with no `/rendermethod` suffix. If any client (a saved
`GetMap`/`GetLegendGraphic` URL, a viewer configuration, a bookmark) refers
to the old composed name such as `mystyle/shadedcontour`, it needs to be
updated to just `mystyle` once you migrate that style to `generic`.

6. Checking your configuration for deprecated usage
-----------------------------------------------------

adaguc-server has a built-in lint pass that flags deprecated `RenderMethod`
usage (and a few other deprecated constructs) without needing to render
anything. Run it with:

```sh
adagucserver --lint --config yourdataset.xml
```

This reports, among other checks:

-   `multiple rendermethods in one style or layer is deprecated: [...]` —
    when a style or layer has a comma-separated `RenderMethod` list.
-   `rendermethod "..." is deprecated` — for any individual render-method
    keyword other than `generic`, `nearest`, `rgba`, `polyline` or `point`
    (this includes `contour`, `shaded`, `contourshaded`, `bilinear`, `barb`,
    `vector`, and their combinations).
-   `Deprecated to have multiple legends in one style` — when multiple
    `<Legend>` entries are combined with multiple render methods.
-   `The <NameMapping> element inside a <Style> is deprecated and will be
    ignored. Move 'title' and 'abstract' to attributes of the <Style>
    element.` — a related, separately deprecated construct: move
    per-rendermethod `title`/`abstract` overrides to the `title=`/`abstract=`
    attributes on `<Style>` itself.

Quick reference
----------------

| Old `RenderMethod` keyword(s) | What it did | New equivalent |
|---|---|---|
| `contour`, `nearestcontour`, `bilinearcontour` | Draw contour lines | `generic` (or omit) + keep `<ContourLine>` — contouring is automatic once `<ContourLine>` is present |
| `shaded`, `contourshaded`, `shadedcontour` | Fill discrete classes with color, optionally with contours | `generic` (or omit) + `<ShadeInterval>` (+ `<ContourLine>` if combined) — shading is automatic once `<ShadeInterval>` is present |
| `bilinear` / `nearest` as interpolation selector | Chose the sampling method | `<RenderSettings interpolationmethod="bilinear"/>` or `"nearest"` |
| `barb`, `vector`, `barbshaded`, `vectorshadedcontour`, ... | Drew wind barbs/vectors with one fixed look | `generic` + one or more `<Vector>` elements (optionally range-scoped with `min`/`max`) + `<DataPostProc algorithm="pointsfromgrid">` when drawing from a grid |
| Comma-separated list producing several WMS styles (`style/bilinear`, `style/shadedcontour`, ...) | Fan one `<Style>` out into several named variants | Several `<Style>` blocks, each with `<IncludeStyle name="base"/>` plus the one differing element, each using `generic` |
| `<NameMapping>` inside `<Style>` | Per-rendermethod title/abstract override | `title=` / `abstract=` attributes directly on `<Style>` |

See also [RenderMethod](../configuration/RenderMethod.md),
[Style](../configuration/Style.md), [Vector](../configuration/Vector.md) and
[Configuration](../configuration/Configuration.md) for the full attribute
reference.
