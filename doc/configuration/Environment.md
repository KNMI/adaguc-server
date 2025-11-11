Environment (name, default)
===============

Back to [Configuration](./Configuration.md)

Available since: 2.8.6

Allows to substitute global system environment variables into a dataset configuration.

- name The global system environment variable and the name inside the dataset configuration. Note that these should be prefixed with `ADAGUCENV`
- default The default value if no environment variable is set.


It can for example be used in the following dataset configuration snippet:


```xml
<?xml version="1.0" encoding="UTF-8"?>
<Configuration>
  <Environment name="ADAGUCENV_RETENTIONPERIOD" default="PT10M" />
  <Environment name="ADAGUCENV_ENABLECLEANUP" default="dryrun" />
  <Settings enablecleanupsystem="{ADAGUCENV_ENABLECLEANUP}" cleanupsystemlimit="5" />
  <Layer type="database">
    <FilePath filter=".*\.nc$" retentionperiod="{ADAGUCENV_RETENTIONPERIOD}"
      retentiontype="datatime">
      /data/adaguc-data/livetimestream/</FilePath>

    <Variable>data</Variable>
    <Dimension name="time" interval="PT1M" quantizeperiod="PT1M" quantizemethod="low">time</Dimension>
    <RenderMethod>rgba</RenderMethod>
  </Layer>

</Configuration>
```

By default it will not enable the cleanup, but only inform what would be deleted. But by setting the following environment:

```
export ADAGUCENV_RETENTIONPERIOD="P7D"
export ADAGUCENV_ENABLECLEANUP=true
```

Cleanup will be enabled and retentionperiod will be set to 7 days