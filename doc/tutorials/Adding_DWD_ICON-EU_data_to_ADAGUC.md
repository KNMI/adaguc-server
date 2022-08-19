Adding DWD ICON-EU data to ADAGUC
=================================

In this tutorial we will add some fields from DWD's public ICON-EU model
to ADAGUC. The data will be retrieved from an AWS S3 bucket, converted
to NetCDF with FIMEX (http://fimex.met.no) and then added to the ADAGUC
AutoWMS service.

For retrieving the data we will use the AWS client program, which you
can install by running
```
sudo apt install python-pip
pip install aws-cli
```

The script copyICON.sh (attached below) uses the aws client program to
retrieve data files for a number of parameters and is called like this:

```
mkdir \$HOME/ICON\_data
cd \$HOME/ICON\_data
bash copyICON.sh 2018112200
```
The ICO files should appear in a subdirectory named 2018112200

For conversion to NetCDF we will use fimex, installed in a Docker
container.
Download the attached Dockerfile and use it to build a Docker image for
fimex.
```
docker build -t fimex .
```

This Docker image can be run as followed:
```
docker run --mount type=bind,source=\`pwd\`,target=/app -w /app fimex
fimex --input.config cdmGribReaderConfig\_DWD\_ICON\_EU.xml --input.file
"glob:2018112200/\*grib2" --input.type grib2 --input.printNcML
--ncml.config DWD\_ICON\_EU.ncml --output.type nc4 --output.file
ICON\_2018112200.nc
```

FIMEX needs a few configuration files to describe the conversion:

-   cdmGribReaderConfig\_DWD\_ICON\_EU.xml - describes the grib
    parameters
-   DWD\_ICON\_EU.ncml - describes some changes to be made to the NetCDF
    parameters for removing unwanted dimensions

We can then copy the file the AutoWMS directory for previewing:
```
cp ICON\_2018112200.nc \$HOME/data/adaguc-autowms
```

URL:
http://localhost:3382/adaguc-services/adagucserver?source=ICON\_2018112109.nc&amp;service=wms&amp;request=getcapabilities
