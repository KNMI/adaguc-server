**Version ?.?.? - ?**

- Added option ADAGUC_TRACE_TIMINGS to measure the amount spent on db access, file reading and image generation.

**Version 3.1.2. 2025-06-10**

- Prevent invalid filenames in tests

**Version 3.1.1. 2025-06-05**

- Fix occasional time and reftime dependency issues

**Version 3.1.0. 2025-06-02**

- The calculation and rendering of windbarbs is now handled differently. The computation of grid-relative x/y wind components is now handled via a datapostprocessor, named `convert_uv_components`. Use ```<DataPostProc algorithm="convert_uv_components"/>```
- The jacobian transformation code is refactored, see [CImgRenderFieldVectors.md](adagucserverEC/CImgRenderFieldVectors.cpp)
- Wind direction and wind speed based on a vector component are now advertised in the GetFeatureInfo request. In addition the numbers are now displayed with a precision of two digits.
- A new processor named `filter_dataobjects` is added
- A new processor named `metadata_variable` is added
- DataPostProcessors can now also be configured via the style. This could be used to make different style with different units. Like windspeed in kts or m/s.
- StatusFlag class is refactored to a struct, and is no longer a pointer in the StatusFlagList vector
- The json version of GetFeatureInfo now outputs multiple variables and has an additional property called layername.
- GetFeatureInfo response via application/html as shown in adaguc-viewer and geoweb now outputs the 6 different components for vectors when the `convert_uv_components` processor is used.
- see [DataPostProc.md](doc/configuration/DataPostProc.md) for details

**Version 3.0.4. 2025-06-03**

- Fixed the (slightly) incorrect formatting of the model data time range for EDR.
Test is adapted.
- A print statement was removed.

**Version 3.0.3. 2025-05-14**

- Fixed bug where a multidimensional dataset with a configured `DataBaseTable` could not get queried

**Version 3.0.2. 2025-04-17**

- Fixed bug where dbz to rr postprocessor did not handle doubles well

**Version 3.0.1. 2025-04-11**

- Fixed bug where windspeed was not calculated according to u and v components when using style barbshaded or vectorshaded

**Version 3.0.0. 2025-04-07**
- EDR: /cube call (based on WCS)
- EDR: /instances call using reference times
- EDR: support for vertical dimensions (z in EDR requests) for model_levels, pressure_levels etc
- EDR: support for custom dimensions [using custom: namespace] for example for percentile classes or ensemble members
- EDR: By default EDR is now enabled for all datasets and layers. EDR support can be switched on or off for a dataset or for a layer. Setting `enable_edr="false"` to a dataset's `<Settings>` element disables EDR for a dataset. Using `enable_edr="false"` in a layer's `<Layer>` element disables EDR for that layer. Using  enable_edr="true" in a `<Layer>` element forces enabling EDR for a single layer, even if it is disabled in the dataset `<Settings>` element.
- EDR: getMetadata request output has been extended with a few attributes for EDR support
- EDR: uses getMetadata request which reads from database for increased performance as no dataset configuration needs to be read. the. This should make response times of EDR calls faster.
- EDR:  now support requests for generating soundings, ensemble plots (percentiles or plumes), time series, returning a CoverageCollection with multiple Coverages or a single Coverage in case of a single result.
- EDR: dimensions can be configured to be `hidden` with the attribute `hidden="true"` in the `<Dimension>` element. These dimensions will be marked as hidden in getMetadata output and do not have to be specified for EDR requests like `/cube` or `/position`. Hidden dimensions do not appear in EDR output. Hidden dimensions are useful when layers have a single value dimension (like a single height).
-  EDR: dimensions can by typed in the configuration by specifying the `dim_type` attribute of the `<Dimension>` elements. The type can be `time`, `reference_time`, `vertical` or `custom`. Vertical dimensions end up as z in EDR. A layer can have a single custom dimension, for example for ensemble member.
- See docs: https://github.com/KNMI/adaguc-server/blob/master/doc/configuration/EDRConfiguration/EDR.md

**Version 2.32.0 2025-03-31**
- Added odim hdf5 volume data support, as extension to existing knmi hdf5 volume data support.

**Version 2.31.3 2025-03-24**
- Fix units in legend for toknots data post processor

**Version 2.31.2 2025-03-19**
- Add debugging config for vscode (launch.json)

**Version 2.31.1 2025-03-19**
- Fix tests for convert h5 volscan not working for psql

**Version 2.31.0 2025-03-19**
- Refactored scan scripts

**Version 2.30.1 2025-03-19**
- A request which results in exceeding the `maxquerylimit` (default=512) will now return HTTP 422

**Version 2.30.0 2025-03-18**
- Improve knmi hdf5 volume scan support, so that more files and variables are supported and projection is more accurate

**Version 2.29.6 2025-03-17**
- Sorting function for timeseries now works without causing intermittent crashes

**Version 2.29.5 2025-02-14**
- Timeseries are now sorted properly by key.

**Version 2.29.2 2024-12-23**
- Fixed issue with irregular grids for SSS SMOS L4 OI - LOPS-v2021 and Global Ocean - Coriolis Observation Re-Analysis CORA5.2. Added test (Issue #316)

**Version 2.29.1 2024-12-23**
- Default value for time dimension can now be based on default value of reference_time dimension (Issue #403)

**Version 2.29.0 2024-11-11**
- Adds support for grids where the data cells are defined by lat_bnds and lon_bnds parameters describing the cell bounds.

**Version 2.28.5 2024-11-04**
- Added Solar Terminator post-processor to use in combination with LiveUpdate layer type. 

**Version 2.28.4 2024-11-12**
- Fix bug where autofinddataset would not use Layer configurations which do not have a type configured

**Version 2.28.3 2024-10-24**
- Fix bug where directory reader could not figure out the file type (opendir ent->d_type == DT_UNKNOWN)

**Version 2.28.2 2024-10-23**
- Fix bug which caused sheduled metadata updates to only work intermittently.

**Version 2.28.1 2024-10-11**
- Windbarbs on modelfields can now display the windspeed in kts as text when rendertextforvectors in RenderSettings is set to true.

**Version 2.28.0 2024-09-11**
- Metadata for layer items like variables, projections, dimensions and styles are now stored in a database table called `layermetadata`. This can be disabled via the `enablemetadatacache` property in [Settings](doc/configuration/Settings.md).

**Version 2.27.0 2024-09-02**
- PostgreSQL query from `getFilesAndIndicesForDimensions` has been rewritten, which fixes https://github.com/KNMI/adaguc-server/issues/341.
- Optimized existing PostgreSQL queries and reduced number of PostgreSQL queries in general. This results in better performance, the benchmark tool runs 9% faster.

**Version 2.26.0 2024-07-12**
- Added EDR cube call for gridded datsets to ADAGUC.

**Version 2.25.0 2024-07-11**
- Fixed issue in EDR position / timeseries functionality via WMS GetFeatureInfo on multidimensional data when selecting multiple elevations at the same time. Added testcases for testing timeseries.

**Version 2.24.0 2024-07-09**
- Fixed occasional 404's in GetCapabilities: Statuscode is now kept to 200 for GetCapabilities and the corresponding error message for the specific layer is embedded in the document

**Version 2.23.0 2024-06-07**
- DataPostProcessor `windspeed_knots_to_ms` was added to convert knots to meters per second, use `<DataPostProc algorithm="windspeed_knots_to_ms"/>`
- TimeSeries fetching with GetFeatureInfo now support multimodel ensembles
- DataPostProcessor has been refactored

**Version 2.22.0 2024-05-22**
- EDR: Parameters can now be detailed with metadata like standard_names and units: https://github.com/KNMI/adaguc-server/issues/359. 
- See [Configure_EDR_service](doc/tutorials/Configure_EDR_service.md) for details.

**Version 2.21.3 2024-04-26**
- Fixed intermittent issue with ODIM reader being enabled/disabled for hdf files

**Version 2.21.2 2024-04-26**
- When using docker compose the Redis container now automatically starts when system restarts

**Version 2.21.1 2024-04-10**
- Support INT64 in CDFDataWriter

**Version 2.21.0 2024-03-14**

- Added support for Redis caching. Redis caching can be enabled by providing a Redis service via the ADAGUC_REDIS environment and configuring caching settings for a dataset in the [Settings](doc/configuration/Settings.md) element.
- Improved speed of EDR service and added support to cache EDR calls
- Various performance improvements

**Version 2.20.2 2024-02-28**
- Removed locking mechanism

**Version 2.20.1 2024-02-20**
- Fixed sometimes failing detection of irregular grids

**Version 2.20.0 2024-02-15**
- Data postprocessor to calculate WFP is added

**Version 2.19.0 2024-02-14**
- Support Irregular grids based on 1D lat/lon variables
- Support Irregular grids based on 2D lat/lon variables

**Version 2.18.0 2024-02-14**
- Support live update layer, displays a GetMap image with the current time per second for the last hour.

**Version 2.17.0 2024-02-14**
- Colors in GetMap images now matches exactly the colors configured in the legend and style in png32 mode
- CSV reader is more flexible
- Sub configuration files (via include) can now substitute variables as well

**Version 2.15.1 2024-01-22**
- Support time/height profile data from https://dataplatform.knmi.nl/dataset/ceilonet-chm15k-backsct-la1-t12s-v1-0

**Version 2.15.0 2024-01-29**
- PostgreSQL queries have been optimized

**Version 2.14.3 2024-01-19**
- Opendap services are accessible again in the Layer configuration: https://github.com/KNMI/adaguc-server/issues/315

**Version 2.14.2 2024-01-15**
- Fix issue where the wrong dimension was forced to a value
- Add Cache-Control header to WCS requests (DescribeCoverage and GetCoverage)
- Fix Cache-control when dimensions are forced to a value
- Make AAIGRID comparison looser (ignore whitespace)
- Add unit test for Cache-Control headers for WCS AAIGRID format

**Version 2.14.1 2023-12-08**
- Set keep-alive (to support running behind proxy/load balancer)

**Version 2.14.0 2023-11-23**
- EDR support (position endpoint only), see [EDRConfiguration](doc/configuration/EDRConfiguration/EDR.md)

**Version 2.13.9 2023-11-01**
- Cache-Control header can be configured via the Settings item

**Version 2.13.8 2023-11-01**
- Fixed issue #311, dimension units are now propagated from the Layer configuration to the GetCapabilities

**Version 2.13.7 2023-10-27**
- PNG files can now also contain a reference_time text tag
- Finnish national projection (EPSG:3067)

**Version 2.13.6 2023-10-06**
- Restored access log to original format (was broken since move to FastAPI)

**Version 2.13.5 2023-10-05**
- KNMI HDF5 dual-pol data now picks scans 15, 16, 6,
14, 5, 13, 4, 12, 3
- KNMI HDF5 dual-pol now derives ZDR as Z-Zv

**Version 2.13.4 2023-10-04**
- Improved Dockerfile multistage build leading to smaller images
- Updated Python package versions

**Version 2.13.3 2023-09-28**
- Reverted outline barbs, added outline option for point barbs

**Version 2.13.2 2023-09-25**
- Fixed a bug in autofinddataset option

**Version 2.13.0 2023-09-20**
- The script adaguc-server-addfile.sh is now able to find for a given file the corresponding dataset via the commandline option autofinddataset
- Cleaning of model data is now done based on the forecast_reference_time dimension


**Version 2.12.0 2023-08-24**
- Support for DataPostProcessor Operator, to add, substract, multiply or divide layers
- Dimensions can now be fixed and hidden, to make layers representing a single elevation based on a variable with multiple elevations
- Support to substract two elevations from the same variable

**Version 2.11.4 2023-09-06**
- Cleanup is now always triggered when a file is scanned.

**Version 2.11.3 2023-09-05**
- Less logging and robuster scanner

**Version 2.11.2 2023-08-21**

- Hot fix: Accidently removed necessary python dependencies in Docker build

**Version 2.11.1 2023-08-21**

- Avoid double logging
- Timeout to prevent endless running of processes
- Fix encoded proj4_params issue https://github.com/KNMI/adaguc-server/issues/279

**Version 2.11.0 2023-08-17**
- Update Docker base image to Python 3.10 based on Debian 12 ("bookworm")

**Version 2.10.5 2023-06-02**
- Contourlines can have dashes
- Contour text can have an outline
- Windbarbs have a white outline

**Version 2.10.4 2023-06-01**
- Fix: Racmo datasets with rotated_pole projection does work again

**Version 2.10.3 2023-05-17**
- Fix: Empty CRS was advertised in case of point data

**Version 2.10.2 2023-05-17**
- Fix: Geos projection without sweep working again

**Version 2.10.1 2023-05-12**
- Added Strict-Transport-Security, X-Content-Type-Options and Content-Security-Policy headers when running over https

**Version 2.10.0 2023-05-10**
- Use the new API of the PROJ library, because the old one is deprecated and removed from PROJ version 8 and up.
Adaguc now requires at least version 6 of the PROJ library.
This should simplify installing on recent version of Ubuntu.

**Version 2.9.0 2023-04-20**
- Using fastapi as server

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

