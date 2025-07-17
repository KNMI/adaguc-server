# Instructions on how to install dependencies for red hat

[Back to Developing](../../Developing.md)

```
#/bin/bash
yum update -y && yum install -y \
    epel-release

yum clean all && yum groupinstall -y "Development tools"

yum update -y && yum install -y \
    hdf5-devel \
    netcdf \
    netcdf-devel \
    proj \
    proj-devel \
    udunits2 \
    udunits2-devel \
    make \
    libxml2-devel \
    cairo-devel \
    gd-devel \
    postgresql-devel \
    postgresql-server \
    gdal-devel

```