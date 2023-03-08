# Instructions on how to install dependencies for Ubuntu 22

[Back to Developing](../../Developing.md)

```
#/bin/bash

sudo apt-get update

sudo apt-get install cmake python3-lxml postgresql libcurl4-openssl-dev libcairo2-dev libxml2-dev libgd-dev postgresql-server-dev-all postgresql-client libudunits2-dev udunits-bin g++ m4 netcdf-bin libnetcdf-dev libhdf5-dev libsqlite3-dev python3-netcdf4 python3-dev sqlite3 libsqlite3-dev 
```

## Until adaguc is moved to newer proj api, the following needs to be done too:

Make sure no system proj can be used:

```
sudo apt-get remove libproj-dev proj-bin libproj22
```

In the adaguc-server folder do:
```
# Install PROJ
mkdir customlibs && cd customlibs
export CUSTOMLIBS=`pwd`/build
export LD_LIBRARY_PATH=${CUSTOMLIBS}/lib:$LD_LIBRARY_PATH
export PATH=${CUSTOMLIBS}/bin:$PATH
export PROJ_ROOT=${CUSTOMLIBS}
export LDFLAGS=-L${CUSTOMLIBS}/lib
export CPPFLAGS=-I${CUSTOMLIBS}/include

wget https://download.osgeo.org/proj/proj-7.2.0.tar.gz
tar -xzvf proj-7.2.0.tar.gz 
cd proj-7.2.0/
./configure --prefix ${CUSTOMLIBS}
make -j4
make install

cd .. # Into the customlibs directory again.

# Install GDAL
wget https://download.osgeo.org/gdal/3.4.0/gdal-3.4.0.tar.gz
tar -xzvf gdal-3.4.0.tar.gz
cd gdal-3.4.0/
./configure --prefix ${CUSTOMLIBS}
make -j6
make install
```

Do this each time for a new terminal/shell
```
# From the adaguc-server root folder:
export CUSTOMLIBS=`pwd`/customlibs/build
ls ${CUSTOMLIBS}/include -l $ should list header .h files
ls ${CUSTOMLIBS}/lib -l $ should list lib .so files
export LD_LIBRARY_PATH=${CUSTOMLIBS}/lib:$LD_LIBRARY_PATH
export PATH=${CUSTOMLIBS}/bin:$PATH
export PROJ_ROOT=${CUSTOMLIBS}
export PROJ_LIBRARY=${CUSTOMLIBS}
export LDFLAGS=-L${CUSTOMLIBS}/lib
export CPPFLAGS=-I${CUSTOMLIBS}/include
export GDAL_INCLUDE_DIR=${CUSTOMLIBS}/include
export GDAL_ROOT=${CUSTOMLIBS}
``

Please note: When the adaguc-server binary is compiled, it should only make use of proj .so files in the ${CUSTOMLIBS} folder. You can check this with `ldd ./bin/adagucserver | grep proj`.