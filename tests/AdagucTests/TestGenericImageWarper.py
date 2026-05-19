import os
import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import (
    make_adaguc_env,
    run_adaguc_and_compare_image,
    update_db,
)

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestGenericImageWarper:
    testresultspath = "testresults/TestGenericImageWarper/"
    expectedoutputsspath = "expectedoutputs/TestGenericImageWarper/"
    env = {}
    AdagucTestTools().mkdir_p(testresultspath)

    @pytest.mark.parametrize(
        ("filetocheck", "layers", "styles"),
        [
            (
                "ahn_utrechtseheuvelrug_500m_style_colormapped_nearest_elevation.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_colormapped_nearest_elevation",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_elevation.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_colormapped_bilinear_elevation",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_shaded_nearest_elevation.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_shaded_nearest_elevation",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_shaded_bilinear_elevation.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_shaded_bilinear_elevation",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_multicolor.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_colormapped_bilinear_multicolor",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_multicolor_contours.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_colormapped_bilinear_multicolor_contours",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_multicolor_contours_extrasmooth.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_colormapped_bilinear_multicolor_contours_extrasmooth",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_multicolor_contours_extrasmooth_discretized.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_colormapped_bilinear_multicolor_contours_extrasmooth_discretized",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_hillshaded_opaque.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_hillshaded_opaque/hillshaded",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_hillshaded_transparent.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_hillshaded_transparent/hillshaded",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_hillshaded_style_colormapped_bilinear_elevation.png",
                "ahn_utrechtseheuvelrug_500m_hillshaded",
                "style_colormapped_bilinear_elevation",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_hillshaded_style_colormapped_bilinear_multicolor_contours.png",
                "ahn_utrechtseheuvelrug_500m_hillshaded",
                "style_colormapped_bilinear_multicolor_contours",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_hillshaded_style_dutch_mountains.png",
                "ahn_utrechtseheuvelrug_500m_hillshaded",
                "style_dutch_mountains",
            ),
        ],
    )
    def test_GenericImageWarperOnAHNDataset(self, filetocheck: str, layers: str, styles: str):
        AdagucTestTools().cleanTempDir()

        config = ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.ahn_utrechtse_heuvelrug_500m.xml"
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        assert status == 0

        status, data, _ = AdagucTestTools().runADAGUCServer(
            f"LAYERS={layers}&STYLES={styles}&DATASET=adaguc.tests.ahn_utrechtse_heuvelrug_500m&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&WIDTH=512&HEIGHT=512&CRS=EPSG%3A3857&BBOX=590359.2826213799,6784102.0088095935,707368.84890462,6894816.225400406&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF&SHOWLEGEND=true",
            env=env,
            showLog=False,
        )

        AdagucTestTools().writetofile(self.testresultspath + filetocheck, data)

        assert status == 0
        assert AdagucTestTools().compareImage(
            self.expectedoutputsspath + filetocheck,
            self.testresultspath + filetocheck,
            maxAllowedColorDifference=1,
            maxAllowedColorPercentage=0.02,
        )

    def test_getmap_latlon_latdecreasing_and_increasing_genericbilinear(self):
        """
        Test rendering of data aligned upwards and downwards with bilinear interpolation
        """
        env = make_adaguc_env("adaguc.tests.genericrender.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_getmap_latlon_latdecreasing_and_increasing_genericbilinear.png",
            "DATASET=adaguc.tests.genericrender&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=latlon_latdecreasing_and_increasing_genericbilinear&WIDTH=256&HEIGHT=512&CRS=EPSG%3A3857&BBOX=471070.27730388555,6649427.860630986,982084.0646480684,7697681.648355679&STYLES=autobilinear&FORMAT=image/png&TRANSPARENT=TRUE&&time=2017-10-15T07%3A31%3A53Z&0.9458434770354436",
        )

    def test_getmap_latlon_latdecreasing_and_increasing_genericnearest(self):
        """
        Test rendering of data aligned upwards and downwards with nearest interpolation
        """
        env = make_adaguc_env("adaguc.tests.genericrender.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_getmap_latlon_latdecreasing_and_increasing_genericnearest.png",
            "DATASET=adaguc.tests.genericrender&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=latlon_latdecreasing_and_increasing_genericnearest&WIDTH=256&HEIGHT=512&CRS=EPSG%3A3857&BBOX=471070.27730388555,6649427.860630986,982084.0646480684,7697681.648355679&STYLES=autogeneric&FORMAT=image/png&TRANSPARENT=TRUE&&time=2017-10-15T07%3A31%3A53Z&0.9458434770354436",
        )
