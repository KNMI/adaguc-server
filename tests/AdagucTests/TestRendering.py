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

ADAGUC_PATH = os.environ['ADAGUC_PATH']


class TestRendering(unittest.TestCase):
    testresultspath = "testresults/TestRendering/"
    expectedoutputsspath = "expectedoutputs/TestRendering/"
    env = {'ADAGUC_CONFIG': ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," +
            ADAGUC_PATH + "/data/config/datasets/adaguc.testGeoJSONReader_time.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_RenderingGeoJSON_countries_autowms(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_RenderingGeoJSON_countries_autowms.png"
        env = {'ADAGUC_CONFIG': ADAGUC_PATH +
            "/data/config/adaguc.autoresource.xml"}
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=countries.geojson&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=features&WIDTH=256&HEIGHT=128&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=default&FORMAT=image/png&TRANSPARENT=TRUE", env=env, showLog=False)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools(
        ).readfromfile(self.expectedoutputsspath + filename))

    def test_RenderingGeoJSON_time_GetCapabilities(self):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + \
            ADAGUC_PATH + '/data/config/datasets/adaguc.testGeoJSONReader_time.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env={'ADAGUC_CONFIG': config}, isCGI=False, showLog=False)
        self.assertEqual(status, 0)
        # Test GetCapabilities
        filename = "test_RenderingGeoJSON_time_GetCapabilities.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities", env={'ADAGUC_CONFIG': config})
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(
            self.testresultspath + filename, self.expectedoutputsspath + filename))

    def test_RenderingGeoJSON_3timesteps(self):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + \
            ADAGUC_PATH + '/data/config/datasets/adaguc.testGeoJSONReader_time.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env={'ADAGUC_CONFIG': config}, isCGI=False, showLog=False)
        self.assertEqual(status, 0)

        dates = ['2018-12-04T12:00:00Z',
                '2018-12-04T12:05:00Z',
                '2018-12-04T12:10:00Z']

        for date in dates:
            filename = ("test_RenderingGeoJSON_3timesteps"+date+".png").replace(":","_")
        
            status, data, headers = AdagucTestTools().runADAGUCServer(
                "&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=features&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=40,-10,60,40&STYLES=features&FORMAT=image/png&TRANSPARENT=TRUE&TIME=" + date, env={'ADAGUC_CONFIG': config})
            AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
            self.assertEqual(status, 0)
            self.assertEqual(data.getvalue(), AdagucTestTools(
            ).readfromfile(self.expectedoutputsspath + filename))



    def test_RenderingGeoJSON_CTR_EHAM_Schiphol_GetMap_AutoWMS(self):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + 'data/config/adaguc.autoresource.xml'
        AdagucTestTools().runADAGUCServer(
            "source=geojsons/EHAM_Schiphol.geojson&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities", env={'ADAGUC_CONFIG': config})
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=geojsons/EHAM_Schiphol.geojson&service=WMS&request=getmap&format=image/png&layers=features&width=400&CRS=EPSG:4326&STYLES=&EXCEPTIONS=INIMAGE&showlegend=true&0.5975561120796958", env={'ADAGUC_CONFIG': config})        
        filename = "test_GeoJSON_CTR_EHAM_Schiphol_GetMap_AutoWMS.png"
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareImage(
            self.testresultspath + filename, self.expectedoutputsspath + filename))

    def test_RenderingGeoJSON_CTR_EHAM_Schiphol_GetMapNoPoints(self):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,adaguc.tests.CTRRendering.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env={'ADAGUC_CONFIG': config}, isCGI=False, showLog=False)
        self.assertEqual(status, 0)
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.CTRRendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=EHAM_Schiphol_onlyoutlines&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=480839.95441816666,6790323.698828231,596332.8754416516,6931740.434113708&STYLES=EHAM_Schiphol_filled%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&0.9458806689743082", env={'ADAGUC_CONFIG': config})        
        filename = "test_RenderingGeoJSON_CTR_EHAM_Schiphol_GetMapNoPoints.png"
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareImage(
            self.testresultspath + filename, self.expectedoutputsspath + filename))

    def test_RenderingGeoJSON_CTR_EHAM_Schiphol_GetMapOnlyLines(self):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,adaguc.tests.CTRRendering.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env={'ADAGUC_CONFIG': config}, isCGI=False, showLog=False)
        self.assertEqual(status, 0)
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.CTRRendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=EHAM_Schiphol_onlyoutlines&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=480839.95441816666,6790323.698828231,596332.8754416516,6931740.434113708&STYLES=EHAM_Schiphol_onlyoutlines%2Fpolylines&FORMAT=image/png&TRANSPARENT=TRUE&&0.9458806689743082", env={'ADAGUC_CONFIG': config})        
        filename = "test_RenderingGeoJSON_CTR_EHAM_Schiphol_GetMapOnlyLines.png"
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareImage(
            self.testresultspath + filename, self.expectedoutputsspath + filename))

    def test_RenderingGeoJSON_CTR_EHAM_Schiphol_GetMapLinesPointsAndNearest(self):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,adaguc.tests.CTRRendering.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env={'ADAGUC_CONFIG': config}, isCGI=False, showLog=False)
        self.assertEqual(status, 0)
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.CTRRendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=EHAM_Schiphol_onlyoutlines&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=480839.95441816666,6790323.698828231,596332.8754416516,6931740.434113708&STYLES=EHAM_Schiphol_filledoutlines_and_points%2Fnearestpolyline&FORMAT=image/png&TRANSPARENT=TRUE&&0.9458806689743082", env={'ADAGUC_CONFIG': config})        
        filename = "test_RenderingGeoJSON_CTR_EHAM_Schiphol_GetMapLinesPointsAndNearest.png"
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareImage(
            self.testresultspath + filename, self.expectedoutputsspath + filename))        


    def test_RenderingGeoJSON_CTR_EHAM_Schiphol_GetMapOnlyPoints(self):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,adaguc.tests.CTRRendering.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env={'ADAGUC_CONFIG': config}, isCGI=False, showLog=False)
        self.assertEqual(status, 0)
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.CTRRendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=EHAM_Schiphol_onlyoutlines&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=480839.95441816666,6790323.698828231,596332.8754416516,6931740.434113708&STYLES=EHAM_Schiphol_only_points%2Fpoint&FORMAT=image/png&TRANSPARENT=TRUE&&0.9458806689743082", env={'ADAGUC_CONFIG': config})        
        filename = "test_RenderingGeoJSON_CTR_EHAM_Schiphol_GetMapOnlyPoints.png"
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareImage(
            self.testresultspath + filename, self.expectedoutputsspath + filename))        

    def test_RenderingGeoJSON_CTR_EHAM_Schiphol_GetFeatureInfoNoPoints(self):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,adaguc.tests.CTRRendering.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env={'ADAGUC_CONFIG': config}, isCGI=False, showLog=False)
        self.assertEqual(status, 0)
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.CTRRendering&&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=EHAM_Schiphol_onlyoutlines&QUERY_LAYERS=EHAM_Schiphol_onlyoutlines&CRS=EPSG%3A3857&BBOX=463835.69270030205,6790367.198023051,579328.613723787,6905987.034344364&WIDTH=910&HEIGHT=911&I=493&J=439&FORMAT=image/gif&INFO_FORMAT=application/json&STYLES=&", env={'ADAGUC_CONFIG': config})        
        filename = "test_RenderingGeoJSON_CTR_EHAM_Schiphol_GetFeatureInfoNoPoints.json"
        AdagucTestTools().writetojson(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareFile(
            self.testresultspath + filename, self.expectedoutputsspath + filename))        


    def test_RenderingGeoJSON_CTR_EHAM_Schiphol_GetFeatureInfoExactlyOnPoint(self):
        """
        Note: https://github.com/KNMI/adaguc-server/issues/544
        The getfeatureinfo should return both the point and the polygon info.

        """
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,adaguc.tests.CTRRendering.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env={'ADAGUC_CONFIG': config}, isCGI=False, showLog=False)
        self.assertEqual(status, 0)
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.CTRRendering&&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=EHAM_Schiphol_onlyoutlines&QUERY_LAYERS=EHAM_Schiphol_onlyoutlines&CRS=EPSG%3A3857&BBOX=444505.27315159835,6795686.027258951,579300.9326094447,6930629.813815102&WIDTH=910&HEIGHT=911&I=571&J=489&FORMAT=image/gif&INFO_FORMAT=application/jsonl&STYLES=&", env={'ADAGUC_CONFIG': config})        
        filename = "test_RenderingGeoJSON_CTR_EHAM_Schiphol_GetFeatureInfoExactlyOnPoint.json"
        AdagucTestTools().writetojson(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareFile(
            self.testresultspath + filename, self.expectedoutputsspath + filename))        

        

