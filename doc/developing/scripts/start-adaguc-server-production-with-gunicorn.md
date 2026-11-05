# To start the adaguc-server with gunicorn

[Back to Developing](../../Developing.md)
```
source env/bin/activate
python -m pip install -r requirements.txt
python -m pip install .

export ADAGUC_PATH=`pwd`
export ADAGUC_DATASET_DIR=/data/adaguc-datasets
export ADAGUC_DATA_DIR=/data/adaguc-data
export ADAGUC_AUTOWMS_DIR=/data/adaguc-autowms
export ADAGUC_CONFIG=${ADAGUC_PATH}/python/adaguc/adaguc-server-config-python-postgres.xml
export ADAGUC_NUMPARALLELPROCESSES=4
export ADAGUC_DB="user=adaguc password=adaguc host=localhost dbname=adaguc"
gunicorn --bind 0.0.0.0:8080 --workers=1 -k adaguc.fastapi_server.no_proxy_headers_uvicorn_worker.NoProxyHeadersUvicornWorker --disable-redirect-access-to-syslog --access-logfile - adaguc.fastapi_server.main:app

```