WMS OPENDAP
===========

The server can also read data remotely from an OpenDAP service. The same
routine applies as given in [Create a WMS service on a file](Create a WMS service on a file.md),
except that you have to provide an OpenDAP url.

For example:
```
<!-- Layer configuration -->
<Layer type="database">
<FilePath>http://opendap.nmdc.eu/knmi/thredds/dodsC/essence/run031/2D\_3hrly/temp2\_031\_2006-2010.nc</FilePath>
<Variable>temp2</Variable>
<Styles>rainbow</Styles>
<Dimension name="time" interval="PT3H"
default="max">time</Dimension>
</Layer>
```

After you have added a new dataset, you always need to do:
```
/data/software/adagucserver/bin/adagucserver --updatedb --config
/data/services/config/newservice.xml
```

Some facts about Opendap
------------------------

-   OPeNDAP is the name of the organization and the name of the protocol
-   Open-source Project for a Network Data Access Protocol
-   Data is stored at remote server
-   Data model is similar to NetCDF’s data model (with differences)
    -   N-dimensional array container, with variables, dimensions and
        attributes
-   Only requested pieces of data are sent
    -   Accessing small pieces of large files on a remote server can
        still be quick
    -   Data is requested based on sub-setting along dimensions
-   OPeNDAP resources can be opened locally on your computer as if it
    were local files using the NetCDF library
    -   Local files versus remote files is transparent
-   The concept of a file is gone, an OPeNDAP endpoint can represent
    thousands of files aggregated along a dimension
    -   E.g. Usually concatenate a large time series observation to one
        endpoint using the time dimension

