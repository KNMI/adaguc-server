WMS Extensions
==============

GetFeatureInfo
--------------

The GetFeatureInfo call needs all parameters for a GetMap request,
except REQUEST and adds X and Y (the pixel coordinates of interest) and
QUERY\_LAYERS (the layer(s) of interest)

For multimodel timeseries please check [Ensemble timeseries](Ensemble timeseries.md)

The ADAGUC service extends the GetFeatureInfo call in a number of
aspects:

New request parameters:

-   ...
-   JSONP=<javascript\_function\_name> *data is returned as
    JavaScript code, where provided function is called with JSON result*

Extended request parameters:

-   INFO\_FORMAT=<FORMAT> \_which can be image/gif, image/png,
    text/plain, application/xml, text/html, application/json

### JSON format

The URL
http://bvmlab-218-41.knmi.nl/cgi-bin/ECMD\_EUR.cgi?&SERVICE=WMS&amp;REQUEST=GetFeatureInfo&amp;VERSION=1.1.1
&SRS=EPSG%3A4326&amp;LAYERS=201306130000%2FLS\_precipitation&amp;QUERY\_LAYERS=201306130000%2FLS\_precipitation
&BBOX=-180,-131.06076210092687,180,131.06076210092687&amp;WIDTH=971&amp;HEIGHT=707&amp;X=482&amp;Y=224
&FORMAT=image/gif&amp;INFO\_FORMAT=application/json&amp;STYLES=&&time=2013-06-13T00:00:00Z/2013-06-16T03:00:00Z

gives the following result:
```<code class="json">
\[

{
"name": "large\_scale\_precipitation",
"standard\_name": "large\_scale\_precipitation",
"units": "mm",
"point": {
"SRS": "EPSG:4326",
"coords": "-1.297631,48.012358"
},
"dims": "time",
"data": {
"2013-06-13T03:00:00Z": "0.000",
"2013-06-13T06:00:00Z": "0.000",
"2013-06-13T09:00:00Z": "0.222",
"2013-06-13T12:00:00Z": "0.552",
"2013-06-13T15:00:00Z": "0.565",
"2013-06-13T18:00:00Z": "0.565",
"2013-06-13T21:00:00Z": "0.565",
"2013-06-14T00:00:00Z": "0.568",
"2013-06-14T03:00:00Z": "0.568",
"2013-06-14T06:00:00Z": "0.568",
"2013-06-14T09:00:00Z": "0.568",
"2013-06-14T12:00:00Z": "0.568",
"2013-06-14T15:00:00Z": "0.568",
"2013-06-14T18:00:00Z": "0.568",
"2013-06-14T21:00:00Z": "0.566",
"2013-06-15T00:00:00Z": "0.566",
"2013-06-15T03:00:00Z": "0.566",
"2013-06-15T06:00:00Z": "0.614",
"2013-06-15T09:00:00Z": "1.021",
"2013-06-15T12:00:00Z": "1.027",
"2013-06-15T15:00:00Z": "1.027",
"2013-06-15T18:00:00Z": "1.027",
"2013-06-15T21:00:00Z": "1.027",
"2013-06-16T00:00:00Z": "1.027",
"2013-06-16T03:00:00Z": "1.027"
}
}

\]
</code>```

This output is more or less self-describing. The requested coordinate is
always returned in EPSG:4326. A list of the data's dimensions is also
provided.
The following code is an example of Javascript to handle the json:
```<code class="JavaScript">
//JSON result is parsed and stored in the variable precip
precip\["units"\] is the units description
precip\["dims"\] is an array of the dimensions.
precip\["data"\]\["2013-06-16T03:00:00Z"\] is the last value of the
requested timeseries data.
</code>```

GetPointValue
-------------

GetPointValue is kind of a shorthand for GetFeatureInfo. It enables one
to do a GetFeatureInfo like request, without specifying a WMS GetMap
request.
This can be used to retrieve for example a time series of data for a
certain lat/lon coordinate or in the case of ensemble data to get all 50
timeseries for a "plume" image.

Parameters for a GetPointValue request are:

An example of a multi-dimensional data request:
http://bvmlab-218-41.knmi.nl/cgi-bin/ECMD\_EUR.cgi?&SERVICE=WMS&amp;REQUEST=GetPointValue&amp;VERSION=1.1.1
&SRS=EPSG:4326&amp;QUERY\_LAYERS=201306130000/Temperature\_pl&amp;X=5.2&amp;Y=52.0&amp;INFO\_FORMAT=application/json&amp;
time=2013-06-13T00:00:00Z/2013-06-13T12:00:00Z&amp;DIM\_pressure=300,500

Output:
```<code class="json">
\[

{
"name": "air\_temperature",
"standard\_name": "air\_temperature",
"units": "Celsius",
"point": {
"SRS": "EPSG:4326",
"coords": "5.200000,52.000000"
},
"dims": \[
"pressure",
"time"
\],
"data": {
"300": {
"2013-06-13T00:00:00Z": "-39.54",
"2013-06-13T03:00:00Z": "-39.69",
"2013-06-13T06:00:00Z": "-39.62",
"2013-06-13T09:00:00Z": "-39.57",
"2013-06-13T12:00:00Z": "-39.76"
},
"500": {
"2013-06-13T00:00:00Z": "-12.46",
"2013-06-13T03:00:00Z": "-12.20",
"2013-06-13T06:00:00Z": "-12.19",
"2013-06-13T09:00:00Z": "-12.61",
"2013-06-13T12:00:00Z": "-12.59"
}
}
}

\]
</code>```

A request can also be for multiple variables:
http://bvmlab-218-41.knmi.nl/cgi-bin/ECMD\_EUR.cgi?&SERVICE=WMS&amp;REQUEST=GetPointValue&amp;VERSION=1.1.1
&SRS=EPSG:4326&amp;QUERY\_LAYERS=201306130000/Temperature\_pl,201306130000/ZGeo\_Height&amp;X=5.2&amp;Y=52.0
&INFO\_FORMAT=application/json&amp;time=2013-06-13T00:00:00Z/2013-06-13T12:00:00Z&amp;DIM\_pressure=300,500

Output:
```<code class="json">
\[

{
"name": "air\_temperature",
"standard\_name": "air\_temperature",
"units": "Celsius",
"point": {
"SRS": "EPSG:4326",
"coords": "5.200000,52.000000"
},
"dims": \[
"pressure",
"time"
\],
"data": {
"300": {
"2013-06-13T00:00:00Z": "-39.54",
"2013-06-13T03:00:00Z": "-39.69",
"2013-06-13T06:00:00Z": "-39.62",
"2013-06-13T09:00:00Z": "-39.57",
"2013-06-13T12:00:00Z": "-39.76"
},
"500": {
"2013-06-13T00:00:00Z": "-12.46",
"2013-06-13T03:00:00Z": "-12.20",
"2013-06-13T06:00:00Z": "-12.19",
"2013-06-13T09:00:00Z": "-12.61",
"2013-06-13T12:00:00Z": "-12.59"
}
}
},
{
"name": "height",
"standard\_name": "height",
"units": "m",
"point": {
"SRS": "EPSG:4326",
"coords": "5.200000,52.000000"
},
"dims": \[
"pressure",
"time"
\],
"data": {
"300": {
"2013-06-13T00:00:00Z": "9426",
"2013-06-13T03:00:00Z": "9411",
"2013-06-13T06:00:00Z": "9398",
"2013-06-13T09:00:00Z": "9392",
"2013-06-13T12:00:00Z": "9377"
},
"500": {
"2013-06-13T00:00:00Z": "5723",
"2013-06-13T03:00:00Z": "5710",
"2013-06-13T06:00:00Z": "5696",
"2013-06-13T09:00:00Z": "5689",
"2013-06-13T12:00:00Z": "5677"
}
}
}

\]
</code>```

GetMetaData
-----------

The GetMetaData request fetches the metadata for a layer of a service.

An example URL:

http://bvmlab-218-41.knmi.nl/cgi-bin/ECMD\_EUR.cgi?&SERVICE=WMS&amp;REQUEST=GetMetaData&amp;VERSION=1.1.1
&LAYER=201306130000%2FZGeo\_Height&amp;FORMAT=text/html&amp;&time=2013-06-16T03%3A00%3A00Z&amp;DIM\_pressure=300

Output:
```
CCDFDataModel {
dimensions:
lats\_1 = 101 ;
lons\_1 = 201 ;
time = 26 ;
pressure\_level = 5 ;
time2 = 25 ;
variables:
float lats\_1(lats\_1) ;
lats\_1:units = "degrees\_north" ;
lats\_1:long\_name = "latitude" ;
lats\_1:standard\_name = "latitude" ;
float lons\_1(lons\_1) ;
lons\_1:units = "degrees\_east" ;
lons\_1:long\_name = "longitude" ;
lons\_1:standard\_name = "longitude" ;
double time(time) ;
time:long\_name = "time" ;
time:standard\_name = "time" ;
time:units = "hours since 2013-06-13 00:00:00" ;
float pressure\_level(pressure\_level) ;
pressure\_level:units = "hPa" ;
pressure\_level:standard\_name = "pressure\_level" ;
pressure\_level:long\_name = "pressure\_level" ;
pressure\_level:positive = "down" ;
double time2(time2) ;
...
...
...
```

The metadata returned usually looks like an ncdump output for the
requested data.

ncWMS extensions
----------------

A number of extensions that where built into ncwms
(http://www.resc.rdg.ac.uk/trac/ncWMS) are also built into the ADAGUC
service.
[NCWMSExtensions](NCWMSExtensions.md)
