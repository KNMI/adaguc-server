#  Configuration options to enable EDR collections

An EDR service by ADAGUC currently supports the /instances call to retrieve the model_runs (treating the forecast_reference_time as instance name), the /position call for timeseries and point data and the /cube call for extracting 2d/3d data slices.

Data output is in CoverageJSON

[Back to main configuration](../Configuration.md)

For a tutorial on EDR configuration, please check [Configure_EDR_service](../../tutorials/Configure_EDR_service.md)

The OGC API EDR service can be configured in a dataset configuration by adding the following elements:

```xml
    <OgcApiEdr>
        <EdrCollection name="harmonie" time_interval="R48/{reference_time}+1/PT60M" vertical_name="height" >
            <EdrParameter name="air_temperature__at_height" unit="Celsius" standard_name="air_temperature"  parameter_label="Temperature of air" observed_property_label="Air temperature"/>
        </EdrCollection>
        <EdrCollection name="....">
          <EdrParameter name="..."/>
        </EdrCollection>
    </OgcApiEdr>
```

EdrCollection:
- name: The name of the EDR collection
- vertical_name?: For example pressure_level, the name of the vertical dimension in the WMS layer configuration
- time interval?: A description of the available times steps of a single run (EDR collection instance), expressed as a repeat number starting at the reference time (with an optional offset in time steps). For example "R2/{reference_time}+1/PT60M" means ["2024-06-01T01:00:00Z", "2024-06-01T02:00:00Z"] for a reference_time of 2024-06-1T00:00:00Z. This time range will be presented in the /collections call output. If the time_interval is not specified the whole time range from the WMS GetCapabilities output is used.

One dataset file can contain multiple collection definitions.
All parameters in an EDR collection should share identically named vertical and custom dimensions. It is not required that all parameters have exactly the same set of values for the shared dimensions; for example the fist parameter could have values 2 and 10 for it's height dimension and the second parameter could have the values 2, 10 and 100 for the height dimension.


EdrParameter:
- name: The name of the layer in the WMS service
- unit: The unit of the Layer
- standard_name: Standard name of parameter
- parameter_label?: Label text for parameter
- observed_property_label?: Observed property label (for covjson)


