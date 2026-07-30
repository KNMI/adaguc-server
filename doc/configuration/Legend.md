Legend (name, type, file)
=========================

Back to [Configuration](./Configuration.md)

-   name - The name of the legend
-   type - colorRange,interval or file
-   file - The SVG gradient file in case of legend type "file"
-   regularspacing - Set to "true" to render a grouped legend, with every ShadeInterval shown as an equal-height band (see "Grouped legends" below)

There are three types of legends,

1.  legends with gradually changing colors (continuous)
2.  legends with colors within certain intervals (discrete).
3.  legends stored in a SVG gradient file as can be found on
    http://soliton.vm.bytemark.co.uk/pub/cpt-city/views/totp-cpt.html
4.  grouped legends

The first two legend types are configured by palette elements. With the
legend object one can define a maximum of 240 different colors. The
indexes and min/max parameters in the palette object relate directly to
the palette of the internal image.

When using the server to output palette based 8 bit PNG images, the
legend defined here corresponds directly to the palette colors in
the PNG image.

1.  See [palette](palette.md) for a detailed description on palette
    possibilities.
2.  See [Predefined Legends](Predefined Legends.md) for some precooked legends for
    several physical quantities like temperature, pressure,
    precipitation, etc..

Define a continuous legend (colorRange)
---------------------------------------

The XML fragment below describes a colorRange Legend (continuous
colors):

```xml

<Legend name="ColorPalette" type="colorRange">
  <palette index="0" red="0" green="0" blue="255"/>
  <palette index="80" red="0" green="255" blue="255"/>
  <palette index="120" red="0" green="255" blue="0"/>
  <palette index="160" red="255" green="255" blue="0"/>
  <palette index="240" red="255" green="0" blue="0"/>
</Legend>

```
Index refers to the position in the palette, red green and blue to the
color values in the palette. Internally an interpolation between the
color values is performed.

Discrete legends (interval)
---------------------------

The following XML fragment describes a discrete legend (interval):

```xml
<Legend name="KNMIRadarPalette_mmh" type="interval">
  <palette min="0" max="38" red="255" green="255" blue="255"/>
  <palette min="38" max="80" red="170" green="170" blue="170"/>
  <palette min="80" max="118" red="85" green="85" blue="85"/>
  <palette min="118" max="160" red="255" green="128" blue="128"/>
  <palette min="160" max="198" red="255" green="0" blue="0"/>
  <palette min="198" max="255" red="0" green="0" blue="0"/>
</Legend>
```

SVG Gradient legends (file)
---------------------------

The legend with type "file" is loaded from a SVG gradient file, in that
case only the file attribute needs to be specified. E.g.

```xml
<Legend name="test" type="svg" file="/data/adaguc-datasets/legends/earth.svg"/>
```


Grouped legends
---------------

Grouped interval legends are intended for datasets that span a large value range,
but where displaying every interval individually would result in an overly long 
and difficult-to-read legend.

Unlike continuous (`colorRange`) legends, grouped legends are still composed of
individual `ShadeInterval` elements. However, a `ShadeInterval` may optionally
define a `fillcolor2`. In that case, the interval is rendered as a linear gradient 
between the two colors. If only `fillcolor` is specified, the interval is rendered
as a solid color.

A grouped legend is selected automatically whenever at least one `ShadeInterval` 
defines the `fillcolor2` attribute. 

Example:

```xml
<ShadeInterval min="0.2"   max="0.4"   fillcolor="#4A4A4A"/>
<ShadeInterval min="0.4"   max="0.7"   fillcolor="#B8B8B8"/>
<ShadeInterval min="0.7"   max="1.5"   fillcolor="#23BA46"
               fillcolor2="#058501"/>
<ShadeInterval min="1.5"   max="10.0"  fillcolor="#FB2600"
               fillcolor2="#912B14"/>
<ShadeInterval min="10.0"  max="20.0"  fillcolor="#C198B3"
               fillcolor2="#C8106A"/>
```

Each `ShadeInterval` is displayed as a single legend band of equal height,
regardless of the numeric width of the interval. This allows irregularly spaced
classification intervals to be represented in a compact and readable legend.

If gaps exist between consecutive `ShadeInterval` elements, they are displayed
as blank spaces in the legend.

The legend colors are generated using the same interpolation logic as the map
renderer.
