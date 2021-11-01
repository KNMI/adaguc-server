# Python wrapper for the adaguc-server

This can be used to run adaguc-server from Python.

## Prerequisites:

- You will need python3 and the ability to create virtualenv with python
- You need to have the adaguc-server binaries available. The adaguc Python library will use these binaries to work with the adaguc-server. You can compile adaguc in your root adaguc-server directory with the `bash compile.sh` command. Building adaguc requires some dependencies in order to compile adaguc. For details look at the data/scripts directory in the root adaguc folder, there you can find some scripts to install dependencies and setup Postgres for Ubuntu18/20 and RedHat.


## To install the python wrapper:

From the root adaguc-server folder:
```
python3 -m venv env
source env/bin/activate
python3 -m pip install Pillow chardet numpy netcdf4 six requests pillow aggdraw lxml setuptools wheel flask flask_cors gunicorn
cd ./python/lib/ && python3 setup.py develop && cd ../../
```
This will create and activate a `env` virtualenv directory in your adaguc root folder.
## To start the python flask webserver for adaguc:

From the root adaguc-server folder:
```
source env/bin/activate
export ADAGUC_PATH=`pwd`
export ADAGUC_DATASET_DIR=/data/adaguc-datasets
export ADAGUC_DATA_DIR=/data/adaguc-data
export ADAGUC_AUTOWMS_DIR=/data/adaguc-autowms
export ADAGUC_CONFIG=${ADAGUC_PATH}/python/lib/adaguc/adaguc-server-config-python-postgres.xml
export ADAGUC_DB="user=adaguc password=adaguc host=localhost dbname=adaguc"
python3 ./python/python-adaguc-server/main.py
```

Note: the data directories cannot point to a symbolic link, for security purposes adaguc checks if the path contains no symbolic links.

## To scan the datasets

`bash ./Docker/adaguc-server-updatedatasets.sh <dataset name>`

## Reminder on how to install a python virtual env:
## Reminder on how to install a python virtual env on Ubuntu:

```
apt-get install python3-venv python3-pip
python3 -m pip install --user --upgrade setuptools wheel
# Setup a python3 virtual environment:
python3 -m venv env
source env/bin/activate
```

## To start the adaguc-server with gunicorn

source env/bin/activate
export ADAGUC_PATH=`pwd`
export ADAGUC_DATASET_DIR=/data/adaguc-datasets
export ADAGUC_DATA_DIR=/data/adaguc-data
export ADAGUC_AUTOWMS_DIR=/data/adaguc-autowms
export ADAGUC_CONFIG=${ADAGUC_PATH}/python/lib/adaguc/adaguc-server-config-python-postgres.xml
export ADAGUC_DB="user=adaguc password=adaguc host=localhost dbname=adaguc"
export PYTHONPATH=${ADAGUC_PATH}/python/python-adaguc-server
gunicorn --bind 0.0.0.0:8080 --workers=16 wsgi:app --disable-redirect-access-to-syslog --access-logfile -