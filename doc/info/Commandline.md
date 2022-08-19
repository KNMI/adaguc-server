Commandline
===========

adagucserver
------------

### Update the database

**--updatedb**

-   --updatedb
-   --config the configuration files to use
-   --path for a absolute path to update
-   --tailpath for scanning specific sub directory (Tailpath is not
    allowed to start with a /)
-   -~~layername Optionally specify the layername to scan, as specified
    in Layer~~>Name to update
-   --rescan Ignores file modification date and re-inserts the file.
-   --noclean Do not remove file records from the database
-   --recreate Drops and recreates database tables
-   --dumpheader Shows NetCDF header

### Get layers - scan a file, returns the possible layers for that file

**--getlayers**

-   --getlayers
-   --file
-   --inspiredatasetcsw
-   --datasetpath

### --createtiles

-   --adagucserver --createtiles --config
    \~/adagucserver/data/config/adaguc.pik.xml

### Output report to a file

**--report**

-   --report=filename to write report specified by filename
-   --report to write report specified in environment variable
    ADAGUC\_CHECKER\_FILE. If variable ADAGUC\_CHECKER\_FILE is not set,
    write file to default location ./checker\_report.txt

aggregate\_time
---------------

Aggregates multiple files over the time dimension into one big file.
Does work on files without unlimited dims as well.
Commandlande arguments are <input dir> <output file>,
optionaly third argument can be a comma separated list of variable names
to add the time dimension to.

E.g.
```
aggregate\_time /data/swe\_L3A\_daily/ /tmp/swe\_L3A\_daily.nc swe
```
