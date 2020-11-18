# Python wrapper for the adaguc-server

This can be used to run adaguc-server from Python.

## To install adaguc python:

Prerequisites: You need to have the adaguc-server binaries available. The adaguc Python library is using these binaries to work with the adaguc-server.

You can compile adaguc with the `bash compile.sh` command, you will need some depencies in order to compile adaguc. For details look at `sudo bash ./data/scripts/ubuntu_20_install_dependencies.sh `

### Virtual env dependencies:

```
apt-get install python3-venv python3-pip
python3 -m pip install --user --upgrade setuptools wheel
```

### Setup a python3 virtual environment:

```
python3 -m venv env
```

### Activate the virtual environment:

```
source env/bin/activate
```

### Install Adaguc Python dependencies in the virtualenv:

```
python3 -m pip install Pillow chardet numpy netcdf4 six requests pillow aggdraw lxml
```

### Install the adaguc python library

```
cd data/python/
python3 setup.py sdist
pip3 install dist/adaguc-0.0.1.tar.gz
cd ../../
```

### Set the ADAGUC_PATH environment variable which points to your adaguc-server folder (with compiled binaries):

```
export ADAGUC_PATH="${PWD}"
```

### The run.py example scripts should now work:

```
python3 ./data/python/examples/runautowms/run.py
```

## Developing the python wrapper for adaguc-server

To develop the python code from this directory you can do:

In ./adaguc-server/data/python:

```
python3 -m pip install --user --upgrade setuptools wheel
python3 -m venv env
source env/bin/activate
python3 -m pip install Pillow chardet numpy netcdf4 six requests pillow aggdraw lxml setuptools wheel
python3 setup.py develop
```

To create a package of the library do:

```
python setup.py sdist
```

Adaguc needs to be compiled in order to let the examples work.
