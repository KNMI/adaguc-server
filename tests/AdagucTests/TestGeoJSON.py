import os
import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import make_adaguc_env, update_db

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestGeoJSON:
    testresultspath = "testresults/TestGeoJSON/"
    expectedoutputsspath = "expectedoutputs/TestGeoJSON/"

    AdagucTestTools().mkdir_p(testresultspath)

    def test_GeoJSON_countries_autowms(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_GeoJSON_countries_autowms.png"
        env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=countries.geojson&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=features&WIDTH=256&HEIGHT=128&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=default&FORMAT=image/png&TRANSPARENT=TRUE",
            env=env,
            showLog=False,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())

        assert status == 0
        assert data.getvalue() == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_GeoJSON_time_GetCapabilities(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testGeoJSONReader_time.xml")
        update_db(env)

        # Test GetCapabilities
        filename = "test_GeoJSON_time_GetCapabilities.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer("&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())

        assert status == 0
        assert AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename)

    @pytest.mark.parametrize(
        "date",
        [
            ("2018-12-04T12:00:00Z"),
            ("2018-12-04T12:05:00Z"),
            ("2018-12-04T12:10:00Z"),
        ],
    )
    def test_GeoJSON_3timesteps(self, date: str):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testGeoJSONReader_time.xml")
        update_db(env)

        filename = f"test_GeoJSON_timesupport{date}.png".replace(":", "_")

        status, data, _ = AdagucTestTools().runADAGUCServer(
            "&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=features&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=40,-10,60,40&STYLES=features&FORMAT=image/png&TRANSPARENT=TRUE&TIME="
            + date,
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())

        assert status == 0
        assert data.getvalue() == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)
