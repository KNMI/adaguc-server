# HARMONIE data for Adaguc

This tutorial will show how to prepare and visualize some data from KNMI's HARMONIE weathermodel in ADAGUC

## Download GRIB data files

HARMONIE data files in GRIB format can be downloaded from [KNMI dataplatform](https://dataplatform.knmi.nl)
The input data will be a set of HARMONIE files in the N55 domain (Europe) called HA40_N55_YYYYmmddHHMM_0FF00_GB or the N25 domain (Netherlands) called HA40_N25_YYYYmmddHHMM_0FF00_GB (where FF are the forecast lead times, usually 0-48 and YYYYmmDDHHMM is the analysis or forecast reference time for the model run).
Download a set of these files to your liking into a subdirectory called GRIBS.

## Conversion to NetCDF

For conversion to NetCDF we will use fimex from met.no (see [FIMEX on GitHub](https://github.com/metno/fimex), installed in a Docker container.

Download the [Dockerfile](data/HARMONIE/Dockerfile) and use it to build a Docker image for fimex in the directory where the Dockerfile is with:

> docker build -t fimex .

This Docker image can be run as follows:

> docker run  --mount type=bind,source=`pwd`,target=/app  -w /app fimex  fimex --input.config <CDMCONFIG> --input.file <INPUTSPEC> --input.type grib --input.printNcML  --ncml.config <NCMLCONFIG> --output.type nc4  --output.file <NCOUTPUT>

where:
* CDMCONFIG is the name of a cdmConfigFile in the current directory
* INPUTSPEC is the specification of the input file or files. This can be a single file name or a file glob
* NCMLCONFIG is the name of an NCML file (NetCdf Markup Language), which can (optionally) be used to fix the resulting file (for example remove single-valued dimensions or rename variables)
* NCOUTPUT is the name of the NetCDf output file

In this tutorial we can use [cdmGribReaderConfigHarmN55.xml](data/HARMONIE/cdmGribReaderConfigHarmN55.xml) or [cdmGribReaderConfigHarmN25.xml](data/HARMONIE/cdmGribReaderConfigHarmN25.xml) as CDMCONFIG. Copy one of thes files to your working directory.

For "fixing" the resulting output file we'll use [HarmN55_dims.ncml](data/HARMONIE/HarmN55_dims.ncml) or [HarmN25_dims.ncml](data/HARMONIE/HarmN25_dims.ncml) Copy one of thes files to your working directory.

Let's say we downloaded GRIBFILES for the 202111021200 run of the N25 domain. The file glob will then be: *glob:GRIBS/HA40_N25_202111021200**

The command for running fimex will then be:
> docker run  --mount type=bind,source=`pwd`,target=/app  -w /app fimex  fimex --input.config cdmGribReaderConfigHarmN55.xml --input.file "glob:GRIBS/HA40_N55_202111021200*" --input.type grib --input.printNcML  --ncml.config HarmN55_dims.ncml --output.type nc4  --output.file HA40_N55_202111021200.nc

This will result in a NetCDF file, which we can for example inspect with ncdump:
```
netcdf HARM_N55_202111021200 {
dimensions:
	time = UNLIMITED ; // (49 currently)
	pressure = 12 ;
	height0 = 1 ;
	height1 = 1 ;
	height2 = 6 ;
	height3 = 6 ;
	total_atmosphere = 1 ;
	rlon = 340 ;
	rlat = 340 ;
variables:
	double time(time) ;
		time:long_name = "time" ;
.
.
.
		y_wind__at_10m:grid_mapping = "projection_rotated_ll" ;
		y_wind__at_10m:coordinates = "longitude latitude" ;
	float air_temperature__at_2m(time, rlat, rlon) ;
		air_temperature__at_2m:_FillValue = 9.96921e+36f ;
		air_temperature__at_2m:long_name = "air_temperature__at_2m" ;
		air_temperature__at_2m:standard_name = "air_temperature" ;
		air_temperature__at_2m:units = "K" ;
		air_temperature__at_2m:grid_mapping = "projection_rotated_ll" ;
		air_temperature__at_2m:coordinates = "longitude latitude" ;

// global attributes:
		:Conventions = "CF-1.0" ;
		:institution = "Royal Dutch Meteorological Institute, KNMI" ;
		:source = "HARMONIE" ;
		:title = "HARM_N25" ;
		:min_time = "2021-11-02 12:00:00Z" ;
		:max_time = "2021-11-04 12:00:00Z" ;
		:Expires = "2021-12-02" ;
		:references = "unknown" ;
		:comment = "none" ;
		:history = "2021-11-02T22:40:33 creation by fimex" ;
}
```
## Inspect file by autowms
This file can be copied to the autowms directory of Adaguc to see if everything works.

## Configure dataset for files
This file can also be configured as a dataset. One or more of these files can be copied to the data directory HARM_N55 of this dataset. The parameters in the file can then be configured for visualisation in the dataset configuration file [HARM_N55.xml](data/HARMONIE/HARM_N55.xml)
This file only configures a few layers. The layers icing_index and vertical_integral_of_cloud_liquid are left out as an exercise.

