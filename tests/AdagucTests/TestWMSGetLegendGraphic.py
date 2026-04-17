import os
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import make_adaguc_env, update_db

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestWMSGetLegendGraphic:
    """
    TestWMSGetLegendGraphic class to thest WMS GetLegendGraphic behaviour of adaguc-server.
    """

    testresultspath = "testresults/TestWMSGetLegendGraphic/"
    expectedoutputsspath = "expectedoutputs/TestWMSGetLegendGraphic/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_WMSGetLegendGraphic_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetLegendGraphic_testdatanc.png"

        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&VERSION=1.3.0&request=GetLegendGraphic&layer=testdata&format=image/png&STYLE=auto/nearest&layers=testdata&transparent=true",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetLegendGraphic_adaguc_scaling_dataset_scaling1x(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.scaling.xml")
        update_db(env)

        filename = "test_WMSGetLegendGraphic_adaguc_scaling_dataset_scaling1x.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.scaling&SERVICE=WMS&&version=1.1.1&service=WMS&request=GetLegendGraphic&layer=testdata&format=image/png&STYLE=testdata/nearest&layers=testdata&&&transparent=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetMapWithGetLegendGraphic_adaguc_scaling_dataset_scaling1x(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.scaling.xml")
        update_db(env)

        filename = "test_WMSGetMapWithGetLegendGraphic_adaguc_scaling_dataset_scaling1x.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.scaling&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=1024&HEIGHT=1024&CRS=EPSG%3A3857&BBOX=-3099408.36963744,3701316.1668297593,3704230.74564144,10633468.46539724&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&showlegend=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetMapWithGetLegendGraphic_adaguc_scaling_dataset_scaling4x(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.scaling.xml")
        update_db(env)

        filename = "test_WMSGetMapWithGetLegendGraphic_adaguc_scaling_dataset_scaling4x.png"

        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.scaling&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=1024&HEIGHT=1024&CRS=EPSG%3A3857&BBOX=-3099408.36963744,3701316.1668297593,3704230.74564144,10633468.46539724&STYLES=testdatascaling4x%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&showlegend=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetLegendGraphic_inverted_min_max(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.invertedlegend.xml")
        update_db(env)

        filename = "test_WMSGetLegendGraphic_inverted_min_max.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.invertedlegend&SERVICE=WMS&&version=1.1.1&service=WMS&request=GetLegendGraphic&layer=testdata&format=image/png&STYLE=testdatainverted/nearest&layers=testdata&&&transparent=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetLegendGraphic_log10(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.invertedlegend.xml")
        update_db(env)

        filename = "test_WMSGetLegendGraphic_log10.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.invertedlegend&SERVICE=WMS&&version=1.1.1&service=WMS&request=GetLegendGraphic&layer=testdata&format=image/png&STYLE=testdatalog10/nearest&layers=testdata&&&transparent=false",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetLegendGraphic_NearestRenderWithShadeInterval(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.nearestshadeinterval.xml")
        update_db(env)

        filename = "test_WMSGetLegendGraphic_NearestRenderWithShadeInterval.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=nearestshadeinterval&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetLegendGraphic&LAYER=testdata&WIDTH=150&HEIGHT=800&STYLES=shadedstyleprecise%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetLegendGraphic_LongTempLegend(self):
        """Skip some labels when legend is too long"""
        """Corresponds to the special case for definedLegendOnShadeClasses case in CCreateLegendRenderDiscreteLegend """
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/test.uwcw_ha43_dini_5p5km_10x8.xml")
        update_db(env)

        filename = "test_WMSGetLegendGraphic_LongTempLegend.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=test.uwcw_ha43_dini_5p5km_10x8&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetLegendGraphic&LAYER=air_temperature_pl&WIDTH=100&HEIGHT=400&STYLE=temperature%2Fshaded&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetLegendGraphic_LongTempLegend_height260(self):
        """Skip some labels when legend is too long, making sure the whole range is contained"""
        """Corresponds to the special case for definedLegendOnShadeClasses case in CCreateLegendRenderDiscreteLegend """
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/test.uwcw_ha43_dini_5p5km_10x8.xml")
        update_db(env)

        filename = "test_WMSGetLegendGraphic_LongTempLegend_height260.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=test.uwcw_ha43_dini_5p5km_10x8&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetLegendGraphic&LAYER=air_temperature_pl&WIDTH=260&HEIGHT=260&STYLE=temperature%2Fshaded&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetLegendGraphic_LongTempLegend_clipping(self):
        """Skip some labels when legend is too long and clip"""
        """Corresponds to the special case for definedLegendOnShadeClasses case in CCreateLegendRenderDiscreteLegend """
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/test.uwcw_ha43_dini_5p5km_10x8.xml")
        update_db(env)

        filename = "test_WMSGetLegendGraphic_LongTempLegend_clipping.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=test.uwcw_ha43_dini_5p5km_10x8&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetLegendGraphic&LAYER=air_temperature_pl&WIDTH=100&HEIGHT=400&STYLE=temperature_clip%2Fshaded&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetLegendGraphic_LongTempLegend2(self):
        """Another test showing another type of legend that skips classes without data"""
        """Corresponds to the discreteLegendOnInterval case in CCreateLegendRenderDiscreteLegend """
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/test.uwcw_ha43_dini_5p5km_10x8.xml")
        update_db(env)

        filename = "test_WMSGetLegendGraphic_LongTempLegend2.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=test.uwcw_ha43_dini_5p5km_10x8&SERVICE=WMS&&version=1.3.0&service=WMS&request=GetLegendGraphic&layer=air_temperature_pl&format=image/png&STYLE=temperature_wow/shaded&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)
