Dimension (name,interval,default,units) <value>
=====================================================

Back to [Configuration](./Configuration.md)

-   name - The name of the dimension in the netcdf file
-   interval - Optional, the time resolution of the dataset in
    [ISO8601](ISO8601.md) format.
-   default - The default value of the dimension in [ISO8601](ISO8601.md)
    format, can also be "min" or "max" to select the first or latest
    date as default. Defaults to "max".
-   units - Override the units of the dimension
-   quantizeperiod - Optional, see below
-   quantizemethod - Optional, see below
-   <value> - The name of the dimension in the WMS service

```xml
<Layer>
  <Dimension name="time" interval="P1D" default="2011-06-30T00:00:00Z">time</Dimension>
....
</Layer>
```

-   See [ISO8601](../info/ISO8601.md) for the time resolution specification

Time quantization
-----------------

Time values received in the URL as input can be rounded to more discrete
time periods. For example when noon, 12:03:53, is received as input, it
is possible to round this to 12:00:00. This enables the service to
respond to fuzzy time intervals.

* quantizeperiod - The time resolution to round to
* quantizemethod - Optional, can be either low, high and round,
defaults to round.
  * low - rounds down
  * high - rounds up
  * round - rounds to closest

Example with 5 minute quantization perdiod and method round:

```xml 
<Dimension name="time" interval="PT5M" quantizeperiod="PT5M" quantizemethod="round" >time</Dimension>
```

This will convert the following inputs to internal dates:

-   2015-01-21T15:14:59Z --> 2015-01-21T15:15:00Z
-   2015-01-21T15:16:23Z --> 2015-01-21T15:15:00Z
-   2015-01-21T15:18:36Z --> 2015-01-21T15:20:00Z
-   2015-01-21T15:21:01Z --> 2015-01-21T15:20:00Z

