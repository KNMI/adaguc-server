# Install adaguc-server on Ubuntu 18

Step 1: Install the dependencies using the apt package manager:
```sudo bash ./data/scripts/ubuntu_18_install_dependencies.sh ```

Step 2: Setup the postgres server:
```sudo bash ./data/scripts/ubuntu_18_setup_postgres.sh ```

Step 3: Compile adaguc-server:
```bash compile.sh && bash runtests.sh```

Step 4: Setup adaguc-services server:
```sudo bash ./data/scripts/ubuntu_18_setup_adaguc-services.sh```

# Verify that adaguc-server is working

### Test a NetCDF file with autoWMS ###

 Copy a test netcdf file and display:
```cp ./data/datasets/testdata.nc /data/adaguc-autowms/```

* You can now load the test dataset via http://localhost:8080//wms?source=testdata.nc& in http://geoservices.knmi.nl/viewer2.0
* Or directly via: http://geoservices.knmi.nl/viewer2.0/?#addlayer('http://localhost:8080//wms?source=testdata.nc&','testdata')

### Test a Dataset configuration with aggregation of MSG HRVIS ###

Copy a test dataset for satellite imagery
```
cp ./data/config/msg_hrvis_hdf5_example.xml /data/adaguc-datasets
pushd .
cd /data/adaguc-data
wget -nc -r -l2 -A.h5   -I /knmi/thredds/fileServer/,/knmi/thredds/catalog/ 'http://opendap.knmi.nl/knmi/thredds/catalog/ADAGUC/testsets/projectedgrids/meteosat/catalog.html'
popd
export ADAGUC_PATH=`pwd`
export ADAGUC_CONFIG=./data/config/adaguc.vm.xml
bash ./Docker/adaguc-server-updatedatasets.sh msg_hrvis_hdf5_example
```
* You can now load the test dataset via http://localhost:8080//wms?dataset=msg_hrvis_hdf5_example& in http://geoservices.knmi.nl/viewer2.0
* Or directly via: http://geoservices.knmi.nl/viewer2.0/?#addlayer('http://localhost:8080//wms?dataset=msg_hrvis_hdf5_example&','HRVIS')
