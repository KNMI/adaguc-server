Adding GOES-16 data to ADAGUC
=============================

ADAGUC can also handle GOES-16 NetCDF data.
GOES-16 NetCDF data is distributed by NOAA through an public dataset on
Amazon S3. See https://registry.opendata.aws/ for more information on
the public datasets available there.

The data is stored in a S3 bucket named noaa-goes16. The AWS S3 bucket
can be accessed through the AWS command-line client, which you invoke
with the command **aws**. This command is available on the ADAGUC
workshop VM and can usually be installed with pip:
```
pip install awscli
```

The AWS client by default wants to use AWS credentials; these can be
entered by running **aws config**. Open datasets can usually be accessed
by anonymous users, by adding the **--no-sign-request** switch to the
command.

Listing the primary folders in the noaa-goes16 bucket:
```
aws s3 ls noaa-goes16/
```

This generates a list of GOES-16 products (the PRE string indicates a
S\# bucket (pseudo)folder name):
```
PRE ABI-L1b-RadC/
PRE ABI-L1b-RadF/
PRE ABI-L1b-RadM/
PRE ABI-L2-CMIPC/
PRE ABI-L2-CMIPF/
PRE ABI-L2-CMIPM/
PRE ABI-L2-MCMIPC/
PRE ABI-L2-MCMIPF/
PRE ABI-L2-MCMIPM/
PRE GLM-L2-LCFA/
```
The folders with names ending in F contain the full-disc data.
For example the ABI-L2-MCMIPF/ folder contains multi-channel (16
channels) data for the full earth disc. In these folders, files are
stored in folders per year and per day of the year. For
example a file could be stored in the folder
noaa-goes16/ABI-L2-MCMIPF/2018/324.
The contents of such a directory can be listed by the command:
```
aws s3 ls noaa-goes16/ABI-L2-MCMIPF/2018/324/12/
```
This command yields a list of filenames, which are rather difficult, so
you might want to copy/paste them:
```
2018-11-20 12:12:59 360805563
OR\_ABI-L2-MCMIPF-M3\_G16\_s20183241200344\_e20183241211122\_c20183241211199.nc
2018-11-20 12:28:56 365359389
OR\_ABI-L2-MCMIPF-M3\_G16\_s20183241215343\_e20183241226122\_c20183241227173.nc
2018-11-20 12:42:59 370656716
OR\_ABI-L2-MCMIPF-M3\_G16\_s20183241230343\_e20183241241110\_c20183241241199.nc
2018-11-20 12:58:01 375511280
OR\_ABI-L2-MCMIPF-M3\_G16\_s20183241245343\_e20183241256110\_c20183241256212.nc
```

As an example let's look at one of these files in the ADAGUC AutoWMS:
Copy the required file from the S3 bucket with a command like:
```
aws s3 cp
s3://noaa-goes16/ABI-L2-MCMIPF/2018/324/12/OR\_ABI-L2-MCMIPF-M3\_G16\_s20183241245343\_e20183241256110\_c20183241256212.nc
\$HOME/data/adaguc-autowms
```

(Don't forget to add s3:// before the filename!)

Refreshing the autowms list should show the copied file in the directory
listing of the AutoWMS panel. The file can then be selected and viewed
in Adaguc.

The selected variable from satellite file will now be shown with a
default color table. Better suited colr tables could be configured
manually.

There are also some quality indicator parameters in the files; these are
usually not very nice to visualize.
