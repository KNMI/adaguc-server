"""
Tests WMS GetMap via POST with form url encoded parameters, and the STYLESHEET parameter.
"""

import os
from importlib import reload
from urllib.parse import parse_qsl, urlencode
from unittest.mock import patch
from fastapi.testclient import TestClient
import main
import pytest

GET_MAP_QUERY = "source=testdata.nc&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE"
EXPECTED_GET_MAP = os.path.join(os.environ["ADAGUC_PATH"], "tests", "expectedoutputs", "TestWMS", "test_WMSGetMap_testdatanc.png")
# A valid stylesheet with characters that would break naive query string handling: &, =, %, + and a newline.
# Only a Legend that no style uses, so the image does not change.
STYLESHEET = '{"Legend": {"@name": "x a&b", "#text": "\\n  &WIDTH=1 100%+"}}'


@pytest.fixture(autouse=True)
def isolate_environment():
    """This fixture restores the original env vars after each test"""
    with patch.dict(os.environ):
        yield


@pytest.fixture(name="client")
def fixture_client():
    """Testclient with the autoresource configuration"""
    os.environ["ADAGUC_CONFIG"] = os.path.join(os.environ["ADAGUC_PATH"], "data", "config", "adaguc.autoresource.xml")
    os.environ["ADAGUC_STYLESHEET_ENABLE"] = "TRUE"
    reload(main)
    return TestClient(main.app)


def read_expected():
    with open(EXPECTED_GET_MAP, "rb") as f:
        return f.read()


def test_getmap_post_form_urlencoded(client):
    response = client.post("/wms", data=dict(parse_qsl(GET_MAP_QUERY)))
    assert response.status_code == 200
    assert response.content == read_expected()


def test_getmap_post_with_stylesheet(client):
    params = dict(parse_qsl(GET_MAP_QUERY))
    params["STYLESHEET"] = STYLESHEET
    response = client.post("/wms", data=params)
    assert response.status_code == 200
    assert response.content == read_expected()


def test_getmap_get_with_stylesheet(client):
    response = client.get("/wms?" + GET_MAP_QUERY + "&" + urlencode({"STYLESHEET": STYLESHEET}))
    assert response.status_code == 200
    assert response.content == read_expected()


def test_getmap_post_merges_query_and_body(client):
    query, body = GET_MAP_QUERY.split("&LAYERS=")
    response = client.post("/wms?" + query, data=dict(parse_qsl("LAYERS=" + body)))
    assert response.status_code == 200
    assert response.content == read_expected()


def test_post_requires_form_urlencoded(client):
    response = client.post("/wms", content=GET_MAP_QUERY, headers={"Content-Type": "text/plain"})
    assert response.status_code == 415
