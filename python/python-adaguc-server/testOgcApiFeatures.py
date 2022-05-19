import pytest
from main import create_app
import json
import sys
import tempfile
import os
import shutil
import logging
from adaguc.AdagucTestTools import AdagucTestTools
import unittest

logger = logging.getLogger(__name__)

def set_environ():
    tempdir = tempfile.mkdtemp()
    # print("tmpdir: %s", tempdir)
    os.environ["ADAGUC_CONFIG"]=os.path.join(os.environ["ADAGUC_PATH"], 'data', 'config', "adaguc.ogcfeatures.xml")

    return tempdir

def setup_test_data(tmpdir):
    datasetdir=os.environ["ADAGUC_DATASET_DIR"]

    print("About to ingest data")
    AdagucTestTools().cleanTempDir()
    status, data, headers = AdagucTestTools().runADAGUCServer(
        args=['--updatedb', '--config',
                os.environ["ADAGUC_CONFIG"]+","+"netcdf_5d.xml"],
        isCGI=False,
        showLogOnError=False,
        showLog=True)

    return None

@pytest.fixture
def app():
    #Initialize adaguc-server
    tempdir=set_environ()
    print("tempdir", tempdir)
    setup_test_data(tempdir)
    app = create_app()
    yield app
    print("tempdir",tempdir)
# shutil.rmtree(tempdir)

@pytest.fixture
def client(app):
    with app.test_client() as client_instance:
        yield client_instance

def test_root(client):
    resp = client.get("/adaguc-server?dataset=netcdf_5d&request=getcapabilities&service=wms&version=1.3.0")
    print("getcap:", resp.data)

    resp = client.get("/ogcapi/")
    print("resp:", resp, resp.json)
    assert resp.json["description"]  ==  'ADAGUC OGCAPI-Features server demo'
    print(">>>>>>")

    resp = client.get("/ogcapi/collections")
    colls = resp.json
    assert len(colls["collections"])==1
    print("resp:", resp, resp.json)

def test_collections(client):
    resp = client.get("/ogcapi/collections")
    colls = resp.json
    assert len(colls["collections"])==1
    #print("resp:", resp, resp.json)
