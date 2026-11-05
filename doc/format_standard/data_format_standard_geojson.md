GeoJSON format
==============

[Back](./README.md)

ADAGUC reads GeoJSON `FeatureCollection` files with `Polygon`, `MultiPolygon`, `LineString`, `MultiLineString` and `Point` features. This page describes how a file can carry dimensions such as time, reference time and elevation, so that a collection of GeoJSON files can be served as one layer.

1. Dimensions in a FeatureCollection
1. Polygon and line layers
1. Point layers

## 1) Dimensions in a FeatureCollection
-------------------------------------

Dimensions are set with a top-level `dimensions` object in the `FeatureCollection`. Each entry describes one dimension with a single `value`, so one file holds one time step, one reference time and one elevation. To serve several time steps, reference times or elevations, put each combination in its own file and point the layer `FilePath` at the directory; the files are indexed in the database like any other dataset.

```json
{
  "type": "FeatureCollection",
  "dimensions": {
    "reference_time": { "value": "2018-12-04T06:00:00Z", "units": "ISO8601" },
    "time": { "value": "2018-12-04T12:00:00Z", "units": "ISO8601" },
    "elevation": { "value": 500, "units": "m" }
  },
  "features": [ ... ]
}
```

The supported dimensions are:

| Name | `value` | WMS parameter | Notes |
|---|---|---|---|
| `time` | ISO8601 date string, or a number in seconds since 1970-01-01 | `TIME` | |
| `reference_time` | ISO8601 date string, or a number in seconds since 1970-01-01 | `DIM_reference_time` | Stored with standard_name `forecast_reference_time` |
| `elevation` | Number (integer or decimal) | `ELEVATION` | `units` is passed on to the GetCapabilities document |

Notes:
- For `time` and `reference_time` the `units` field is ignored; numbers are always interpreted as seconds since 1970-01-01. A missing or non-positive value results in 1970-01-01T00:00:00Z.
- The order of the entries in the `dimensions` object does not matter.
- The dimensions are detected automatically, no `<Dimension>` elements are needed in the layer configuration. They can still be added to configure for example the default value.

## 2) Polygon and line layers
----------------------------

Polygon and line features are rendered by configuring the layer with `<Variable>features</Variable>`. The `features` variable gets the `elevation`, `reference_time` and `time` dimensions from the `dimensions` object.

Example data with a reference time and a time per file is available in [data/datasets/geojson_reference_time](../../data/datasets/geojson_reference_time), with configuration [adaguc.testGeoJSONReader_reference_time.xml](../../data/config/datasets/adaguc.testGeoJSONReader_reference_time.xml):

```xml
<Layer type="database">
  <FilePath filter="^.*\.geojson$">{ADAGUC_PATH}/data/datasets/geojson_reference_time/</FilePath>
  <Name>features</Name>
  <Title>Features</Title>
  <Variable>features</Variable>
  <Styles>features</Styles>
</Layer>
```

A specific file is then selected with for example:

```
&REQUEST=GetMap&LAYERS=features&TIME=2018-12-04T12:00:00Z&DIM_reference_time=2018-12-04T06:00:00Z
```

## 3) Point layers
----------------

For point features, each feature property becomes a variable that can be used in `<Variable>`, for example `<Variable>name</Variable>`. These property variables get the `time`, `reference_time` and `elevation` dimensions from the `dimensions` object as well.

Example data with a reference time, time and elevation per file is available in [data/datasets/geojson_point_dimensions](../../data/datasets/geojson_point_dimensions), with configuration [adaguc.testGeoJSONReader_point_dimensions.xml](../../data/config/datasets/adaguc.testGeoJSONReader_point_dimensions.xml):

```xml
<Layer type="database">
  <FilePath filter="^.*\.geojson$">{ADAGUC_PATH}/data/datasets/geojson_point_dimensions/</FilePath>
  <Name>points</Name>
  <Title>Points</Title>
  <Variable>name</Variable>
  <Styles>stations_mistpost</Styles>
</Layer>
```

A specific file is then selected with for example:

```
&REQUEST=GetMap&LAYERS=points&TIME=2018-12-04T12:00:00Z&DIM_reference_time=2018-12-04T06:00:00Z&ELEVATION=1000
```
