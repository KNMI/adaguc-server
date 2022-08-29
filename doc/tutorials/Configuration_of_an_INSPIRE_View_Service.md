[Configuration](Configuration.md)

Configuration of an INSPIRE View Service
========================================

The following elements need to be configured to create an INSPIRE View
Service:

-   WMS->Title needs to be set to \[INSPIRE::TITLE\]
-   WMS->Abstract needs to be set to \[INSPIRE::ABSTRACT\]
-   WMS~~>RootLayer~~>Title needs to be set to \[INSPIRE::TITLE\]

<!-- -->

-   WMS~~>Inspire~~>ViewServiceCSW - The global view service
    document (CSW service) describing the view services
-   WMS~~>Inspire~~>DatasetCSW - The dataset CSW service
    describing the dataset
-   WMS~~>Inspire~~>AuthorityURL - The name and URL of the
    Authority offering the data
-   WMS~~>Inspire~~>Identifier - The identifier of the Authority

<!-- -->

-   Layer~~>Abstract~~ An abstract describing the layer, Name and
    Title needs to be set as normal

```

<WMS>
<Title>\[INSPIRE::TITLE\]</Title>
<Abstract>\[INSPIRE::ABSTRACT\]</Abstract>
<RootLayer>
<Title>\[INSPIRE::TITLE\]</Title>
<Abstract></Abstract>
</RootLayer>
<TitleFont location="/data/fonts/FreeSans.ttf" size="19"/>
<SubTitleFont location="/data/fonts/FreeSans.ttf" size="10"/>
<DimensionFont location="/data/fonts/FreeSans.ttf" size="7"/>
<ContourFont location="/data/fonts/FreeSans.ttf" size="7"/>
<GridFont location="/data/fonts/FreeSans.ttf" size="5"/>
<WMSFormat name="image/png" format="image/png"/>
<Inspire>
<ViewServiceCSW>http://data-test.knmi.nl/inspire/csw?Service=CSW&amp;Request=GetRecordById&amp;Version=2.0.2&amp;id=9155a021-ac62-464c-99e2-b4c809e5ecde&amp;outputSchema=http://www.isotc211.org/2005/gmd&amp;elementSetName=full</ViewServiceCSW>
<DatasetCSW>http://data-test.knmi.nl/inspire/csw?Service=CSW&amp;Request=GetRecordById&amp;Version=2.0.2&amp;id=0d6f8f49-94d0-4e21-a7c1-2d703ce1fd22&amp;outputSchema=http://www.isotc211.org/2005/gmd&amp;elementSetName=full</DatasetCSW>
<AuthorityURL name="NL.KNMI" onlineresource="http://knmi.nl/"/>
<Identifier authority="NL.KNMI" id="id_value"/>
</Inspire>
</WMS>

<Layer>
...
<Abstract>nice abstract describing the layer</Abstract>
</Layer>

```

Configuration of an INSPIRE View Service with one service endpoint and multiple datasets
========================================================================================

This tutorial is based on a single endpoint for a WMS service, where
several datasets are identified with the DATASET= parameter.

The endpoint is for example:
```
http://data-test.knmi.nl/inspire/wms/cgi-bin/wms.cgi?
```

Datasets within the WMS service are identified with the DATASET=
parameter, for example:

```
http://data-test.knmi.nl/inspire/wms/cgi-bin/wms.cgi?DATASET=urn:xkdc:ds:nl.knmi::kisinspire2/1/&
```

In the GetCapabilities document this URL is returned as online resource
for the GetMap, GetFeatureInfo and GetLegendGraphic requests.

The datasets identified with the DATASET parameter are extra
configuration files which are included in the main configuration file of
the WMS service. To enable this feature, the [Dataset](Dataset.md) option
needs to be configured in the WMS service.

When everything is configured correctly, there is one global
configuration file and several dataset configuration files representing
the datasets.

-   /data/config/inswms.xml - The global configuration file
-   /data/datasetconfigs/urn_xkdc_ds_nl.knmi_*kisinspire2_1*.xml -
    The dataset configuration file, which is loaded by using the
    DATASET=urn:xkdc:ds:nl.knmi::kisinspire2/1/ parameter. Note that the
    DATASET identifier is escaped, the ':' and '/' characters become
    '_' on the filesystem.

The global configuration file is named /data/config/inswms.xml and can
look like this:

```

<WMS>
<Title>\[INSPIRE::TITLE\]</Title>
<Abstract>\[INSPIRE::ABSTRACT\]</Abstract>
<RootLayer>
<Title>\[INSPIRE::TITLE\]</Title>
<Abstract></Abstract>
</RootLayer>
<TitleFont location="/data/fonts/FreeSans.ttf" size="19"/>
<SubTitleFont location="/data/fonts/FreeSans.ttf" size="10"/>
<DimensionFont location="/data/fonts/FreeSans.ttf" size="7"/>
<ContourFont location="/data/fonts/FreeSans.ttf" size="7"/>
<GridFont location="/data/fonts/FreeSans.ttf" size="5"/>
<WMSFormat name="image/png" format="image/png"/>
<Inspire>
<ViewServiceCSW>http://data-test.knmi.nl/inspire/csw?Service=CSW&amp;Request=GetRecordById&amp;Version=2.0.2&amp;id=9155a021-ac62-464c-99e2-b4c809e5ecde&amp;outputSchema=http://www.isotc211.org/2005/gmd&amp;elementSetName=full</ViewServiceCSW>
<AuthorityURL name="NL.KNMI" onlineresource="http://knmi.nl/"/>
<Identifier authority="NL.KNMI" id="id_value"/>
</Inspire>
</WMS>

<Dataset enabled="true" location="/data/datasetconfigs/"/>
```

Note that in the Inspire section the DatasetCSW is removed, as this
option is dependent of the dataset in question.

The dataset configuration file needs to have the filename
urn_xkdc_ds_nl.knmi_*kisinspire2_1*.xml and looks like this:
```
<?xml version="1.0" encoding="UTF-8" ?>
<Configuration>
<WMS>
<Inspire>
<DatasetCSW>http://data-test.knmi.nl/inspire/csw?Service=CSW&amp;Request=GetRecordById&amp;Version=2.0.2&amp;id=0d6f8f49-94d0-4e21-a7c1-2d703ce1fd22&amp;outputSchema=http://www.isotc211.org/2005/gmd&amp;elementSetName=full</DatasetCSW>
</Inspire>
</WMS>
<Layer type="database">
<FilePath
filter=".\*\\.nc\$">/data/kisinspire2/1/</FilePath>
<Name>TG_air_temperature</Name>
<Title>Temperature</Title>
<Variable>TG</Variable>
<Abstract>Daily mean temperature in (0.1 degrees
Celsius);</Abstract>
</Layer>
</Configuration>
```
This file is located in /data/datasetconfigs/

Here the WMS~~>Inspire~~>DatasetCSW is given, including the Layer.

This sub configuration file can be generated with the adagucserver using
the following command:

```
adagucserver --getlayers --file
/data/kisinspire2/1/inspire_daily_weather_observations_kis_v20131021.nc
--inspiredatasetcsw
"http://data-test.knmi.nl/inspire/csw?Service=CSW&Request=GetRecordById&Version=2.0.2&id=0d6f8f49-94d0-4e21-a7c1-2d703ce1fd22&outputSchema=http://www.isotc211.org/2005/gmd&elementSetName=full"
--datasetpath /data/kisinspire2/1/ >
/data/datasetconfigs/urn_xkdc_ds_nl.knmi_*kisinspire2_1*.xml
```

To update the database with these two configuration files you can use
the following command:

```
adagucserver --updatedb --config
/data/config/inswms.xml,/data/datasetconfigs/urn_xkdc_ds_nl.knmi_*kisinspire2_1*.xml
--tailpath <optional, path to granule in datasetdirectory>
```
