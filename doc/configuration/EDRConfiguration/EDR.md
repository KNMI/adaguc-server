#  Configuration options to enable EDR collections

[Back to main configuration](../Configuration.md)

The EDR service by ADAGUC currently supports:

- `/instances` to retrieve the model_runs (treating the forecast_reference_time as instance name)
- `/position` for timeseries and point data
- `/cube` for extracting 2d/3d data slices

Data output is in CoverageJSON.

For a tutorial on EDR configuration, please check [Configure_EDR_service](../../tutorials/Configure_EDR_service.md).

## Configuration details

By default Adaguc will try to enable EDR for configured Adaguc layers. If your dataset is called `my_dataset.xml`, your layers can be found under the collection `my_dataset`.

It can be useful to group collections together, for example to indicate that collections belong to the same forecast model.
To group collections together, you use the `<Group collection="my_collection">` element to your layer. The layer will then be available under `my_dataset.my_collection`.

### Dimension types

You can specify a dimension type when configuring a dimension. The following dimension types are supported:

- `type="time"`: the time dimension, EDR will call this dimension `t`
- `type="reference_time"`: the reference time, EDR will call this dimension `custom:reference_time`
- `type="height"`: the height dimension, EDR will call this dimension `z`
- `type="custom"`: the custom (non-time, non-height, non-reference time) dimension, EDR will call this dimension `custom:your_dimension_name`

See [Dimension.md](/doc/configuration/Dimension.md) for additional Dimension configuration options.

## Collection rules

One dataset file can contain multiple collection definitions.

All parameters in an EDR collection should share identically named vertical and custom dimensions. For example: if you have a layer with a custom "member" dimension, and another layer with a custom "percentile" dimension, you should put these into separate collections through `<Group collection="member">` and `<Group collection="percentile">`. You will then find these in EDR under the collections `my_dataset.member` and `my_dataset.percentile` respectively.

It is not required that all parameters have exactly the same set of values for the shared dimensions; for example the first parameter could have values 2 and 10 for it's height dimension and the second parameter could have the values 2, 10 and 100 for the height dimension.

## Metadata API

Internally, EDR will use the metadata endpoint under `/adagucserver?service=wms&request=getmetadata&format=application/json`.

If you scan your dataset with `docker exec -i -t my-adaguc-server /adaguc/scan.sh -d my_dataset`, adaguc will fill/update the metadata for your dataset. Additionally, will periodically update the metadata for all configured datasets.
