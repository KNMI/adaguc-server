# Configure an EDR timeseries service using AdagucServer

- [Back to readme](./Readme.md)
- [Configuration details for EDR](../configuration/EDRConfiguration/EDR.md)

## Prerequisites

Make sure adaguc-server is running, follow the instructions at [Starting the adaguc-server with docker](../Running.md)

## Step 1: Copy a file with timesteps into the adaguc-data folder

Copy the file from the adaguc-server repository under `data/datasets/forecast_reference_time/HARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc` into the configured adaguc-data folder:

```bash
cp ${ADAGUC_PATH}/data/datasets/forecast_reference_time/HARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc ${ADAGUC_DATA_DIR}
```

## Step 2: Configure a dataset for this datafile, including EDR support

Create the following file at the filepath `$ADAGUC_DATASET_DIR/edr.xml`. You can also consider changing `<FilePath>` to `/data/adaguc-data/*.nc`.

```xml
<?xml version="1.0" encoding="UTF-8" ?>
<Configuration>
    <!-- Styles -->
    <Style name="temperature">
        <Legend fixedclasses="true" textformatting="%0.0f" tickinterval="2">bluewhitered</Legend>
        <Min>-10</Min>
        <Max>10</Max>
    </Style>

    <!-- Layers -->
    <Layer type="database">
        <Name>my_temperature</Name>
        <Title>Temperature</Title>
        <Abstract>The temperature at 2m height</Abstract>
        <Group collection="mycollection" />
        <FilePath>/data/adaguc-data/tutorial/HARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc</FilePath>
        <Variable units="Celsius" long_name="air_temperature">air_temperature__at_2m</Variable>
        <Styles>temperature</Styles>
    </Layer>
</Configuration>
```

For the given example this will result in the following parameter name definition:

```json
"parameter_names": {
  "my_temperature": {
    "type": "Parameter",
    "id": "my_temperature",
    "label": "Temperature",
    "description": "The temperature at 2m height",
    "unit": {
      "symbol": {
        "value": "Celsius",
        "type": "http://www.opengis.net/def/uom/UCUM"
      }
    },
    "observedProperty": {
      "id": "https://vocab.nerc.ac.uk/standard_name/air_temperature",
      "label": "air_temperature"
    }
  }
}
```

## Step 3: Scan the new data

```bash
docker exec -i -t my-adaguc-server scan.sh -d edr
```

## Step 4: Check if the EDR endpoint works

The EDR endpoint is at `/edr/collections`. This will list all the configured collections.

You can find the collection from above under `edr.mycollection`. This EDR collection name is a combination of the dataset name (`edr`) and the "group collection" (`mycollection`).

The following examples test the `/collections`, `/instances`, `/position` and the `/cube` EDR calls:
- https://yourhostname/edr/collections
- https://yourhostname/edr/collections/edr.mycollection
- https://yourhostname/edr/collections/edr.mycollection/instances
- https://yourhostname/edr/collections/edr.mycollection/instances/201712150900
- https://yourhostname/edr/collections/edr.mycollection/instances/201712150900/position?coords=POINT(4.782%2052.128)&datetime=2017-12-15T09:00Z/2017-12-17T09:00Z&parameter-name=air_temperature__at_2m&crs=EPSG:4326&f=CoverageJSON
- https://yourhostname/edr/collections/edr.mycollection/instances/201712150900/cube?bbox=5,52,7,54&datetime=2017-12-15T09:00Z/2017-12-17T09:00Z&parameter-name=air_temperature__at_2m&crs=EPSG:4326&f=CoverageJSON


You can also try pasting your EDR endpoint https://yourhostname/edr/collections in https://labs.metoffice.gov.uk/edr/static/html/query.html.


See:

![](2023-11-23-AdagucServer_EDR_In_MetOffice_EDR_Viewer.png)

## Trajectory queries

Use `/trajectory` on a collection or instance with a 2D WKT `LINESTRING`:

```text
/edr/collections/edr.mycollection/trajectory?coords=LINESTRING(4.7%2052.1,5.5%2052.5)&parameter-name=my_temperature&z=*
```

The response is a CoverageJSON Section with selected levels in `z` and
`[time, longitude, latitude]` tuples along the path. It returns native cell centres
and values without interpolation. Consecutive duplicate cells are suppressed;
later revisits remain. Omitted `z` selects all visible levels; hidden heights keep
their configured selection and are reported in `z`. Omitted `datetime` uses the
configured default, as for position. Without `parameter-name`, only parameters
with time and an explicitly typed vertical dimension are selected. Explicitly
requesting an unsuitable parameter returns HTTP 400. A path outside the grid
returns HTTP 404.

Supported: numeric parameters with time and a vertical dimension configured with `type="vertical"`,
on regular grids with matching one-dimensional axes across source files,
including rotated-pole grids. Each axis needs at least two cells; spatial
dimensions must be the final two variable dimensions. Layers requiring multiple
source variables or `orgname` renaming are unsupported.

Only `EPSG:4326` and CoverageJSON are supported. `resolution_x` and `resolution_y`
are rejected; rendering stride is ignored. Vertical coordinates must be finite
numbers. Failed trajectory data calls return HTTP 400.

The current fixed limits are:

- **1,000 input vertices:** coordinate pairs supplied in the `LINESTRING`.
- **2,000 path samples per layer:** crossed grid cells in path order, including
  later revisits. Consecutive duplicates do not count again.
- **1 million sample/dimension combinations per layer:** path samples multiplied
  by the number of selected time/level/member/other dimension combinations.
  For example, 500 samples × 20 times × 90 levels counts as 900,000.

The value limit applies separately to each parameter, not to the whole response.
The existing database `maxquerylimit` also restricts dimension selections, and
source files across all requested parameters must fit the file store's open-file
capacity. Complex projected paths can hit a separate subdivision limit.
