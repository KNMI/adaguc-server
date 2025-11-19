import json
import os
from io import BytesIO
from adaguc.CGIRunner import CGIRunner
import unittest
import shutil
import sys
import subprocess
from lxml import etree
from lxml import objectify
import re
from adaguc.AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestGenericImageWarper(unittest.TestCase):
    testresultspath = "testresults/TestGenericImageWarper/"
    expectedoutputsspath = "expectedoutputs/TestGenericImageWarper/"
    env={}
    AdagucTestTools().mkdir_p(testresultspath)

    def test_GenericImageWarperOnAHNDataset(self):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.ahn_utrechtse_heuvelrug_500m.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        test_cases = [
            {
                "filetocheck": "ahn_utrechtseheuvelrug_500m_style_colormapped_nearest_elevation.png",
                "layers": "ahn_utrechtseheuvelrug_500m",
                "styles": "style_colormapped_nearest_elevation",
            },
            {
                "filetocheck": "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_elevation.png",
                "layers": "ahn_utrechtseheuvelrug_500m",
                "styles": "style_colormapped_bilinear_elevation",
            },
            {
                "filetocheck": "ahn_utrechtseheuvelrug_500m_style_shaded_nearest_elevation.png",
                "layers": "ahn_utrechtseheuvelrug_500m",
                "styles": "style_shaded_nearest_elevation",
            },
            {
                "filetocheck": "ahn_utrechtseheuvelrug_500m_style_shaded_bilinear_elevation.png",
                "layers": "ahn_utrechtseheuvelrug_500m",
                "styles": "style_shaded_bilinear_elevation",
            },
            {
                "filetocheck": "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_multicolor.png",
                "layers": "ahn_utrechtseheuvelrug_500m",
                "styles": "style_colormapped_bilinear_multicolor",
            },
            {
                "filetocheck": "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_multicolor_contours.png",
                "layers": "ahn_utrechtseheuvelrug_500m",
                "styles": "style_colormapped_bilinear_multicolor_contours",
            }, 
            {
                "filetocheck": "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_multicolor_contours_extrasmooth.png",
                "layers": "ahn_utrechtseheuvelrug_500m",
                "styles": "style_colormapped_bilinear_multicolor_contours_extrasmooth",
            },
            {
                "filetocheck": "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_multicolor_contours_extrasmooth_discretized.png",
                "layers": "ahn_utrechtseheuvelrug_500m",
                "styles": "style_colormapped_bilinear_multicolor_contours_extrasmooth_discretized",
            },
            {
                "filetocheck": "ahn_utrechtseheuvelrug_500m_style_hillshaded_opaque.png",
                "layers": "ahn_utrechtseheuvelrug_500m",
                "styles": "style_hillshaded_opaque/hillshaded",
            }, {
                "filetocheck": "ahn_utrechtseheuvelrug_500m_style_hillshaded_transparent.png",
                "layers": "ahn_utrechtseheuvelrug_500m",
                "styles": "style_hillshaded_transparent/hillshaded",
            },{
                "filetocheck": "ahn_utrechtseheuvelrug_500m_hillshaded_style_colormapped_bilinear_elevation.png",
                "layers":"ahn_utrechtseheuvelrug_500m_hillshaded",
                "styles": "style_colormapped_bilinear_elevation",
            },{
                "filetocheck": "ahn_utrechtseheuvelrug_500m_hillshaded_style_colormapped_bilinear_multicolor_contours.png",
                "layers":"ahn_utrechtseheuvelrug_500m_hillshaded",
                "styles": "style_colormapped_bilinear_multicolor_contours",
            },{
                "filetocheck": "ahn_utrechtseheuvelrug_500m_hillshaded_style_dutch_mountains.png",
                "layers":"ahn_utrechtseheuvelrug_500m_hillshaded",
                "styles": "style_dutch_mountains",
            },




            
        ]
        for test_case in test_cases:
            filename = test_case["filetocheck"]
            status, data, headers = AdagucTestTools().runADAGUCServer(
                f"LAYERS={test_case['layers']}&STYLES={test_case['styles']}&DATASET=adaguc.tests.ahn_utrechtse_heuvelrug_500m&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&WIDTH=512&HEIGHT=512&CRS=EPSG%3A3857&BBOX=590359.2826213799,6784102.0088095935,707368.84890462,6894816.225400406&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF&SHOWLEGEND=true",
                env=env,
                showLog=False,
            )
 
            AdagucTestTools().writetofile(
                self.testresultspath + filename, data.getvalue()
            )

            self.assertEqual(status, 0)
            self.assertTrue(
                AdagucTestTools().compareImage(
                    self.expectedoutputsspath + filename,
                    self.testresultspath + filename,
                    80,
                )
            )

   