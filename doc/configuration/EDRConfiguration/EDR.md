#  Configuration options to enable EDR collections

[Back to main configuration](../Configuration.md)

For a tutorial on EDR configuration, please check [Configure_EDR_service](../../tutorials/Configure_EDR_service.md)

The OGC API EDR service can be configured in a dataset configuration by adding the following elements:

```xml
    <OgcApiEdr>
        <EdrCollection name="harmonie">
            <EdrParameter name="air_temperature__at_2m" unit="Celsius"/>
        </EdrCollection>
    </OgcApiEdr>
```

EdrCollection:
- name: The name of the EDR collection
- vertical_dimension?: For example pressure_level

EdrParameter:
- name: The name of the layer in the WMS service
- unit: The unit of the Layer

