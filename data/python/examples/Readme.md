# Running adaguc-server from python


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


# To install adaguc python:

Prerequisites: You need to have the adaguc-server binaries available. The adaguc Python library is using these binaries to work with the adaguc-server.  

You can compile adaguc with the `bash compile.sh` command, you will need some depencies in order to compile adaguc. For details look at ```sudo bash ./data/scripts/ubuntu_18_install_dependencies.sh ```

```
# Setup a python3 virtual environment:
apt-get install python3-venv python3-pip
python3 -m pip install --user --upgrade setuptools wheel
python3 -m venv env

# Activate the virtual environment:
source env/bin/activate

# Install the python dependencies

python3 -m pip install Pillow chardet numpy netcdf4 six requests pillow aggdraw lxml setuptools wheel

# Install the adaguc python library
pip install adaguc-0.0.1.tar.gz

# Set the ADAGUC_PATH environment variable which points to your adaguc-server folder (with compiled binaries)

`export ADAGUC_PATH='/home/plieger/code/github/KNMI/adaguc-server'`

# The run.py example scripts should now work.



```