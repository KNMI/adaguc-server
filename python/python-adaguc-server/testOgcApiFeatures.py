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
    os.environ["ADAGUC_CONFIG"]=os.path.join(os.environ["ADAGUC_PATH"], 'data', 'config', "adaguc.ogcfeatures.xml")


def setup_test_data():
    print("About to ingest data")
    AdagucTestTools().cleanTempDir()
    for service in ["netcdf_5d.xml", "dataset_a.xml"]:
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config',
                    os.environ["ADAGUC_CONFIG"]+","+service],
            isCGI=False,
            showLogOnError=False,
            showLog=True)

    return None

@pytest.fixture
def app():
    #Initialize adaguc-server
    set_environ()
    setup_test_data()
    app = create_app()
    yield app

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

def test_collections(client):
    resp = client.get("/ogcapi/collections")
    colls = resp.json
    assert len(colls["collections"])==2
