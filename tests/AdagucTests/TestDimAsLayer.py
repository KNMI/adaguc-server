"""Tests for Dimension aslayers="true": one layer per dimension value, e.g. one layer per model run."""

import os
import shutil
import subprocess
import time

import pytest
from lxml import etree
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import make_adaguc_env, update_db

ADAGUC_PATH = os.environ["ADAGUC_PATH"]
DATA_DIR = f"{ADAGUC_PATH}/data/datasets/dimaslayers"
WMS_NS = {"wms": "http://www.opengis.net/wms"}

# Generated layer name -> (time value of the run, elevations of the run)
RUNS = {
    "trajectory_2026-11-12T080000Z": ("2026-11-12T08:00:00Z", ["50"]),
    "trajectory_2026-11-12T100000Z": ("2026-11-12T10:00:00Z", ["60"]),
    "trajectory_2026-11-12T103000Z": ("2026-11-12T10:30:00Z", ["20", "40", "70"]),
}

# All configurations show the same three runs: additional layers with aslayers, additional layers without aslayers and a visible additional layer
DATASETS = [
    "adaguc.tests.dimaslayers",
    "adaguc.tests.dimaslayers_additional_without_aslayers",
    "adaguc.tests.dimaslayers_visible_additional",
]

GETMAP = "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&WIDTH=320&HEIGHT=240&CRS=EPSG%3A4326&BBOX=49,4,56,12&STYLES=&FORMAT=image/png&TRANSPARENT=FALSE"


def get_layers(env: dict) -> dict:
    """Returns the named layers from GetCapabilities as {name: {"title": str, "dims": {name: (values, default)}}}"""
    status, data, _ = AdagucTestTools().runADAGUCServer("SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities", env=env)
    assert status == 0
    root = etree.fromstring(data)
    layers = {}
    for layer in root.iterfind(".//wms:Layer", WMS_NS):
        name = layer.find("wms:Name", WMS_NS)
        if name is None:
            continue
        dims = {dim.get("name"): (dim.text.split(","), dim.get("default")) for dim in layer.iterfind("wms:Dimension", WMS_NS)}
        layers[name.text] = {"title": layer.find("wms:Title", WMS_NS).text, "dims": dims}
    return layers


def get_trajectory_layer_names(layers: dict) -> set:
    return {name for name in layers if name.startswith("trajectory")}


def copy_runs_to_tmp(*runs: str) -> str:
    """Copies the files of the given runs (file name prefixes like 20261112T0800) to the temporary data directory"""
    tmp_data_dir = os.path.join(os.environ["ADAGUC_TMP"], "dimaslayers")
    os.makedirs(tmp_data_dir, exist_ok=True)
    for file_name in os.listdir(DATA_DIR):
        if any(file_name.startswith(f"trajectory_{run}_") for run in runs):
            shutil.copy(os.path.join(DATA_DIR, file_name), tmp_data_dir)
    return tmp_data_dir


class TestDimAsLayer:
    testresultspath = "testresults/TestDimAsLayer/"
    expectedoutputsspath = "expectedoutputs/TestDimAsLayer/"

    AdagucTestTools().mkdir_p(testresultspath)

    def test_DimAsLayer_GetCapabilities_has_one_layer_per_run(self):
        env = make_adaguc_env(f"{ADAGUC_PATH}/data/config/datasets/adaguc.tests.dimaslayers.xml")
        update_db(env)
        layers = get_layers(env)

        # The configured layer and the hidden additional layers are not advertised, only one layer per run
        assert get_trajectory_layer_names(layers) == set(RUNS)
        for name, (run_time, elevations) in RUNS.items():
            layer = layers[name]
            assert layer["title"] == f"Trajectory {run_time}"
            # The time dimension is fixed and hidden, the elevation dimension only lists the elevations of this run
            assert layer["dims"] == {"elevation": (elevations, elevations[-1])}

    def test_DimAsLayer_GetCapabilities_additional_layers_without_aslayers(self):
        env = make_adaguc_env(f"{ADAGUC_PATH}/data/config/datasets/adaguc.tests.dimaslayers_additional_without_aslayers.xml")
        update_db(env)
        layers = get_layers(env)

        # The hidden copies of the additional layers per run are not advertised
        assert get_trajectory_layer_names(layers) == set(RUNS)

    def test_DimAsLayer_GetCapabilities_visible_additional_layer(self):
        # The main layer is configured before the additional layer: the visible additional layer must still be advertised per run.
        env = make_adaguc_env(f"{ADAGUC_PATH}/data/config/datasets/adaguc.tests.dimaslayers_visible_additional.xml")
        update_db(env)
        layers = get_layers(env)

        point_layer_names = {name.replace("trajectory_", "trajectory_pt_", 1) for name in RUNS}
        assert get_trajectory_layer_names(layers) == set(RUNS) | point_layer_names
        for name, (run_time, elevations) in RUNS.items():
            point_layer = layers[name.replace("trajectory_", "trajectory_pt_", 1)]
            assert point_layer["title"] == f"Trajectory points {run_time}"
            assert point_layer["dims"] == {"elevation": (elevations, elevations[-1])}

    @pytest.mark.parametrize("dataset", DATASETS)
    @pytest.mark.parametrize("layer_name", list(RUNS))
    def test_DimAsLayer_GetMap_shows_run_of_layer(self, dataset: str, layer_name: str):
        # The additional point and line layers must show the same run as the layer, for every configuration
        env = make_adaguc_env(f"{ADAGUC_PATH}/data/config/datasets/{dataset}.xml")
        update_db(env)

        filename = f"{layer_name}.png"
        result_path = f"{self.testresultspath}{dataset}/"
        AdagucTestTools().mkdir_p(result_path)
        status, data, _ = AdagucTestTools().runADAGUCServer(f"{GETMAP}&LAYERS={layer_name}", env=env)
        AdagucTestTools().writetofile(result_path + filename, data)
        assert status == 0
        assert AdagucTestTools().compareImage(self.expectedoutputsspath + filename, result_path + filename)

    @pytest.mark.parametrize("elevation", ["20", "40", "70"])
    def test_DimAsLayer_GetMap_with_elevation(self, elevation: str):
        env = make_adaguc_env(f"{ADAGUC_PATH}/data/config/datasets/adaguc.tests.dimaslayers.xml")
        update_db(env)

        filename = f"trajectory_2026-11-12T103000Z_elevation_{elevation}.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(f"{GETMAP}&LAYERS=trajectory_2026-11-12T103000Z&ELEVATION={elevation}", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareImage(self.expectedoutputsspath + filename, self.testresultspath + filename)

    def test_DimAsLayer_GetMap_configured_layer_is_not_available(self):
        env = make_adaguc_env(f"{ADAGUC_PATH}/data/config/datasets/adaguc.tests.dimaslayers.xml")
        update_db(env)

        status, _, _ = AdagucTestTools().runADAGUCServer(f"{GETMAP}&LAYERS=trajectory", env=env)
        assert status != 0

    def test_DimAsLayer_layer_metadata_is_stored_for_each_run(self):
        env = make_adaguc_env(f"{ADAGUC_PATH}/data/config/datasets/adaguc.tests.dimaslayers.xml")
        update_db(env, force_update=True)

        query = "select distinct layername from layermetadata where datasetname = 'adaguc.tests.dimaslayers'"
        result = subprocess.run(["psql", os.environ["ADAGUC_DB"], "-Atc", query], capture_output=True, text=True, check=True)
        layer_names = set(result.stdout.split())
        assert set(RUNS) <= layer_names

    def test_DimAsLayer_layers_follow_scanned_runs(self):
        env = make_adaguc_env(f"{ADAGUC_PATH}/data/config/datasets/adaguc.tests.dimaslayers_retention.xml")
        tmp_data_dir = copy_runs_to_tmp("20261112T0800")
        update_db(env, force_update=True)
        assert get_trajectory_layer_names(get_layers(env)) == {"trajectory_2026-11-12T080000Z"}

        # A new run becomes a new layer after scanning
        copy_runs_to_tmp("20261112T1000")
        update_db(env, force_update=True)
        assert get_trajectory_layer_names(get_layers(env)) == {"trajectory_2026-11-12T080000Z", "trajectory_2026-11-12T100000Z"}

        # A removed run disappears after scanning
        os.remove(os.path.join(tmp_data_dir, "trajectory_20261112T0800_50.geojson"))
        update_db(env, force_update=True)
        assert get_trajectory_layer_names(get_layers(env)) == {"trajectory_2026-11-12T100000Z"}

    def test_DimAsLayer_retention_removes_old_runs(self):
        env = make_adaguc_env(f"{ADAGUC_PATH}/data/config/datasets/adaguc.tests.dimaslayers_retention.xml")
        tmp_data_dir = copy_runs_to_tmp("20261112T0800", "20261112T1000", "20261112T1030")

        # The files of the first run are older than the retention period of one day
        three_days_ago = time.time() - 3 * 24 * 3600
        old_file = os.path.join(tmp_data_dir, "trajectory_20261112T0800_50.geojson")
        os.utime(old_file, (three_days_ago, three_days_ago))

        update_db(env, force_update=True)

        assert not os.path.exists(old_file)
        assert len(os.listdir(tmp_data_dir)) == 4
        assert get_trajectory_layer_names(get_layers(env)) == {"trajectory_2026-11-12T100000Z", "trajectory_2026-11-12T103000Z"}
