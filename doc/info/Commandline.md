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
-   --verboseoff Do not show verbose logs to trace errors
-   --autofinddataset When providing a --path, adaguc will automatically find the matching dataset. 

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
    ADAGUC_CHECKER_FILE. If variable ADAGUC_CHECKER_FILE is not set,
    write file to default location ./checker_report.txt

aggregate_time
---------------

Aggregates multiple files over the time dimension into one big file.
Does work on files without unlimited dims as well.
Commandlande arguments are <input dir> <output file>,
optionaly third argument can be a comma separated list of variable names
to add the time dimension to.

E.g.
```
aggregate_time /data/swe_L3A_daily/ /tmp/swe_L3A_daily.nc swe
```
