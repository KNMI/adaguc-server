# Running adaguc-server from python


To develop the python code from this directory you can do:

In ./adaguc-server/data/python:
```
python3 -m pip install --user --upgrade setuptools wheel
python3 -m venv env
source env/bin/activate
python3 -m pip install Pillow chardet numpy netcdf4 six requests pillow aggdraw lxml setuptools
python3 setup.py develop
```

Adaguc needs to be compiled in order to let the examples work.
