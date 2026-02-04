import os
import pytest
from adaguc.AdagucTestTools import AdagucTestTools

from conftest import make_adaguc_env, update_db

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestCSV:
    testresultspath = "testresults/TestCSV/"
    expectedoutputsspath = "expectedoutputs/TestCSV/"

    AdagucTestTools().mkdir_p(testresultspath)

    @pytest.mark.parametrize(
        ("date"),
        [
            ("2018-12-04T12:00:00Z"),
            ("2018-12-04T12:05:00Z"),
            ("2018-12-04T12:10:00Z"),
            ("2018-12-04T12:15:00Z"),
            ("2018-12-04T12:20:00Z"),
            ("2018-12-04T12:25:00Z"),
            ("2018-12-04T12:30:00Z"),
            ("2018-12-04T12:35:00Z"),
            ("2018-12-04T12:40:00Z"),
            ("2018-12-04T12:45:00Z"),
            ("2018-12-04T12:50:00Z"),
            ("2018-12-04T12:55:00Z"),
        ],
    )
    def test_CSV_12timesteps(self, date: str):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testCSVReader.xml")
        update_db(env)

        filename = ("test_CSV_timesupport" + date + ".png").replace(":", "_")
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=-180,-90,180,90&STYLES=windbarb&FORMAT=image/png&TRANSPARENT=TRUE&TIME="
            + date,
            env=env,
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        assert status == 0
        assert data.getvalue() == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_CSV_windbarbs_Cairo_png(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testCSVReader.xml")
        update_db(env)

        filename = "test_CSV_windbarbs.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=windallspeeds&width=1600&height=500&CRS=EPSG:4326&STYLES=&EXCEPTIONS=INIMAGE&showlegend=false&0.817264530295692&bbox=-11,-1,11,32&transparent=true&FORMAT=image/png&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        assert status == 0
        assert data.getvalue() == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_CSV_negative_values(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testCSVReader.xml")
        update_db(env)

        filename = "test_CSV_negative_values.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=tn&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=4000904.446200706,-1231688.3419664246,4468921.645102525,-685668.2765809689&STYLES=name/point&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        assert status == 0
        assert data.getvalue() == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_CSV_reference_time(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testCSVReader_reference_time.xml")
        update_db(env)

    def test_CSV_reference_time_GetCapabilities(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testCSVReader_reference_time.xml")
        update_db(env)

        # Test GetCapabilities
        filename = "test_CSV_reference_timesupport_GetCapabilities.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer("&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())

        assert status == 0
        assert AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename)

    @pytest.mark.parametrize(
        ("time", "reference_time"),
        [
            ("2018-12-04T12:00:00Z", "2018-12-04T12:00:00Z"),
            ("2018-12-04T12:05:00Z", "2018-12-04T12:00:00Z"),
            ("2018-12-04T12:05:00Z", "2018-12-04T12:05:00Z"),
            ("2018-12-04T12:10:00Z", "2018-12-04T12:05:00Z"),
        ],
    )
    def test_CSV_reference_time_GetMap(self, time: str, reference_time: str):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testCSVReader_reference_time.xml")
        update_db(env)

        # Test WMS GetMap
        filename = ("test_CSV_reference_timesupport" + time + "_" + reference_time + ".png").replace(":", "_")
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=-180,-90,180,90&STYLES=windbarb&FORMAT=image/png&TRANSPARENT=TRUE&TIME="
            + time
            + "&DIM_reference_time="
            + reference_time,
            env=env,
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        assert status == 0
        assert data.getvalue() == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_CSV_radiusandvalue(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testCSV_radiusandvalue.xml")
        update_db(env)

        filename = "test_CSV_radiusandvalue.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=radiusandvalue&WIDTH=256&HEIGHT=512&CRS=EPSG%3A3857&BBOX=-8003558.6330057755,1638420.481402514,-7346556.700484946,2491778.5155690867&STYLES=magnitude&FORMAT=image/png&TRANSPARENT=TRUE&showlegend=true",
            env=env,
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        assert status == 0
        assert data.getvalue() == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_CSV_radiusandvalue_and_symbol(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.testCSV_radiusandvalue.xml")
        update_db(env)

        filename = "test_CSV_radiusandvalue_and_symbol.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=radiusandvalue_and_symbol&WIDTH=256&HEIGHT=512&CRS=EPSG%3A3857&BBOX=-8003558.6330057755,1638420.481402514,-7346556.700484946,2491778.5155690867&STYLES=magnitude&FORMAT=image/png&TRANSPARENT=TRUE&showlegend=true",
            env=env,
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        assert status == 0
        assert data.getvalue() == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)
