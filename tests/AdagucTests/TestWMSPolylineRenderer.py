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


class TestWMSPolylineRenderer(unittest.TestCase):
    testresultspath = "testresults/TestWMSPolylineRenderer/"
    expectedoutputsspath = "expectedoutputs/TestWMSPolylineRenderer/"
    env = {'ADAGUC_CONFIG': ADAGUC_PATH +
            "/data/config/adaguc.tests.dataset.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_WMSPolylineRenderer_borderwidth_1px(self):
        AdagucTestTools().cleanTempDir()

        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + \
            ADAGUC_PATH + '/data/config/datasets/adaguc.testwmspolylinerenderer.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        stylenames = ["polyline_black_0.5px", "polyline_blue_0.5px", "polyline_blue_1px",
                    "polyline_blue_2px", "polyline_yellow_2px", "polyline_red_6px"]
        for stylename in stylenames:
            sys.stdout.write("\ntest style %s " % stylename)
            sys.stdout.flush()
            filename = "test_WMSPolylineRenderer_"+stylename+".png"
            status, data, headers = AdagucTestTools().runADAGUCServer("DATASET=adaguc.testwmspolylinerenderer&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=neddis&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=51.05009024158416,2.254746,54.44300975841584,7.054254&STYLES=" +
                                                                        stylename+"%2Fpolyline&FORMAT=image/png&TRANSPARENT=TRUE&", env=self.env)
            AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
            self.assertEqual(status, 0)
            self.assertTrue(AdagucTestTools().compareImage(self.expectedoutputsspath + filename, self.testresultspath + filename, maxAllowedColorDifference=2, maxAllowedColorPercentage=0.025))


    def test_WMSPolylineRenderer_cellwarn_fillpolysalpha(self):
        AdagucTestTools().cleanTempDir()

        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + \
            ADAGUC_PATH + '/data/config/datasets/adaguc.testwmspolylinerenderer.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename = "test_WMSPolylineRenderer_cellwarn_fillpolysalpha.png"
        status, data, headers = AdagucTestTools().runADAGUCServer("DATASET=adaguc.testwmspolylinerenderer&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=cellwarn_hail_combined&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=540000,6600000,850000,6750000&STYLES=polygon_innerparts_coloured&FORMAT=image/png&TRANSPARENT=TRUE&", env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareImage(self.expectedoutputsspath + filename, self.testresultspath + filename, maxAllowedColorDifference=2, maxAllowedColorPercentage=0.025))


    def test_WMSPointRenderer_usgs_earthquakes_geojson_auto_nearest_GetMap(self):
        env = {'ADAGUC_CONFIG': ADAGUC_PATH +
           "/data/config/adaguc.autoresource.xml"}

        filename="test_WMSPointRenderer_usgs_earthquakes_geojson_auto_nearest_GetMap.png"
        status, data, headers = AdagucTestTools().runADAGUCServer("source=usgs_earthquakes.geojson&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,mag&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=-21270915.530506067,-12592019.023145191,-1311264.2622161265,17664837.49309645&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&", env=env,  showLog=True)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools(
        ).readfromfile(self.expectedoutputsspath + filename))


    def test_WMSPointRenderer_usgs_earthquakes_geojson_auto_point_GetMap(self):
        env = {'ADAGUC_CONFIG': ADAGUC_PATH +
           "/data/config/adaguc.autoresource.xml"}

        filename="test_WMSPointRenderer_usgs_earthquakes_geojson_auto_point_GetMap.png"
        status, data, headers = AdagucTestTools().runADAGUCServer("source=usgs_earthquakes.geojson&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,mag&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=-21270915.530506067,-12592019.023145191,-1311264.2622161265,17664837.49309645&STYLES=auto%2Fpoint&FORMAT=image/png&TRANSPARENT=TRUE&", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools(
        ).readfromfile(self.expectedoutputsspath + filename))

    def test_WMSPointRenderer_usgs_earthquakes_geojson_auto_nearest_GetMapColorScaleRange0_20(self):
        env = {'ADAGUC_CONFIG': ADAGUC_PATH +
           "/data/config/adaguc.autoresource.xml"}

        filename="test_WMSPointRenderer_usgs_earthquakes_geojson_auto_nearest_GetMapColorScaleRange0_20.png"
        status, data, headers = AdagucTestTools().runADAGUCServer("source=usgs_earthquakes.geojson&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,mag&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=-21270915.530506067,-12592019.023145191,-1311264.2622161265,17664837.49309645&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&e&colorscalerange=0,20", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools(
        ).readfromfile(self.expectedoutputsspath + filename))


    def test_WMSPointRenderer_usgs_earthquakes_geojson_auto_point_GetMapColorScaleRange0_20(self):
        env = {'ADAGUC_CONFIG': ADAGUC_PATH +
           "/data/config/adaguc.autoresource.xml"}

        filename="test_WMSPointRenderer_usgs_earthquakes_geojson_auto_point_GetMapColorScaleRange0_20.png"
        status, data, headers = AdagucTestTools().runADAGUCServer("source=usgs_earthquakes.geojson&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,mag&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=-21270915.530506067,-12592019.023145191,-1311264.2622161265,17664837.49309645&STYLES=auto%2Fpoint&FORMAT=image/png&TRANSPARENT=TRUE&e&colorscalerange=0,20", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools(
        ).readfromfile(self.expectedoutputsspath + filename))

    def test_WMSPointRenderer_usgs_earthquakes_geojson_GetFeatureInfoJSON(self):
        env = {'ADAGUC_CONFIG': ADAGUC_PATH +
           "/data/config/adaguc.autoresource.xml"}

        filename="test_WMSPointRenderer_usgs_earthquakes_geojson_GetFeatureInfoJSON.json"
        status, data, headers = AdagucTestTools().runADAGUCServer("source=usgs_earthquakes.geojson&&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=mag&QUERY_LAYERS=mag&CRS=EPSG%3A3857&BBOX=-23104664.410278287,-14378748.700871969,-3145013.1419883464,15878107.815369671&WIDTH=849&HEIGHT=1287&I=460&J=438&FORMAT=image/gif&INFO_FORMAT=application/json&", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools(
        ).readfromfile(self.expectedoutputsspath + filename))

    def test_WMSPointRenderer_usgs_earthquakes_geojson_GetFeatureInfoJSONOutsidePoint(self):
        env = {'ADAGUC_CONFIG': ADAGUC_PATH +
           "/data/config/adaguc.autoresource.xml"}

        filename="test_WMSPointRenderer_usgs_earthquakes_geojson_GetFeatureInfoJSONOutsidePoint.json"
        status, data, headers = AdagucTestTools().runADAGUCServer("source=usgs_earthquakes.geojson&&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=mag&QUERY_LAYERS=mag&CRS=EPSG%3A3857&BBOX=-15726290.605935914,1725196.0890940144,-10964171.442404782,6983730.098762937&WIDTH=825&HEIGHT=911&I=248&J=368&FORMAT=image/gif&INFO_FORMAT=application/json&STYLES=&", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools(
        ).readfromfile(self.expectedoutputsspath + filename))


    def test_WMSPointRenderer_usgs_earthquakes_geojson_auto_nearest_GetLegendGraphic(self):
        env = {'ADAGUC_CONFIG': ADAGUC_PATH +
           "/data/config/adaguc.autoresource.xml"}

        filename="test_WMSPointRenderer_usgs_earthquakes_geojson_auto_nearest_GetLegendGraphic.png"
        status, data, headers = AdagucTestTools().runADAGUCServer("source%3Dusgs_earthquakes%2Egeojson&SERVICE=WMS&&version=1.1.1&service=WMS&request=GetLegendGraphic&layer=mag&format=image/png&STYLE=auto/nearest&layers=mag&&&transparent=true", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools(
        ).readfromfile(self.expectedoutputsspath + filename))
        
    def test_WMSPointRenderer_usgs_earthquakes_geojson_auto_nearest_GetLegendGraphicColorScaleRange0_20(self):
        env = {'ADAGUC_CONFIG': ADAGUC_PATH +
           "/data/config/adaguc.autoresource.xml"}

        filename="test_WMSPointRenderer_usgs_earthquakes_geojson_auto_nearest_GetLegendGraphicColorScaleRange0_20.png"
        status, data, headers = AdagucTestTools().runADAGUCServer("source%3Dusgs_earthquakes%2Egeojson&SERVICE=WMS&&version=1.1.1&service=WMS&request=GetLegendGraphic&layer=mag&format=image/png&STYLE=auto/nearest&layers=mag&&&transparent=true&colorscalerange=0,20", env=env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools(
        ).readfromfile(self.expectedoutputsspath + filename))

    def test_WMSPointRenderer_usgs_earthquakes_geojson_GetMap_MultiVar(self):
        AdagucTestTools().cleanTempDir()

        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + \
            ADAGUC_PATH + '/data/config/datasets/adaguc.testGeoJSONReaderMultivariable.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename="test_WMSPointRenderer_usgs_earthquakes_geojson_GetMap_MultiVar.png"
        status, data, headers = AdagucTestTools().runADAGUCServer("DATASET=adaguc.testGeoJSONReaderMultivariable&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=usgs_earthquakes_age_magnitude_geojson&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=500571.08859811374,5790835.942920016,1024650.2598764997,7187305.365464885&STYLES=age_magnitude_triangles%2Fpoint&FORMAT=image/png&TRANSPARENT=TRUE&&time=2022-09-22T07%3A25%3A20Z&", env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools(
        ).readfromfile(self.expectedoutputsspath + filename))
