Overview
========

Adaguc is currently hosted on github and consists of several components:

-   https://github.com/KNMI/adaguc-services - Java application for
    running adaguc-server from tomcat or as a spring boot application
-   https://github.com/KNMI/adaguc-server - C** server for serving WMS,
    WCS and OpenDAP
-   https://github.com/KNMI/adaguc-viewer - Javascript web client for
    connecting to WMS
-   https://github.com/KNMI/adaguc-datasets - Example dataset
    configurations

To get started with adaguc it is probably easiest to use the
docker-compose solution which uses pre-build docker images with
adaguc-viewer and adagucs-server:

-   [Docker](Docker.md)

ADAGUC Server with Apache HTTP server
-------------------------------------

Adaguc server is a C** application which can be compiled to an
executable. Adaguc-server can not serve data by itself. A webserver like
Apache Tomcat, Apache HTTP server or [Adaguc-services](Adaguc-services.md) is needed
to make it available to the web.

In the past ADAGUC-Server was traditionally run with Apache HTTP server
using a CGI bash script in between. Today (as of 2018-11) we run
adaguc-server using [Adaguc-services](Adaguc-services.md), which is a self contained
java spring boot application. The docker-container of adaguc-server runs
adaguc-server with adaguc-services. The same environment variables as
described below apply.

Below an overview of the ADAGUC server is displayed. The server and
viewer can be use standalone with other tools as well, due to the use of
open standards. When a request to the server is made via the internet,
at first a BASH shell script is called. This script sets a number of
environment variables and finally calls the ADAGUC server executable.
The server on its turn reads out the environment variable (Config file,
log file) and connects to the database and reads in NetCDF files.
Finally the server returns an image or document. The required
environment variables are:

-   ADAGUC_CONFIG - pointer to the configuration file
-   ADAGUC_LOGFILE - pointer where log messages should be stored,
    includes information logs and error logs
-   ADAGUC_ERRORFILE - optional pointer which logs only error messages
-   ADAGUC_FONT - Place where a TrueType font is stored, e.g.
    FreeSans.ttf
-   ADAGUC_DATARESTRICTION - Optional pointer which controls access
    restrictions, by default set to FALSE, can be combinations of
    "ALLOW_WCS|ALLOW_GFI|ALLOW_METADATA|SHOW_QUERYINFO", separated
    with the | token.
    -   FALSE: No restrictions (default, same as
        "ALLOW_WCS|ALLOW_GFI|ALLOW_METADATA")
    -   ALLOW_WCS: Allows the Web Coverage Service, download of data
    -   ALLOW_GFI: Allows GetFeatureInfo requests, e.g. getting
        information about a certain location
    -   ALLOW_METADATA: Allows getting NetCDF header metadata
        information
    -   SHOW_QUERYINFO: When a query has failed, the corresponding
        query will be presented to the user. This feature is disabled by
        default.
-   ADAGUC_PATH - optional, is used as variable susbstitution
    {ADAGUC_PATH} in the configuration files, should point to the
    adagucserver installation
-   ADAGUC_TMP - optional, is used as variable susbstitution
    {ADAGUC_TMP} in the configuration files, location where tempfiles
    need to be written
-   ADAGUC_ONLINERESOURCE - optional, specify the online resource in
    the CGI script itself, see [OnlineResource](OnlineResource.md) to configure in
    the xml file.

![](ADAGUC_Overview_server.jpg)

The server requires the following libraries:
- HDF5, NetCDF4, UDUNITS, GDAL, LIBXML2, GD, Cairo, Proj.4, SQLITE and
PostgreSQL.

Example bash script:
```
\#!/bin/bash

export LD_LIBRARY_PATH=/build/lib/:\$LD_LIBRARY_PATH
export PROJ_LIB=/build/share/proj/
export ADAGUC_CONFIG=/data/services/config/config.xml
export ADAGUC_NUMPARALLELPROCESSES=4
export ADAGUC_LOGFILE=data/log/server.log
export ADAGUC_ERRORFILE=/data/log/server.errlog
export ADAGUC_FONT=/data/fonts/FreeSans.ttf
export ADAGUC_DATARESTRICTION="FALSE"

echo "Access-Control-Allow-Origin: \*"
echo "Access-Control-Allow-Methods: GET"

/data/build/bin/adagucserver
```

Configuration details can be found under [Configuration](Configuration.md), serveral
tutorials are available under [Tutorials](Tutorials.md)

The server is able to read CF compliant files. In case of projected
source data, a number of CF projections are supported. The server uses
the ADAGUC standard for projected files, this means that a proj4_params
attribute is used in the projection definition. See
http://adaguc.knmi.nl/contents/documents/ADAGUC_Standard.html for
details.

The server can read single files, or multiple files varying along a
dimension stored together in a directories. The server uses the database
to lookup which dates belong to which file and time index. After
changing the configuration, the database needs to be filled with data,
this can be done with the following command:
>adagucserver --updatedb --config myconfig.xml

ADAGUC Viewer
-------------

The viewer is an independent and standalone package. After downloading
and extracting the package in a www directory of a webserver, the viewer
should already work. The viewer can be downloaded from the ADAGUC
software download page at
http://adaguc.knmi.nl/contents/documents/ADAGUC_software.html or it can
be cloned from the repository at https://github.com/KNMI/adaguc-viewer

![](ADAGUC_Overview_viewer.jpg)

For the Wiki and documentation see:
https://dev.knmi.nl/projects/adagucviewer/wiki
