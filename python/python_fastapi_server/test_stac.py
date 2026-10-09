import logging
from types import SimpleNamespace
from unittest.mock import AsyncMock, patch

import pytest
from fastapi.testclient import TestClient

import routers.stac as stac_module
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

    # The EDR collections link is an additional resource of the datasets catalog, not the root.
    assert "data" not in rels


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

    # adaguc::data entries are source= files with no dataset identity, so no EDR link here.
    assert "data" not in rels


def test_stac_catalog_datasets_listing(client: TestClient):
    # adaguc::datasets lists every dataset flatly again (no separate edr-collections/
    # wms-datasets child catalogs): EDR detail now lives on the item itself.
    resp = client.get("/stac/catalog/adaguc::datasets")
    assert resp.status_code == 200
    catalog = resp.json()

    assert "child" not in {link["rel"] for link in catalog["links"]}

    titles = {link["title"] for link in catalog["links"] if link["rel"] == "item"}
    assert "adaguc.testautotiling" in titles
    # Dataset entries are flat, so the .xml suffix must not leak into the title/id.
    assert all(not title.endswith(".xml") for title in titles)


def test_stac_catalog_datasets_matches_raw_autowms_listing(client: TestClient):
    resp = client.get("/stac/catalog/adaguc::datasets")
    autowms = client.get("/autowms?request=getfiles&path=/adaguc::datasets").json()

    titles = {link["title"] for link in resp.json()["links"] if link["rel"] == "item"}
    autowms_titles = {entry["name"] for entry in autowms["result"]}
    assert titles == autowms_titles


def test_stac_catalog_autowms_listing(client: TestClient):
    resp = client.get("/stac/catalog/adaguc::autowms")
    assert resp.status_code == 200
    catalog = resp.json()
    item_titles = {link["title"] for link in catalog["links"] if link["rel"] == "item"}
    assert "alpha-test.png" in item_titles

    # adaguc::autowms entries are source= files with no dataset identity, so no EDR link here.
    rels = {link["rel"] for link in catalog["links"]}
    assert "data" not in rels


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

    assert rels["alternate"]["href"] == (
        "https://adaguc.knmi.nl/adaguc-viewer/index.html"
        "?autowms=http://testserver/autowms"
        "#addlayer('http://testserver/adagucserver?source=alpha-test.png&','pngdata')"
    )


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
    # This dataset has no EDR collection, so no "edr" link should be advertised.
    assert "edr" not in {link["rel"] for link in item["links"]}


def test_stac_item_edr_links_cover_every_sub_collection(client: TestClient):
    # A dataset's layers can be split into several EDR sub-collections (e.g. by vertical level
    # type, named "{dataset}.{group}" such as "hagl"/"ml"/"pl"); metadata is then keyed by those
    # dotted names only, never by the bare dataset name, so every one must be discovered and
    # linked, each with its own parameters. parameter_names is a pydantic
    # RootModel[Dict[str, Parameter]] wrapper (accessed via .root), not a plain dict - this
    # mocks real objects shaped that way rather than plain dicts, to catch that.
    fake_metadata = {
        "adaguc.testautotiling.hagl": {"layer": {}},
        "adaguc.testautotiling.pl": {"layer": {}},
        "unrelated.dataset": {"layer": {}},
    }

    def fake_collection(_metadata, collection_name, _base_url):
        param = SimpleNamespace(label=f"Label for {collection_name}")
        param_id = f"param_{collection_name.rsplit('.', 1)[-1]}"
        return [SimpleNamespace(id=collection_name, parameter_names=SimpleNamespace(root={param_id: param}))]

    with (
        patch.object(stac_module, "get_metadata", new=AsyncMock(return_value=fake_metadata)),
        patch.object(stac_module, "get_collectioninfo_from_md", side_effect=fake_collection),
    ):
        resp = client.get("/stac/item/adaguc::datasets/adaguc.testautotiling.xml")

    assert resp.status_code == 200
    item = resp.json()
    edr_links = [link for link in item["links"] if link["rel"] == "edr"]
    links_by_href = {link["href"]: link for link in edr_links}

    assert set(links_by_href) == {
        "http://testserver/edr/collections/adaguc.testautotiling.hagl",
        "http://testserver/edr/collections/adaguc.testautotiling.pl",
    }
    assert links_by_href["http://testserver/edr/collections/adaguc.testautotiling.hagl"]["edr:parameters"] == {
        "param_hagl": "Label for adaguc.testautotiling.hagl"
    }


def test_stac_item_links_to_per_layer_collections(client: TestClient):
    resp = client.get("/stac/item/adaguc::datasets/adaguc.tests.graticules.xml")
    assert resp.status_code == 200
    item = resp.json()

    children = {link["title"]: link["href"] for link in item["links"] if link["rel"] == "child"}
    assert children == {
        "grid1": "http://testserver/stac/collections/adaguc.tests.graticules.grid1",
        "grid10": "http://testserver/stac/collections/adaguc.tests.graticules.grid10",
    }


def test_stac_item_wms_link_lists_only_first_layer(client: TestClient):
    # The item's "wms" link must advertise only the first layer, even for a multi-layer
    # dataset: some STAC browsers auto-render a preview by combining every name in
    # wms:layers into one GetMap request, which would stack all layers on top of each other.
    resp = client.get("/stac/item/adaguc::datasets/adaguc.tests.graticules.xml")
    assert resp.status_code == 200
    item = resp.json()

    wms_link = next(link for link in item["links"] if link["rel"] == "wms")
    assert wms_link["wms:layers"] == ["grid1"]


def test_stac_item_has_one_thumbnail_per_layer(client: TestClient):
    # A multi-layer dataset must get an individually addressable preview per layer,
    # not a single thumbnail covering only the first one.
    resp = client.get("/stac/item/adaguc::datasets/adaguc.tests.graticules.xml")
    assert resp.status_code == 200
    item = resp.json()

    thumbnails = {k: a for k, a in item["assets"].items() if a.get("roles") == ["thumbnail"]}
    assert set(thumbnails) == {"thumbnail", "thumbnail_grid10"}
    assert "layers=grid1&" in thumbnails["thumbnail"]["href"]
    assert "layers=grid10&" in thumbnails["thumbnail_grid10"]["href"]


def test_stac_item_has_one_viewer_link_per_layer(client: TestClient):
    resp = client.get("/stac/item/adaguc::datasets/adaguc.tests.graticules.xml")
    assert resp.status_code == 200
    item = resp.json()

    viewer_links = {link["href"] for link in item["links"] if link["rel"] == "alternate"}
    assert viewer_links == {
        "https://adaguc.knmi.nl/adaguc-viewer/index.html?autowms=http://testserver/autowms"
        "#addlayer('http://testserver/adagucserver?dataset=adaguc.tests.graticules&','grid1')",
        "https://adaguc.knmi.nl/adaguc-viewer/index.html?autowms=http://testserver/autowms"
        "#addlayer('http://testserver/adagucserver?dataset=adaguc.tests.graticules&','grid10')",
    }


def test_stac_collection_has_viewer_link(client: TestClient):
    resp = client.get("/stac/collections/adaguc.tests.graticules.grid1")
    assert resp.status_code == 200
    collection = resp.json()

    viewer_link = next(link for link in collection["links"] if link["rel"] == "alternate")
    assert viewer_link["href"] == (
        "https://adaguc.knmi.nl/adaguc-viewer/index.html?autowms=http://testserver/autowms"
        "#addlayer('http://testserver/adagucserver?dataset=adaguc.tests.graticules&','grid1')"
    )


def test_stac_item_source_entry_has_no_layer_collection_children(client: TestClient):
    # source= entries have no dataset identity, so they must never get per-layer collection links.
    resp = client.get("/stac/item/adaguc::autowms/alpha-test.png")
    assert resp.status_code == 200
    item = resp.json()
    assert "child" not in {link["rel"] for link in item["links"]}


def test_stac_collection_for_layer(client: TestClient):
    resp = client.get("/stac/collections/adaguc.tests.graticules.grid1")
    assert resp.status_code == 200
    collection = resp.json()

    assert collection["type"] == "Collection"
    assert collection["id"] == "adaguc.tests.graticules.grid1"
    assert collection["extent"]["spatial"]["bbox"] == [[-180.0, -90.0, 180.0, 90.0]]

    rels = {link["rel"]: link for link in collection["links"]}
    assert rels["parent"]["href"] == "http://testserver/stac/item/adaguc::datasets/adaguc.tests.graticules.xml"
    assert rels["wms"]["wms:layers"] == ["grid1"]
    assert collection["assets"]["thumbnail"]["href"]


def test_stac_collection_unknown_dataset(client: TestClient):
    resp = client.get("/stac/collections/not.a.real.dataset.layer")
    assert resp.status_code == 404


def test_stac_collection_unknown_layer(client: TestClient):
    resp = client.get("/stac/collections/adaguc.tests.graticules.nonexistentlayer")
    assert resp.status_code == 404


def test_stac_item_source_entry_never_has_edr_link(client: TestClient):
    # source= entries (data/autowms) have no dataset identity for EDR to key on,
    # so they must never probe for or advertise an "edr" link.
    resp = client.get("/stac/item/adaguc::autowms/alpha-test.png")
    assert resp.status_code == 200
    item = resp.json()
    assert "edr" not in {link["rel"] for link in item["links"]}


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
