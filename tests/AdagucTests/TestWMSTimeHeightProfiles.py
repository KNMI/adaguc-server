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

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestWMSTimeHeightProfiles(unittest.TestCase):
    testresultspath = "testresults/TestWMSTimeHeightProfiles/"
    expectedoutputsspath = "expectedoutputs/TestWMSTimeHeightProfiles/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def compareXML(self, xml, expectedxml):
        obj1 = objectify.fromstring(re.sub(' xmlns="[^"]+"', "", expectedxml, count=1))
        obj2 = objectify.fromstring(re.sub(' xmlns="[^"]+"', "", xml, count=1))

        # Remove ADAGUC build date and version from keywordlists
        for child in obj1.findall("Service/KeywordList")[0]:
            child.getparent().remove(child)
        for child in obj2.findall("Service/KeywordList")[0]:
            child.getparent().remove(child)

        # Boundingbox extent values are too varying by different Proj libraries
        def removeBBOX(root):
            if root.tag.title() == "Boundingbox":
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

    def test_WMSGetCapabilities_TimeHeightProfiles(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetCapabilities_TimeHeightProfiles.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=test/ceilonet/ceilonet_chm15k_backsct_la1_t12s_v1.0_06310_A20231202_extracted_small.nc&SERVICE=WMS&request=getcapabilities",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSGetMap_TimeHeightProfiles(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_TimeHeightProfiles_4326_beta_raw.png"
        wms_arg = "source=test/ceilonet/ceilonet_chm15k_backsct_la1_t12s_v1.0_06310_A20231202_extracted_small.nc&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=beta_raw&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=259793.6688373399,6178346.966712794,1082625.1470173844,7357680.642536524&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&time=2023-12-02T06%3A40%3A00Z"
        status, data, headers = AdagucTestTools().runADAGUCServer(wms_arg, env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
            self.expectedoutputsspath + filename,
        )

    def test_WMSGetFeatureInfoAsPng_TimeHeightProfiles(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetFeatureInfoAsPng_TimeHeightProfiles.png"
        wms_arg = "source=test/ceilonet/ceilonet_chm15k_backsct_la1_t12s_v1.0_06310_A20231202_extracted_small.nc&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=beta_raw&QUERY_LAYERS=beta_raw&BBOX=29109.947643979103,6500000,1190890.052356021,7200000&CRS=EPSG%3A3857&WIDTH=128&HEIGHT=512&I=707&J=557&FORMAT=image/gif&INFO_FORMAT=image/png&STYLES=&time=2023-12-02T06:40:00Z/2023-12-02T06:45:00Z&elevation=0/2500"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=test/ceilonet/ceilonet_chm15k_backsct_la1_t12s_v1.0_06310_A20231202_extracted_small.nc&SERVICE=WMS&request=getcapabilities",
            env=self.env,
        )
        status, data, headers = AdagucTestTools().runADAGUCServer(wms_arg, env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
            self.expectedoutputsspath + filename,
        )
