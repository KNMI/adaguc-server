# Python wrapper for the adaguc-server

This can be used to run adaguc-server from Python.

## Prerequisites: 

- You need to have the adaguc-server binaries available. The adaguc Python library is using these binaries to work with the adaguc-server. You can compile adaguc with the `bash compile.sh` command, you will need some dependencies in order to compile adaguc. For details look at `sudo bash ./data/scripts/ubuntu_20_install_dependencies.sh `
- You will need python3 and the ability to create virtualenv with python

## To install the python wrapper:

From the root adaguc-server folder:
```
python3 -m venv env
source env/bin/activate
python3 -m pip install Pillow chardet numpy netcdf4 six requests pillow aggdraw lxml setuptools wheel flask flask_cors gunicorn
python3 ./python/lib/setup.py develop
```

## To start the python flask webserver for adaguc:

From the root adaguc-server folder:
```
source env/bin/activate
export ADAGUC_PATH=`pwd`
export ADAGUC_DATASET_DIR=/data/adaguc-datasets
export ADAGUC_DATA_DIR=/data/adaguc-data
export ADAGUC_AUTOWMS_DIR=/data/adaguc-autowms
python3 ./python/python-adaguc-server/main.py
```

## Reminder on how to install a python virtual env:

```
apt-get install python3-venv python3-pip
python3 -m pip install --user --upgrade setuptools wheel
# Setup a python3 virtual environment:
python3 -m venv env
source env/bin/activate
```