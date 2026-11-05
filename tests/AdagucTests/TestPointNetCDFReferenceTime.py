import re
import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import make_adaguc_env, update_db

DATASET = "{ADAGUC_PATH}/data/config/datasets/adaguc.tests.netcdfpoint_reference_time.xml"
LAYERS = ["ta_reftime_dimension", "ta_reftime_scalar", "ta_reftime_scalar_frt"]


class TestPointNetCDFReferenceTime:
    """
    Point NetCDF files with a reference time, either as a dimension of the point variables or as a scalar
    forecast_reference_time variable, also with a name without "time" (frt).
    The files are created with python/format_standard/create_point_reference_time.py.
    """

    testresultspath = "testresults/TestPointNetCDFReferenceTime/"
    expectedoutputsspath = "expectedoutputs/TestPointNetCDFReferenceTime/"

    AdagucTestTools().mkdir_p(testresultspath)

    def test_PointNetCDF_reference_time_GetCapabilities(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env(DATASET)
        update_db(env)

        filename = "test_PointNetCDF_reference_time_GetCapabilities.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer("SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities", env=env)
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

        env = make_adaguc_env(DATASET)
        update_db(env)

        # All layouts contain the same values, so they share the expected image
        filename = f"test_PointNetCDF_reference_time{time}_{reference_time}.png".replace(":", "_")
        status, data, _ = AdagucTestTools().runADAGUCServer(
            f"SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS={layer}&WIDTH=300&HEIGHT=300&CRS=EPSG%3A4326&BBOX=51.4,3.8,53.1,6.2&STYLES=values%2Fpoint&FORMAT=image/png&TRANSPARENT=FALSE&TIME="
            + time
            + "&DIM_reference_time="
            + reference_time,
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + layer + "_" + filename, data)

        assert status == 0
        assert AdagucTestTools().compareImage(self.expectedoutputsspath + filename, self.testresultspath + layer + "_" + filename)

    @pytest.mark.parametrize("layer", LAYERS)
    def test_PointNetCDF_reference_time_WCSDescribeCoverage(self, layer: str):
        """The time axis of the coverage is the time dimension, not reference_time"""
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env(DATASET)
        update_db(env)

        status, data, _ = AdagucTestTools().runADAGUCServer(f"SERVICE=WCS&REQUEST=DescribeCoverage&COVERAGE={layer}", env=env)

        assert status == 0
        temporal_domain = re.search(r"<temporalDomain>(.*)</temporalDomain>", data.decode(), re.DOTALL)
        assert temporal_domain is not None
        assert "2020-12-20T12:00:00Z" in temporal_domain.group(1)
        assert "06:00:00Z" not in temporal_domain.group(1)
        assert "09:00:00Z" not in temporal_domain.group(1)
