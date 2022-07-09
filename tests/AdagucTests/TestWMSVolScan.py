import os
import os.path
from io import BytesIO
import unittest
import shutil
import subprocess
import json
from lxml import etree, objectify
import re
from adaguc.AdagucTestTools import AdagucTestTools
from lxml import etree, objectify
import datetime

ADAGUC_PATH = os.environ['ADAGUC_PATH']


class TestWMSVolScan(unittest.TestCase):
    testresultspath = "testresults/TestWMSVolScan/"
    expectedoutputsspath = "expectedoutputs/TestWMSVolScan/"
    env = {'ADAGUC_CONFIG': ADAGUC_PATH +
           "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def compareXML(self, xml, expectedxml):
        obj1 = objectify.fromstring(
            re.sub(' xmlns="[^"]+"', '', expectedxml, count=1))
        obj2 = objectify.fromstring(re.sub(' xmlns="[^"]+"', '', xml, count=1))

        # Remove ADAGUC build date and version from keywordlists
        for child in obj1.findall("Service/KeywordList")[0]:
            child.getparent().remove(child)
        for child in obj2.findall("Service/KeywordList")[0]:
            child.getparent().remove(child)

        # Boundingbox extent values are too varying by different Proj libraries
        def removeBBOX(root):
            if (root.tag.title() == "Boundingbox"):
                # root.getparent().remove(root)
                try:
                    del root.attrib["minx"]
                    del root.attrib["miny"]
                    del root.attrib["maxx"]
                    del root.attrib["maxy"]
                except:
                    pass
            for elem in root.getchildren():
                removeBBOX(elem)

        removeBBOX(obj1)
        removeBBOX(obj2)

        result = etree.tostring(obj1)
        expect = etree.tostring(obj2)

        self.assertEquals(expect, result)

    def checkReport(self, reportFilename="", expectedReportFilename=""):
        self.assertTrue(os.path.exists(reportFilename))
        self.assertEqual(AdagucTestTools().readfromfile(reportFilename),
                         AdagucTestTools().readfromfile(self.expectedoutputsspath + expectedReportFilename))
        os.remove(reportFilename)

    def test_WMSGetCapabilities_VolScan(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetCapabilities_VolScan.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=test/volscan/RAD_NL62_VOL_NA_202106181850.h5&SERVICE=WMS&request=getcapabilities", env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(
            self.testresultspath + filename, self.expectedoutputsspath + filename))

    def test_WMSGetMapKDP_VolScan(self):
        AdagucTestTools().cleanTempDir()
        for layer in ["KDP","RhoHV"]:
            for elev in ["0.3", "20.0"]:
                filename = f"test_WMSGetMap_VolScan_{layer}_{elev}.png"
                wms_arg=f"source=test/volscan/RAD_NL62_VOL_NA_202106181850.h5&SERVICE=WMS&request=getmap&LAYERS={layer}&format=image/png&STYLES=&WMS=1.3.0&CRS=EPSG:4326&BBOX=46,0,58,12&WIDTH=400&HEIGHT=400&SHOWDIMS=true&DIM_scan_elevation={elev}"
                status, data, headers = AdagucTestTools().runADAGUCServer(
                    wms_arg, env=self.env)
                AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
                self.assertEqual(status, 0)
                self.assertEqual(data.getvalue(), AdagucTestTools(
                ).readfromfile(self.expectedoutputsspath + filename))