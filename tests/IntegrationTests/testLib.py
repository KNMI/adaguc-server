import os
import os.path
import unittest
from lxml import etree, objectify
from adaguc.AdagucTestTools import AdagucTestTools


class TestWMSGetCapabilities(unittest.TestCase):
  testresultspath = "IntegrationTests/testresults/TestWMS/"
  expectedoutputsspath = "IntegrationTests/expectedresults/TestWMS/"
  AdagucTestTools().mkdir_p(testresultspath)
  # do with test data
  url = "https://geoservices.knmi.nl/adagucserver?source=testdata.nc&&service=WMS"

  # Make sure to define the ADAGUC PATH in order to run the tests
  # os.environ['ADAGUC_PATH'] = "/home/belen/devel/adaguc/adaguc-server2"

  def test_WMSGetCapabilities_testdatanc(self):
    filename = "test_WMSGetCapabilities_testdatanc.xml"
    status, data, _ = AdagucTestTools().runRemoteADAGUCServer(
        self.url + "&request=GetCapabilities")
    AdagucTestTools().writetofile(self.testresultspath + filename, data)
    self.assertEqual(status, 200)
    self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(
        self.testresultspath + filename, self.expectedoutputsspath + filename))

  def test_WMSGetCapabilitiesGetMap_testdatanc(self):
    AdagucTestTools().cleanTempDir()
    filename = "test_WMSGetCapabilities_testdatanc.xml"
    status, data, _ = AdagucTestTools().runRemoteADAGUCServer(
        self.url + "&request=GetCapabilities")
    AdagucTestTools().writetofile(self.testresultspath + filename, data)
    self.assertEqual(status, 200)
    self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(
        self.testresultspath + filename, self.expectedoutputsspath + filename))
    filename = "test_WMSGetMap_testdatanc.png"
    status, data, _ = AdagucTestTools().runRemoteADAGUCServer(
        self.url + "&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&")
    AdagucTestTools().writetofile(self.testresultspath + filename, data)
    self.assertEqual(status, 200)
    self.assertEqual(data, AdagucTestTools(
    ).readfromfile(self.expectedoutputsspath + filename))

  def test_WMSGetMapGetCapabilities_testdatanc(self):
    AdagucTestTools().cleanTempDir()
    filename = "test_WMSGetMap_testdatanc.png"
    status, data, _ = AdagucTestTools().runRemoteADAGUCServer(
        self.url + "&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&")
    AdagucTestTools().writetofile(self.testresultspath + filename, data)
    self.assertEqual(status, 200)
    self.assertEqual(data, AdagucTestTools(
    ).readfromfile(self.expectedoutputsspath + filename))
    filename = "test_WMSGetCapabilities_testdatanc.xml"
    status, data, _ = AdagucTestTools().runRemoteADAGUCServer(
        self.url + "request=getcapabilities")
    AdagucTestTools().writetofile(self.testresultspath + filename, data)
    self.assertEqual(status, 200)
    self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(
        self.testresultspath + filename, self.expectedoutputsspath + filename))
