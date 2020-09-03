FROM openearth/adaguc-server

WORKDIR /adaguc/adaguc-server-master/data/python/
RUN python3 -m pip install --user --upgrade setuptools wheel
RUN python3 -m venv env
RUN source env/bin/activate && python3 -m pip install Pillow chardet numpy netcdf4 six requests pillow aggdraw lxml setuptools &&  python3 setup.py develop
WORKDIR /adaguc/adaguc-server-master/data/python/examples/runautowms
ENTRYPOINT [ "bash", "-c", "source ../../env/bin/activate && python3 run.py && ls result.png" ] 

