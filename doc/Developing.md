# Developing

[Back to readme](../Readme.md)

For developing the code locally, you need:
* Compile the C++ adaguc-server
* A postgresql server
* Start the application with the python wrapper.
* Test the server with geographical referenced testdata

After the python wrapper is started, the adaguc-server is accessible on your workstation via http. The easiest way to explore datasets is via de autowms feature, which will give you an overview of available data on your machine via de browser.

## 1. Install packages needed by adaguc-server

To be able to compile adaguc-server you need to have the required dependencies installed. These can be installed via the package manager of your system. Scripts are available:

- [Dependencies for redhat](./developing/scripts/redhat_install_dependencies.md)
- [Dependencies for Ubuntu18](./developing/scripts/ubuntu_18_install_dependencies.md)
- [Dependencies for Ubuntu20](./developing/scripts/ubuntu_20_install_dependencies.md)
- [Dependencies for Ubuntu22](./developing/scripts/ubuntu_22_install_dependencies.md)
- [Dependencies for Mac](./developing/scripts/mac_install_dependencies.md)

## 2. Setting up the postgresql server

We provide several scripts to setup postgresql on your machine:

- [Postgres for redhat](./developing/scripts/redhat_setup_postgres.md)
- [Postgres for Ubuntu18](./developing/scripts/ubuntu_18_setup_postgres.md)
- [Postgres for Ubuntu20](./developing/scripts/ubuntu_20_setup_postgres.md)
- [Postgres for Ubuntu22](./developing/scripts/ubuntu_22_setup_postgres.md)

Or alternatively, run a postgresql database using docker:
```shell
docker run --rm -d \
    --name adaguc_db_dev \
    -e POSTGRES_USER=adaguc \
    -e POSTGRES_PASSWORD=adaguc \
    -e POSTGRES_DB=adaguc \
    -p 5432:5432 \
    postgres:13.4
```

When started, the database is available via username adaguc, databasename adaguc, password adaguc, and localhost. You can use the following to inspect the database:

`psql "dbname=adaguc user=adaguc password=adaguc host=localhost"`


## 3. Install python dependencies

To make the application accesible via the web, a python wrapper is available. This requires at least python 3.8 and the ability to create a virtualenv with python.

To install virtualenv please check [Virtual env on ubuntu](./developing/scripts/virtual-env-with-ubuntu.md)

You have to do once:

```
python3 -m venv env
source env/bin/activate
pip3 install --upgrade pip pip-tools
pip3 install -r requirements.txt
pip3 install -r requirements-dev.txt
cd ./python/lib/ && python3 setup.py develop && cd ../../
```
Make sure the data directories are available:

``` 
sudo mkdir -p /data/adaguc-data     # For data files connected to dataset configurations
sudo mkdir -p /data/adaguc-autowms  # For Exploring your own data
sudo mkdir -p /data/adaguc-datasets # For XML dataset config files
sudo chown $USER: /data -R 
```

And after each restart you only have to do

```
source env/bin/activate
export ADAGUC_PATH=`pwd`
export ADAGUC_DATASET_DIR=/data/adaguc-datasets
export ADAGUC_DATA_DIR=/data/adaguc-data
export ADAGUC_AUTOWMS_DIR=/data/adaguc-autowms
export ADAGUC_CONFIG=${ADAGUC_PATH}/python/lib/adaguc/adaguc-server-config-python-postgres.xml
export ADAGUC_NUMPARALLELPROCESSES=4
export ADAGUC_DB="user=adaguc password=adaguc host=localhost dbname=adaguc"
export ADAGUC_ENABLELOGBUFFER=FALSE
```

# 4. compile adaguc server binaries

After the dependencies have been installed you need to execute a script to start the compilation of the adaguc-server binaries.

```
bash compile.sh
```

# 5. Start adaguc-server

```
source env/bin/activate
export ADAGUC_PATH=`pwd`
export ADAGUC_DATASET_DIR=/data/adaguc-datasets
export ADAGUC_DATA_DIR=/data/adaguc-data
export ADAGUC_AUTOWMS_DIR=/data/adaguc-autowms
export ADAGUC_CONFIG=${ADAGUC_PATH}/python/lib/adaguc/adaguc-server-config-python-postgres.xml
export ADAGUC_NUMPARALLELPROCESSES=4
export ADAGUC_DB="user=adaguc password=adaguc host=localhost dbname=adaguc"
export ADAGUC_ENABLELOGBUFFER=FALSE

# To enable core dump generation, additionally do:
#ulimit -c unlimited
#sudo sysctl -w kernel.core_pattern=core-adagucserver #
# Then you can use gdb ./bin/adagucserver core-adagucserver

python3 ./python/python_fastapi_server/main.py
```

The adaguc-server WMS server will then be accessible at http://127.0.0.1:8080/wms. The autowms can be explored at the adaguc-viewer via the following link: https://adaguc.knmi.nl/adaguc-viewer/index.html?autowms=http://localhost:8080/autowms. Keep in mind that you have to disable security, as the server is not running on https.

Note: the data directories cannot point to a symbolic link, for security purposes adaguc checks if the path contains no symbolic links.

Note: For production purposes the server should be started with gunicorn: [Start adaguc-server with gunicorn](./developing/scripts/start-adaguc-server-production-with-gunicorn.md)

You can check the correct functioning of the adaguc-server by starting the functional tests by doing

```
docker compose -f Docker/docker-compose-test.yml up -Vd
bash runtests_psql.sh
```

# 6. Test the server with geographical referenced testdata

Copy a test netcdf file and display:

`cp ./data/datasets/testdata.nc /data/adaguc-autowms/`

- You can browse your local instance via autowms using https://adaguc.knmi.nl/adaguc-viewer/index.html?autowms=http://localhost:8080/autowms
- You can now load the test dataset via https://localhost:8080//wms?source=testdata.nc& in https://adaguc.knmi.nl/adaguc-viewer/index.html
- Or directly via: https://adaguc.knmi.nl/adaguc-viewer/index.html?#addlayer('http://localhost:8080//wms?source=testdata.nc&','testdata')


# 7. Run tests

First start the docker used for testing: 

```
docker compose -f Docker/docker-compose-test.yml up -Vd
```

Then run the tests by doing:

```
bash runtests_psql.sh
```

## To scan datasets


- list datasets: `bash ./scripts/scan.sh -l`
- scan a dataset: `bash ./scripts/scan.sh -d <dataset name>`