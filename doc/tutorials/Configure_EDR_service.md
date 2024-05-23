# Configure an EDR timeseries service using AdagucServer

- [Back to readme](./Readme.md)
- [Configuration details for OgcApiEdr](../configuration/EDRConfiguration/EDR.md)


## Prerequisites

Make sure adaguc-server is running, follow the instructions at [Starting the adaguc-server with docker](../Running.md)

## Step 1: Copy a file with timesteps into the adaguc-data folder

Copy the file HARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc into the adaguc-data folder:


```
cp ${ADAGUC_PATH}/data/datasets/forecast_reference_time/HARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc ${ADAGUC_DATA_DIR}
```
This file is available in the adaguc-server repository with location `data/datasets/forecast_reference_time/HARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc`

## Step 2: Configure a dataset for this datafile, including EDR support

Create the following file at the filepath `$ADAGUC_DATASET_DIR/edr.xml`. You can also consider changing `<FilePath>` to `/data/adaguc-data/*.nc`.

```xml

<Configuration>

    <OgcApiFeatures />

    <OgcApiEdr>
        <EdrCollection name="harmonie">
            <EdrParameter
                name="air_temperature__at_2m"
                unit="°C"
                standard_name="air_temperature"
                observed_property_label="Air temperature"
                parameter_label="Air temperature, 2 metre" />
        </EdrCollection>
    </OgcApiEdr>


    <!-- Styles -->
    <Style name="temperature">
        <Legend fixedclasses="true" textformatting="%0.0f" tickinterval="2">bluewhitered</Legend>
        <Min>-10</Min>
        <Max>10</Max>
    </Style>

    <!-- Layers -->

    <Layer type="database">

        <FilePath>
            /data/adaguc-data/HARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc</FilePath>
        <Variable units="Celsius">air_temperature__at_2m</Variable>
        <Styles>temperature</Styles>
    </Layer>
    <!-- End of configuration /-->
</Configuration>

```


### EdrParameter settings

```xml
<EdrParameter
                name="air_temperature__at_2m"
                unit="°C"
                standard_name="air_temperature"
                observed_property_label="Air temperature"
                parameter_label="Air temperature, 2 metre" />
```                

- name: Mandatory, Should be one of the WMS Layer names as advertised in the WMS GetCapabilities
- unit: Mandatory, Sets the unit for the parameter in the parameter_names section of the collection document
- standard_name: Sets the observedProperty id
- observed_property_label: Sets the observedProperty label
- parameter_label: Sets the label for the parameter in the parameter_names section

For the given example this will result in the following parameter name definition:

```json
parameter_names": {
    "air_temperature__at_2m": {
      "type": "Parameter",
      "id": "air_temperature__at_2m",
      "label": "Air temperature, 2 metre",
      "description": "harmonie - air_temperature__at_2m (air_temperature__at_2m)",
      "unit": {
        "symbol": {
          "value": "°C",
          "type": "http://www.opengis.net/def/uom/UCUM"
        }
      },
      "observedProperty": {
        "id": "https://vocab.nerc.ac.uk/standard_name/air_temperature",
        "label": "Air temperature"
      }
    }
  }
```

*The description is currently read from the GetCapabilities document, using the WMS Layer Title section prefixed with the dataset name.


## Step 3: Scan the new data

```
docker exec -i -t my-adaguc-server /adaguc/adaguc-server-updatedatasets.sh edr
```

## Step 4: Check if the EDR endpoint works


Visit:
- https://yourhostname/edr/collections/harmonie
- https://yourhostname/edr/collections/harmonie/instances
- https://yourhostname/edr/collections/harmonie/instances/?f=application/json
- https:///yourhostname/edr/collections/harmonie/instances/201712150900/position?coords=POINT(4.782 52.128)&datetime=2017-12-15T09:00Z/2017-12-17T09:00Z&parameter-name=air_temperature__at_2m&crs=EPSG:4326&f=CoverageJSON

You can also try it in https://labs.metoffice.gov.uk/edr/static/html/query.html


See:

![](2023-11-23-AdagucServer_EDR_In_MetOffice_EDR_Viewer.png)
