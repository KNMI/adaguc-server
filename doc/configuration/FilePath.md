FilePath (filter, maxquerylimit, ncml) <value>
====================================================

-   filter - A regular expression, used to filter files inside a
    directory
-   maxquerylimit - The maximum amount of time values will be used in
    the single request
-   ncml - The ncmlfile to apply, see
    https://github.com/KNMI/adaguc-server/blob/master/doc/ncml.md for
    details
-   <value> - Directory, file or OpenDAP url

The following configuration adds all files in the specified directory
matching the regular expression \^KMDS*TEST_P1M_OBSL2*_.\*\\.nc\$ to
the service:
```
<FilePath
filter="\^KMDS*TEST_P1M_OBSL2*_.\*\\.nc\$">/data/sdpkdc/kmds/</FilePath>
```

filter
------

-   In case a file or OpenDAP URL is given, the filter can be set to ""
    or can be left blank.

<!-- -->

-   If you would like to filter specific files you can use regular
    expressions.
    For example, if you would like to filter the following files based
    on prefixes:
    -   CFD_index_0.25deg_reg_v8.0.nc
    -   FD_index_0.25deg_reg_v8.0.nc
        You can use the filter "\^CFD_index.**\$" and
        "\^FD_index.**\$" respectively.

<!-- -->

-   The default to scan all files with .nc extension is "\^.\*\\.nc\$"

maxquerylimit
-------------

-   Allows you to set the maximum query limit of the database, default
    is 512 entries. Overrides the settings set in the [DataBase](DataBase.md)
    configuration.

