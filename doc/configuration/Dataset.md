Dataset (enabled, location)
===========================

-   enabled - Set to "true" to enable this feature
-   location - A directory where dataset configuration files are located
-   Note that the DATASET identifier from the URL is escaped, the ':'
    and '/' characters become '\_' on the filesystem. The .xml extension
    will be appended by the server.

Dataset can be used to point to a part of a configuration file which
will be included in the main configuration file. This is useful for
configuring new services with small configuration files. The
configuration files can be loaded by setting the dataset=<name>
key value pair in the url.

E.g.:
<Dataset enabled="true"
location="/data/services/config/datasets"/>

And URL:
service=WMS&amp;dataset=8f0ca2cf-3266-40ca-8d5f-372986645286&amp;request=...
will load additional configuration file
/data/services/config/datasets/8f0ca2cf-3266-40ca-8d5f-372986645286.xml

-   See also [AutoResource](AutoResource.md) for serving files
-   This is applied in INSPIRE view services, as described in
    \[\[Configuration\_of\_an\_INSPIRE\_View\_Service\]\]

Using DATASET includes and serving OpenDAP
------------------------------------------

```
http://yourhost.com/adaguc.dataset.cgi/opendap/8f0ca2cf-3266-40ca-8d5f-372986645286/testdata
- or -
http://yourhost.com/adaguc.dataset.cgi/opendap/8f0ca2cf-3266-40ca-8d5f-372986645286/
```

1.  8f0ca2cf-3266-40ca-8d5f-372986645286 is the dataset identifier
2.  testdata is the variable in the netcdf file, can be left blank. When
    blank the first layer with type database is used.

-   [AutoResource](AutoResource.md) must be disabled in this case
-   See [OpenDAP](OpenDAP.md) for serving data with opendap.

