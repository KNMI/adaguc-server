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
from .AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ['ADAGUC_PATH']

class TestWMSPolylineRenderer(unittest.TestCase):
    testresultspath = "testresults/TestWMSPolylineRenderer/"
    expectedoutputsspath = "expectedoutputs/TestWMSPolylineRenderer/"
    env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"}
    
    AdagucTestTools().mkdir_p(testresultspath);
    

    def test_WMSPolylineRenderer_borderwidth_1px(self):
        AdagucTestTools().cleanTempDir()

        config =  ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + ADAGUC_PATH + '/data/config/datasets/adaguc.testwmspolylinerenderer.xml'
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', config], env = self.env, isCGI = False)
        self.assertEqual(status, 0)
        
        stylenames = ["polyline_black_0.5px","polyline_blue_0.5px","polyline_blue_1px","polyline_blue_2px","polyline_yellow_2px","polyline_red_6px"];
        for stylename in stylenames:
          sys.stdout.write ("\ntest style %s " % stylename)
          sys.stdout.flush()
          filename="test_WMSPolylineRenderer_"+stylename+".png"
          status,data,headers = AdagucTestTools().runADAGUCServer("DATASET=adaguc.testwmspolylinerenderer&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=neddis&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=51.05009024158416,2.254746,54.44300975841584,7.054254&STYLES="+stylename+"%2Fpolyline&FORMAT=image/png&TRANSPARENT=TRUE&", env = self.env)
          AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
          self.assertEqual(status, 0)
          self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
