# To start the adaguc-server with gunicorn

[Back to Developing](../../Developing.md)
```
source env/bin/activate
export ADAGUC_PATH=`pwd`
export ADAGUC_DATASET_DIR=/data/adaguc-datasets
export ADAGUC_DATA_DIR=/data/adaguc-data
export ADAGUC_AUTOWMS_DIR=/data/adaguc-autowms
export ADAGUC_CONFIG=${ADAGUC_PATH}/python/lib/adaguc/adaguc-server-config-python-postgres.xml
export ADAGUC_DB="user=adaguc password=adaguc host=localhost dbname=adaguc"
export PYTHONPATH=${ADAGUC_PATH}/python/python_fastapi_server
gunicorn --bind 0.0.0.0:8080 --workers=4 wsgi:app --disable-redirect-access-to-syslog --access-logfile - --access-logformat 'accesslog %(h)s ; %(t)s ; %(H)s ; %(m)s ; %(u)s ; %(q)s ; %(s)s ; %(M)s ; "%(a)s"'
```