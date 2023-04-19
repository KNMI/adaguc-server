**Version 2.8.7 2023-04-19**
- GeoJSON with labels can now be displayed. See [FeatureInterval](doc/configuration/FeatureInterval.md) for details

**Version 2.8.6 2023-03-23**
- Added a configuration option to limit the number of deletions done at once, configurable with `cleanupsystemlimit="5"`  in [Settings](doc/configuration/Settings.md).
- Added a configuration option to start a dryrun of cleanup, explaining which files would be deleted (but not actually deleting them), configurable with `enablecleanupsystem="dryrun"` in [Settings](doc/configuration/Settings.md)
- Added the option to replace custom variables in the dataset configuration using the [Environment](doc/configuration/Environment.md) keyword. These will get the value from the environment. This has been added to make it possible to let dev branches have a shorter retentionperiod and the main branch a longer retentionperiod. 
- See complete dataset example in [data/config/datasets/adaguc.tests.cleandb-step2.xml](data/config/datasets/adaguc.tests.cleandb-step2.xml)


**Version 2.8.5 2023-03-15**
- Add parameter Unit of Measurement to DescribeCoverage output 

**Version 2.8.4 2023-03-08**
- Nearest neighbour rendering can now be done with discrete classes using ShadeInterval and renderhint="discreteclasses"
- Hex colors now work with both uppercase and lowercase hex digits
- Legend is now not mandatory in a Style definition
- Drawfunction is refactored and de-duplicated, less double code.
- A buffer overflow issue with shadeintervals rendering has been solved (adagucserverEC/CImgWarpNearestNeighbour.h, 222)

**Version 2.8.3 2023-02-15**
- Build docker image for arm64 architecture (e.g. Mac M1)

**Version 2.8.2 2023-01-10**
- WebP quality is now configurable via WMSFormat and WMS Format (use format=image/webp;90) 
- PNG can now read more image types (PNG color_type=6)

**Version 2.8.1 2022-12-24**
- Using debian as base docker image

**Version 2.8.0 2022-12-23**
- Support for building Adaguc on Mac M1 architecture (arm64) 

**Version 2.7.13 2022-12-20**
- Support for HDF5 ODIM files containing one dataset with lat/lon and time.

**Version 2.7.12 2022-12-19**
- Rolling archives are now possible by setting the retentionperiod in the FilePath setting of the layer configuration

**Version 2.7.11 2022-12-15**

- The Docker/adaguc-server-addfile.sh script does now exit with statuscode 1 if the adding of a file failed.

**Version 2.7.10 2022-11-11**

- Web Coverage Service can determine width and height or resx or resy parameters on its own

**Version 2.7.9 2022-09-22**

- GeoJSON reader now supports multiple dataobjects, useful for styling earthquake files with age and magnitude variables.

**Version 2.7.8 2022-09-21**

- Updated documentation

**Version 2.7.7 2022-09-19**

- Added OGCAPI support
- Added RADAR volume scan support
- GeoJSON reader now supports featuretype Point.

**Version 2.7.6 2022-07-12**

- Fix: Legend with a logaritmic scale do work again.

**Version 2.7.5 2022-07-11**

- Implemented quantize for time on ingest too (for rounding product times up/down to values spaced with fixed intervals)

**Version 2.7.4 2022-07-06**

- Continuous Legend graphic supports inverted min and max

**Version 2.7.3 2022-04-13**

- Legend graphic code splitted and refactored

**Version 2.7.2 2022-04-06**

- Points can now be rendered with customizable and scalable symbols

**Version 2.7.0 2022-04-04**

- The time dimension for a layer in a GetCapabilities document can now be set based on the modification time of the file by configuring a dimension using: `<Dimension default="filetimedate">time</Dimension>`

**Version 2.6.9 2022-03-17**

- Support for pointstyle radiusandvalue, to display earthquakes with a disc depending on magnitude and color depending on age.

**Version 2.6.8 2022-02-07**

- Adding GitHub workflow to push automatically to dockerhub


**Version 2.6.7 2022-01-20**
- Support for KNMI HDF5 echotops files (see data/datasets/test/RAD_NL25_ETH_NA_202004301315.h5). When such a file is used, a virtual variable with name "echotops" is available.

**Version 2.6.6 2021-12-31**
- Formatted code with clang-formatter and resolved all warnings.


**Version 2.6.5 2021-12-23**
- Rendermethod `hillshaded` and `generic` has been added. Demo will follow.

**Version 2.6.4 2021-12-23**
- Tiling system has been refactored and has an extra option called autotile="true". This allows to generating tiles during file scanning with updatedb.

**Version 2.6.3 2021-11-30**
- HDF reader now reads variable length string attributes. These are usually written with h5py, using type H5T_C_S1.

**Version 2.6.2 2021-11-22**
- Adjusted logging format, differentiating between acces log and application log.
Version 2.6.1 2021-11-09
- CSV reader now detects floats, ints and strings properly with RegExp (thanks EduardoCastanho)

**Version 2.6.0 2021-11-01**
- Moving to a python wrapper to run adaguc-server. For development with flask, for production with gunicorn.

**Version 2.5.15 2021-09-22**
- The /data folder can now completely be mounted from another filesystem, all adaguc internal folders have moved to /adaguc.

**Version 2.5.14 2021-09-17**
- WCS GetCoverage with Native parameters now returns data
- PNG reader can extraxt bbox, time and crs from png text metadata properties

**Version 2.5.13 2021-07-22**
- Extended logging options:
    - ADAGUC_ENABLELOGBUFFER=FALSE: unbuffered output, gives realtime logging. Unbuffered output can cause a slower service
    - ADAGUC_ENABLELOGBUFFER=TRUE: buffered logging
    - ADAGUC_ENABLELOGBUFFER=DISABLELOGGING: no logging at all

**Version 2.5.11 2021-05-12**
- Correct rendering of IPCC data on North and South pole projections

**Version 2.5.9 2021-05-11**
- Added tests for custom projection of Robinson, EPSG:3411 and EPSG:3413 for handling correct visualization of CMIP6 data for IPCC atlas by Predictia and Unican

**Version 2.5.8 2021-04-07**
- Debug logging can be switched off from a dataset configuration

**Version 2.5.7 2021-04-07**
- Contourlines can now be rendered without text

**Version 2.5.4 2020-11-27**
- Improved drawing of contourlines for new climate normal maps.

**Version 2.5.3 2020-11-18**
- More resilient reading of KNMI HDF5 files

**Version 2.5.2 2020-11-13**
- Support to run Adaguc-server from python3.

**Version 2.5.1 2020-11-13**
- Support for 32 contourline definitions (previousely 8)

**Version 2.5.0 2020-11-13**
- Support CMAKE

**Version 2.4.2 2020-11-09**
- Support for Python 3

**Version 2.2.5 2019-10-18**
- Update db with specified path will now also use the filter specified in the layer.

**Version 2.2.4 2019-09-25**
- AdditionalLayer functionality is Fixed (https://dev.knmi.nl/projects/adagucserver/wiki/AdditionalLayer)

**Version 2.2.3 2019-09-24**
- The CDF Data model supports more NCML features to support DLR-PA VADUGS files out of the box. In addition it is possible to add a time dimension using NCML. Its value can be created from a global attribute using the timeValueFromGlobalAttribute.
- Anydump has the commandline option -ncml to dump with the specified NCML file

**Version 2.2.2 2019-09-10**
- Updatedb supports the --path parameter to scan files specified by an absolute path

**Version 2.2.1 2019-08-27**
- adaguc-server has a new feature called getmetadata. It can dump a NetCDF file via the source parameter and return a json document containing the NetCDF metadata
- Example: <adaguc host>/wms?source=<URLEncoded opendapURL>&service=metdata&request=getmetadata
- adaguc-server supports reading PNG files accompanied by a info file, functional tests are added to demonstrate.

**Version 2.2.0 2019-06-27**
- adaguc-server docker compose uses decoupled postgres, started as separate container
- logfiles are embedded in adaguc-services, single stream of logging
- Improved support for curvilinear grids
- Support for NC_INT64 and NC_UINT64 data types

**Version 2.1.0 2019-04-04**
- Use of new adaguc-services framework version 1.2.0 (Spring boot 2.0)

**Version 2.0.38 2019-03-28**
- CSV reader with support for time and reference time dimensions added
- GeoJSON supports time
- Fixed bug in CRequests causing segmentation fault
- Fixed bug in Tracer which started to write to a closed logfile

**Version 2.0.37 2018-08-29**
- OpenDAP server can now output JSON if header "Content-Type: application/json" is set

**Version 2.0.36 2018-08-29**
- OpenDAP server is now compatible with jsdap JavaScript client

**Version 2.0.35 2018-08-24**
- CTime has now a singleton keeper for reducing CTime intialization times

**Version 2.0.34 2018-07-24**
- Adaguc has ability to generate a report about the files to indicate what choices are made for visualisation and projection

**Version 2.0.33 2018-06-06**
- Dockerfile is now using a multistage build resulting in smaller docker images

**Version 2.0.32 2018-01-22**
- Polylines smaller then 1 px can now be rendered.
- OpenDAPServer is now compatible with Java NetCDF library
- OpenDAPServer tests and polyline tests added

**Version 2.0.31 2017-10-23**
- When using ?DATASET=<yourdateset> configurations the autoscan is now not started, this should be done by you.
- Support for NOAA GOES NetCDF files
- Bug fixing in reading dimensions with scale and offset applied

**Version 2.0.29 2017-10-23**
- Striding of gridfields can now be controlled with attribute striding in RenderSettings (e.g. <Style><RenderSettings striding="4"/></Style>)

**Version 2.0.28 2017-09-10**
- Added 8 functional tests
- Added postgres index on dimension column
- Query limit can now be configured: <DataBase maxquerylimit="1000"/>

**Version 2.0.27 2017-08-16**
- Dimensions without a dimension variable now get an automatically assigned dimension variable, values start with 1 and are increased onwards.
- Initial PNG support with initial slippy map support added

**Version 2.0.26 2017-07-31**
- Added support for LSA SAF hdf5 files for MeteoRomania

**Version 2.0.25 2017-07-28**
- Tilecreation speed is optimized. When optimizeextent is set to true in TileSettings, only the partial area from the big netcdf file is read into memory and used for warping and tilecreation
- The bottom, left, right and top parameters in TileSettings are now optional, when left out they will be automatically detected based on the input file. This simplifies TileSettings if only one input file is needed as input.


**Version 2.0.24 2017-07-28**
- Updatedb has a new flag called --recreate. This drops the filetables and will recreate the them
- Updatedb now synchronizes scanned files with the database per 50 scanned files.

**Version 2.0.23 2017-08-01**
- TileSettings has an extra configuration attribute called "optimizeextent" for optimizing reading small chunks from big grids
- TileSettings debug mode now draw the border of the tiles on the GetMap with lines
- Nearest neighbour renderer mode can be set to precise or fast with RenderSettings element in Styles

**Version 2.0.22 2017-03-27**
- Worked on dockerizing and modularizing
- Added extra configuration examples for adaguc workshop 2017

**Version 2.0.20 2016-11-28**
- CCDFDataModel supports NC_STRING types for attributes

**Version 2.0.19 2016-11-20**
- Tiles can now be generated from very big grids in any cartographic projection
- GeoJSON supports line rendering

**Version 2.0.18 2016-11-06**
- Added linear transformation in Nearest neighbour renderer when geographic mappings are equal.

**Version 2.0.17 2016-10-13**
- Support for POI markers

**Version 2.0.18 2016-10-12**
- Support for 365_day calendars

**Version 2.0.13, 2016-08-15:**
- OpenDAP strings are encoded with two dimensions data(numstrings,maxStrlen64) by the NetCDF library. Internally this is now translated to CDF_STRING
- Anydump is able to list strings
- Timeseries/points are now plotted correctly over opendap

**Version 2.0.12, 2016-08-10:**
- Added support for NetCDF4 groups
- Added anydump tool in CCDFDataModel, make with "make anydump"
- Added initial support for TROPOMI
- Added 360_day calendar support for CLIPC
- Bugfix: GetFeatureInfo now works on Byte data
- Bugfix: Layers with groups can now be served over ADAGUC OpenDAP, group separator is replaced by "_"

**Version 2.0.9, 2016-07-22:**
- AdditionalLayer functionality now works with NetCDF files with two dimensions per variable (only y,x data).
- Added datamask datapostprocessor
- Added include_layer datapostprocessor
- WCS NetCDFDataWriter now supports multiple dataobjects as output.
- WMS GetFeatureInfo can now output multiple data objects when they have the same standard name


**--------**

- Added Dataset configuration option to load additional datasets through the URL
- Added Include configuration option to include additional configuration files

Version 1.4 October 2013
- Inspire view services support

Version 1.3 Augustus 2013
- Added WMS 1.3 support

Version 1.2 (10 June 2013)
- Version made ready for the workshop

Version 1.0.0 (22 August 2011):

   New features:
     *

   Fixed bugs:
     *

