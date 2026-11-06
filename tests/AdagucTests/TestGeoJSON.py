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
        AdagucTestTools().writetofile(self.testresultspath + filename, data)

        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_GeoJSON_time_GetCapabilities(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testGeoJSONReader_time.xml")
        update_db(env)

        # Test GetCapabilities
        filename = "test_GeoJSON_time_GetCapabilities.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer("&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data)

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
        AdagucTestTools().writetofile(self.testresultspath + filename, data)

        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_GeoJSON_reference_time_GetCapabilities(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testGeoJSONReader_reference_time.xml")
        update_db(env)

        filename = "test_GeoJSON_reference_time_GetCapabilities.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer("&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data)

        assert status == 0
        assert AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename)

    @pytest.mark.parametrize(
        ("time", "reference_time", "expected_box"),
        [
            # The reference_time files reuse the geometries of the geojson_time files, so the timesupport images are the expected outputs.
            ("2018-12-04T12:00:00Z", "2018-12-04T06:00:00Z", "2018-12-04T12_00_00Z"),
            ("2018-12-04T12:05:00Z", "2018-12-04T06:00:00Z", "2018-12-04T12_05_00Z"),
            ("2018-12-04T12:00:00Z", "2018-12-04T09:00:00Z", "2018-12-04T12_10_00Z"),
        ],
    )
    def test_GeoJSON_reference_time_GetMap(self, time: str, reference_time: str, expected_box: str):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testGeoJSONReader_reference_time.xml")
        update_db(env)

        filename = f"test_GeoJSON_reference_time{time}_{reference_time}.png".replace(":", "_")

        status, data, _ = AdagucTestTools().runADAGUCServer(
            "&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=features&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=40,-10,60,40&STYLES=features&FORMAT=image/png&TRANSPARENT=TRUE&TIME="
            + time
            + "&DIM_reference_time="
            + reference_time,
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)

        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + f"test_GeoJSON_timesupport{expected_box}.png")

    def test_GeoJSON_point_dimensions_GetCapabilities(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testGeoJSONReader_point_dimensions.xml")
        update_db(env)

        filename = "test_GeoJSON_point_dimensions_GetCapabilities.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer("&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data)

        assert status == 0
        assert AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename)

    @pytest.mark.parametrize(
        ("time", "reference_time", "elevation"),
        [
            ("2018-12-04T12:00:00Z", "2018-12-04T06:00:00Z", "500"),
            ("2018-12-04T12:00:00Z", "2018-12-04T06:00:00Z", "1000"),
            ("2018-12-04T12:05:00Z", "2018-12-04T09:00:00Z", "500"),
        ],
    )
    def test_GeoJSON_point_dimensions_GetMap(self, time: str, reference_time: str, elevation: str):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testGeoJSONReader_point_dimensions.xml")
        update_db(env)

        filename = f"test_GeoJSON_point_dimensions{time}_{reference_time}_{elevation}.png".replace(":", "_")
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=points&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=473630.94842255395,6776005.182974723,597052.5927748629,6902201.422432627&STYLES=stations_mistpost%2Fpoint&FORMAT=image/png&TRANSPARENT=FALSE&TIME="
            + time
            + "&DIM_reference_time="
            + reference_time
            + "&ELEVATION="
            + elevation,
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)

        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_GetMAP_GeoJSON_point_rendering(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.geojsonpoint.xml")
        update_db(env)

        # Test GetCapabilities
        filename = "test_GetMAP_GeoJSON_point_rendering.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.geojsonpoint&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=mistposten&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=473630.94842255395,6776005.182974723,597052.5927748629,6902201.422432627&STYLES=stations_mistpost%2Fpoint&FORMAT=image/png&TRANSPARENT=FALSE&&0.9390440735704288",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)

        assert status == 0
        assert AdagucTestTools().compareImage(self.testresultspath + filename, self.expectedoutputsspath + filename)

    def test_CVE_GHSA_mwgv_59vv_rp2m(self):
        """
        Fixes CVE_GHSA_mwgv_59vv_rp2m as found by Sebastian Alba Vives
        https://github.com/KNMI/adaguc-server/security/advisories/GHSA-mwgv-59vv-rp2m
        """
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.CVE_GHSA_mwgv_59vv_rp2m.xml")
        update_db(env)
