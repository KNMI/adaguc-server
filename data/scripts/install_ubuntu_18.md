# Install adaguc-server on Ubuntu 18

Step 1: Install the dependencies using the apt package manager:
`sudo bash ./data/scripts/ubuntu_18_install_dependencies.sh `

Step 2: Setup the postgres server:
`sudo bash ./data/scripts/ubuntu_18_setup_postgres.sh `

Step 3: Compile adaguc-server:
`bash compile.sh && bash runtests.sh`

Step 4: Install python wrapper for adaguc-server

```
python3 -m venv env
source env/bin/activate
python3 -m pip install Pillow chardet numpy netcdf4 six requests pillow aggdraw lxml setuptools wheel flask flask_cors gunicorn
pip install ./lib/dist/adaguc-0.0.2.tar.gz
```

Step 5: Start adaguc-server using the python wrapper:

```
source env/bin/activate
export ADAGUC_PATH=`pwd`
export ADAGUC_DATASET_DIR=/data/adaguc-datasets
export ADAGUC_DATA_DIR=/data/adaguc-data
export ADAGUC_AUTOWMS_DIR=/data/adaguc-autowms

python3 ./python/python-adaguc-server/main.py
```

# Verify that adaguc-server is working

### Test a NetCDF file with autoWMS

Copy a test netcdf file and display:
`cp ./data/datasets/testdata.nc /data/adaguc-autowms/`

- You can browse your local instance via autowms using http://adaguc.knmi.nl/adaguc-viewer/?autowms=http://localhost:8080/autowms
- You can now load the test dataset via http://localhost:8080//wms?source=testdata.nc& in http://adaguc.knmi.nl/adaguc-viewer/
- Or directly via: http://adaguc.knmi.nl/adaguc-viewer/?#addlayer('http://localhost:8080//wms?source=testdata.nc&','testdata')

### Test a Dataset configuration with aggregation of MSG HRVIS

Copy a test dataset for satellite imagery

```
cp ./data/config/msg_hrvis_hdf5_example.xml /data/adaguc-datasets
pushd .
cd /data/adaguc-data
wget -nc -r -l2 -A.h5   -I /knmi/thredds/fileServer/,/knmi/thredds/catalog/ 'http://opendap.knmi.nl/knmi/thredds/catalog/ADAGUC/testsets/projectedgrids/meteosat/catalog.html'
popd
export ADAGUC_PATH=`pwd`
export ADAGUC_CONFIG=./data/config/adaguc.vm.xml
export ADAGUC_DATASET_DIR=/data/adaguc-datasets/
export ADAGUC_DATA_DIR=/data/adaguc-data/
export ADAGUC_AUTOWMS_DIR=/data/adaguc-autowms/
bash ./Docker/adaguc-server-updatedatasets.sh msg_hrvis_hdf5_example
```

- You can now load the test dataset via http://localhost:8080//wms?dataset=msg_hrvis_hdf5_example& in http://geoservices.knmi.nl/viewer2.0
- Or directly via: http://geoservices.knmi.nl/viewer2.0/?#addlayer('http://localhost:8080//wms?dataset=msg_hrvis_hdf5_example&','HRVIS')

### Important files

- Adaguc-services configuration file is `${ADAGUC_PATH}/adaguc-services/config/adaguc-services-config.xml`
- Adaguc-services and adaguc-server log file is: `${ADAGUC_PATH}/adaguc-services.log`
- Look in the database via `psql "host=localhost port=5432 user=adaguc password=adaguc dbname=adaguc"`
-

# Stop Adaguc Server:

Use this command to kill the service:

```
sudo kill  `ps -ef | grep -v grep | grep java |  grep adaguc-services | grep root | awk '{print $2}'`
```

# Reset the db:

```
sudo -u postgres psql postgres -c "DROP DATABASE adaguc;" ; sudo -u postgres psql postgres -c "CREATE DATABASE adaguc;"
```
