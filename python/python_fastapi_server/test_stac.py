import logging

import pytest
from fastapi.testclient import TestClient

from main import app

logger = logging.getLogger(__name__)


@pytest.fixture(name="client")
def fixture_client():
    yield TestClient(app)


def test_stac_root(client: TestClient):
    resp = client.get("/stac")
    assert resp.status_code == 200
    catalog = resp.json()
    assert catalog["type"] == "Catalog"
    assert catalog["stac_version"] == "1.0.0"

    rels = {link["rel"]: link for link in catalog["links"]}
    assert "self" in rels
    assert "root" in rels
    assert rels["self"]["href"] == rels["root"]["href"]

    children = [link for link in catalog["links"] if link["rel"] == "child"]
    titles = {link["title"] for link in children}
    assert titles == {"adaguc::datasets", "adaguc::data", "adaguc::autowms"}
    for link in children:
        assert link["href"] == rels["self"]["href"] + "/catalog/" + link["title"]


def test_stac_catalog_data_listing(client: TestClient):
    resp = client.get("/stac/catalog/adaguc::data")
    assert resp.status_code == 200
    catalog = resp.json()
    assert catalog["type"] == "Catalog"

    rels = {link["rel"]: link for link in catalog["links"]}
    assert rels["parent"]["href"] == rels["root"]["href"]

    item_titles = {link["title"] for link in catalog["links"] if link["rel"] == "item"}
    assert "testdata.nc" in item_titles
    assert "alpha-test.png" in item_titles


def test_stac_catalog_datasets_listing(client: TestClient):
    resp = client.get("/stac/catalog/adaguc::datasets")
    assert resp.status_code == 200
    catalog = resp.json()

    item_links = [link for link in catalog["links"] if link["rel"] == "item"]
    titles = {link["title"] for link in item_links}
    assert "adaguc.testautotiling" in titles
    # Dataset entries are flat, so the .xml suffix must not leak into the title/id.
    assert all(not title.endswith(".xml") for title in titles)


def test_stac_catalog_autowms_listing(client: TestClient):
    resp = client.get("/stac/catalog/adaguc::autowms")
    assert resp.status_code == 200
    catalog = resp.json()
    item_titles = {link["title"] for link in catalog["links"] if link["rel"] == "item"}
    assert "alpha-test.png" in item_titles


def test_stac_catalog_unknown_path(client: TestClient):
    resp = client.get("/stac/catalog/doesnotexist")
    assert resp.status_code == 404


def test_stac_item_png_source(client: TestClient):
    resp = client.get("/stac/item/adaguc::autowms/alpha-test.png")
    assert resp.status_code == 200
    item = resp.json()

    assert item["type"] == "Feature"
    assert item["id"] == "alpha-test.png"
    assert item["bbox"] == [-180.0, -90.0, 180.0, 90.0]
    assert item["geometry"]["type"] == "Polygon"

    assert "https://stac-extensions.github.io/web-map-links/v1.1.0/schema.json" in item["stac_extensions"]

    rels = {link["rel"]: link for link in item["links"]}
    assert rels["service-desc"]["href"].endswith("request=GetCapabilities")
    assert rels["wms"]["wms:layers"] == ["pngdata"]

    assert item["assets"]["data"]["href"] == rels["wms"]["href"].split("service=WMS")[0]
    thumbnail = item["assets"]["thumbnail"]
    assert "request=GetMap" in thumbnail["href"]
    assert "layers=pngdata" in thumbnail["href"]
    assert thumbnail["type"] == "image/png"


def test_stac_item_netcdf_source(client: TestClient):
    resp = client.get("/stac/item/adaguc::data/testdata.nc")
    assert resp.status_code == 200
    item = resp.json()
    assert item["id"] == "testdata.nc"
    assert item["assets"]["data"]["type"] == "application/x-netcdf"
    assert item["assets"]["data"]["href"].endswith("source=testdata.nc&")


def test_stac_item_not_found(client: TestClient):
    resp = client.get("/stac/item/adaguc::data/does-not-exist.nc")
    assert resp.status_code == 404


def test_stac_item_dataset_xml(client: TestClient):
    resp = client.get("/stac/item/adaguc::datasets/adaguc.testautotiling.xml")
    assert resp.status_code == 200
    item = resp.json()
    assert item["id"] == "adaguc.testautotiling"
    assert item["assets"]["data"]["href"].endswith("dataset=adaguc.testautotiling&")


def test_stac_item_tolerates_double_slash(client: TestClient):
    # A double slash right after the prefix must not desync item lookup from the single-slash
    # paths list_data_files() returns, and must not make os.path.join() discard the data dir.
    resp = client.get("/stac/item/adaguc::autowms//alpha-test.png")
    assert resp.status_code == 200
    assert resp.json()["id"] == "alpha-test.png"


def test_stac_catalog_rejects_percent_encoded_traversal(client: TestClient):
    resp = client.get("/stac/catalog/adaguc::data/%2e%2e/etc")
    assert resp.status_code == 400


def test_stac_item_rejects_percent_encoded_traversal(client: TestClient):
    resp = client.get("/stac/item/adaguc::data/%2e%2e/etc/passwd")
    assert resp.status_code == 400


def test_stac_catalog_missing_subdir_is_404_not_500(client: TestClient):
    resp = client.get("/stac/catalog/adaguc::data/this-subdir-does-not-exist")
    assert resp.status_code == 404


def test_stac_item_missing_subdir_is_404_not_500(client: TestClient):
    resp = client.get("/stac/item/adaguc::data/this-subdir-does-not-exist/x.nc")
    assert resp.status_code == 404


def test_stac_catalog_rejects_sibling_directory_escape(monkeypatch, tmp_path):
    # A traversal that lands in a sibling directory sharing a name prefix with the configured
    # data dir (e.g. "data" vs "data_secret") must still be rejected: a naive startswith()
    # containment check without a path-separator boundary would wrongly allow this through.
    sandbox = tmp_path / "data"
    sandbox.mkdir()
    secret = tmp_path / "data_secret"
    secret.mkdir()
    (secret / "leaked.nc").write_text("secret")

    monkeypatch.setenv("ADAGUC_DATA_DIR", str(sandbox))
    monkeypatch.setenv("ADAGUC_AUTOWMS_DIR", str(sandbox))

    sandboxed_client = TestClient(app)
    resp = sandboxed_client.get("/stac/catalog/adaguc::data/%2e%2e/data_secret")
    assert resp.status_code == 400

    resp = sandboxed_client.get("/stac/item/adaguc::data/%2e%2e/data_secret/leaked.nc")
    assert resp.status_code == 400
