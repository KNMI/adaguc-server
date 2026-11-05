import os
import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import make_adaguc_env, update_db

ADAGUC_PATH = os.environ["ADAGUC_PATH"]

LAYERS = ["ta_reftime_dimension", "ta_reftime_scalar"]


class TestPointNetCDFReferenceTime:
    """
    Point NetCDF files with a reference time, either as a dimension of the point variables or as a scalar
    forecast_reference_time variable. The files are created with python/format_standard/create_point_reference_time.py.
    """

    testresultspath = "testresults/TestPointNetCDFReferenceTime/"
    expectedoutputsspath = "expectedoutputs/TestPointNetCDFReferenceTime/"

    AdagucTestTools().mkdir_p(testresultspath)

    def test_PointNetCDF_reference_time_GetCapabilities(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.netcdfpoint_reference_time.xml")
        update_db(env)

        filename = "test_PointNetCDF_reference_time_GetCapabilities.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer("&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data)

        assert status == 0
        assert AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename)

    @pytest.mark.parametrize("layer", LAYERS)
    @pytest.mark.parametrize(
        ("time", "reference_time"),
        [
            ("2020-12-20T12:00:00Z", "2020-12-20T06:00:00Z"),
            ("2020-12-20T12:10:00Z", "2020-12-20T06:00:00Z"),
            ("2020-12-20T12:00:00Z", "2020-12-20T09:00:00Z"),
        ],
    )
    def test_PointNetCDF_reference_time_GetMap(self, layer: str, time: str, reference_time: str):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.netcdfpoint_reference_time.xml")
        update_db(env)

        # Both layouts contain the same values, so they share the expected image
        filename = f"test_PointNetCDF_reference_time{time}_{reference_time}.png".replace(":", "_")
        status, data, _ = AdagucTestTools().runADAGUCServer(
            f"&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS={layer}&WIDTH=300&HEIGHT=300&CRS=EPSG%3A4326&BBOX=51.4,3.8,53.1,6.2&STYLES=values%2Fpoint&FORMAT=image/png&TRANSPARENT=FALSE&TIME="
            + time
            + "&DIM_reference_time="
            + reference_time,
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + layer + "_" + filename, data)

        assert status == 0
        assert AdagucTestTools().compareImage(self.expectedoutputsspath + filename, self.testresultspath + layer + "_" + filename)
