import os
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import make_adaguc_env, update_db

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestWMSLiveUpdate:
    """
    TestWMSLiveUpdate class to the WMS liveupdate behaviour of adaguc-server.
    """

    testresultspath = "testresults/TestWMSLiveUpdate/"
    expectedoutputsspath = "expectedoutputs/TestWMSLiveUpdate/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_WMSLiveUpdateGetMap_SolarTerminatorEquinox(self):
        # Testing the solar terminator on March 21, 2023
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc_tests_solarterminator.xml")
        update_db(env)
        filename = "test_WMSLiveUpdateGetMap_SolarTerminatorEquinox.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=solarterminator&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=solarterminator&WIDTH=200&HEIGHT=200&CRS=EPSG%3A3857&BBOX=-27591378.677139122,-15819675.465716192,24482445.32432534,23920874.430138264&STYLES=solt_twilight&FORMAT=image/png&TRANSPARENT=TRUE&&time=2023-03-21T00%3A00%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSLiveUpdateGetMap_SolarTerminatorSolstice(self):
        # Testing the solar terminator on December 21, 2022
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc_tests_solarterminator.xml")
        update_db(env)
        filename = "test_WMSLiveUpdateGetMap_SolarTerminatorSolstice.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=solarterminator&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=solarterminator&WIDTH=200&HEIGHT=200&CRS=EPSG%3A3857&BBOX=-27591378.677139122,-15819675.465716192,24482445.32432534,23920874.430138264&STYLES=solt_twilight&FORMAT=image/png&TRANSPARENT=TRUE&&time=2022-12-21T00%3A00%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSLiveUpdateGetMap_SolarTerminatorSolstice_continuous(self):
        # Testing the solar terminator on December 21, 2022 with continuous colours
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc_tests_solarterminator.xml")
        update_db(env)
        filename = "test_WMSLiveUpdateGetMap_SolarTerminatorSolstice_continuous.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=solarterminator&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=solarterminator&WIDTH=200&HEIGHT=200&CRS=EPSG%3A3857&BBOX=-27591378.677139122,-15819675.465716192,24482445.32432534,23920874.430138264&STYLES=solt_continuous&FORMAT=image/png&TRANSPARENT=TRUE&&time=2022-12-21T00%3A00%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSLiveUpdateGetMap_SolarTerminatorQuarterPoint(self):
        # Testing the solar terminator on August 7, 2000
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc_tests_solarterminator.xml")
        update_db(env)
        filename = "test_WMSLiveUpdateGetMap_SolarTerminatorQuarterPoint.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=solarterminator&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=solarterminator&WIDTH=200&HEIGHT=200&CRS=EPSG%3A3857&BBOX=-27591378.677139122,-15819675.465716192,24482445.32432534,23920874.430138264&STYLES=solt_twilight&FORMAT=image/png&TRANSPARENT=TRUE&&time=2000-08-07T00%3A00%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSLiveUpdateGetLegendGraphic_SolarTerminator_twilight(self):
        # Testing the solar terminator legend
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc_tests_solarterminator.xml")
        update_db(env)
        filename = "test_WMSLiveUpdateGetLegendGraphic_SolarTerminator_twilight.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=solarterminator&SERVICE=WMS&&version=1.3.0&service=WMS&request=GetLegendGraphic&layer=solarterminator&format=image/png&STYLE=solt_twilight&layers=solarterminator&&&transparent=true&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSLiveUpdateGetLegendGraphic_SolarTerminator_continuous(self):
        # Testing the solar terminator legend
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc_tests_solarterminator.xml")
        update_db(env)
        filename = "test_WMSLiveUpdateGetLegendGraphic_SolarTerminator_continuous.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=solarterminator&SERVICE=WMS&&version=1.3.0&service=WMS&request=GetLegendGraphic&layer=solarterminator&format=image/png&STYLE=solt_continuous&layers=solarterminator&&&transparent=true&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSLiveUpdateGetFeatureInfo_SolarTerminator(self):
        # Testing the solar terminator feature info
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc_tests_solarterminator.xml")
        update_db(env)
        filename = "test_WMSLiveUpdateGetFeatureInfo_SolarTerminator.html"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc_tests_solarterminator&&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=solarterminator&QUERY_LAYERS=solarterminator&CRS=EPSG%3A3857&BBOX=-48040284.36018957,-50974535.88541803,-10040284.360189572,62500602.23612894&WIDTH=362&HEIGHT=1081&I=354&J=481&INFO_FORMAT=text/html&STYLES=&&time=2026-01-05T16%3A40%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSLiveUpdateGetFeatureInfo_timeseries_SolarTerminator(self):
        # Testing the solar terminator feature info
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc_tests_solarterminator.xml")
        update_db(env)
        filename = "test_WMSLiveUpdateGetFeatureInfo_timeseries_SolarTerminator.html"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=solt&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=solarterminator&query_layers=solarterminator&crs=EPSG%3A3857&bbox=-19000000%2C-78286307.05394192%2C19000000%2C78286307.05394192&width=241&height=993&i=180&j=366&format=image%2Fgif&info_format=application%2Fjson&time=2025-10-01T00%3A00%3A00Z%2F2025-10-02T00%3A00%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSLiveUpdateGetCapabilities_SolarTerminator(self):
        # Testing the solar terminator capabilities
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.solarterminator-custom.xml")
        update_db(env)
        filename = "test_WMSLiveUpdateGetCapabilities_SolarTerminator.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=solt&SERVICE=WMS&request=GetCapabilities",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0

        assert AdagucTestTools().compareTimeRange(self.testresultspath + filename, 7, "PT1H", "solarterminator")
