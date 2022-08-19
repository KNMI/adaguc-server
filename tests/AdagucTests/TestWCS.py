# pylint: disable=line-too-long
# pylint: disable=unused-variable
# pylint: disable=invalid-name

"""
  Run test for WCS Server system of adaguc-server
"""


import os
import os.path
import unittest
from adaguc.AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ['ADAGUC_PATH']


class TestWCS(unittest.TestCase):
  """
  Run test for WCS Server system of adaguc-server
  """
  testresultspath = "testresults/TestWCS/"
  expectedoutputsspath = "expectedoutputs/TestWCS/"
  env = {'ADAGUC_CONFIG': ADAGUC_PATH +
         "/data/config/adaguc.autoresource.xml"}

  AdagucTestTools().mkdir_p(testresultspath)

  def test_WCSGetCapabilities_testdatanc(self):
    """
    Check if WCS GetCapabilities for testdata.nc file is OK
    """
    AdagucTestTools().cleanTempDir()
    filename = "test_WCSGetCapabilities_testdatanc"
    status, data, headers = AdagucTestTools().runADAGUCServer(
        "source=testdata.nc&SERVICE=WCS&request=getcapabilities", env=self.env)
    AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
    self.assertEqual(status, 0)
    self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(
        self.testresultspath + filename, self.expectedoutputsspath + filename))

  def test_WCSGetCoverageAAIGRID_testdatanc(self):
    """
    Check if WCS GetCoverage for testdata.nc as AAIGRID file is OK
    """
    AdagucTestTools().cleanTempDir()
    filename = "test_WCSGetCoverageAAIGRID_testdatanc.grd"
    status, data, headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=aaigrid&BBOX=-180,-90,180,90&RESX=1&RESY=1",
                                                              env=self.env, args=["--report"])
    AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
    self.assertEqual(status, 0)
    self.assertEqual(data.getvalue(), AdagucTestTools(
    ).readfromfile(self.expectedoutputsspath + filename))

  def test_WCSGetCoverageNetCDF3_testdatanc(self):
    """
    Check if WCS GetCoverage for testdata.nc as NetCDF3 file is OK
    """
    AdagucTestTools().cleanTempDir()
    status, data, headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=NetCDF3&BBOX=-180,-90,180,90&RESX=1&RESY=1",
                                                              env=self.env, args=["--report"])
    self.assertEqual(status, 0)
    self.assertEqual(data.getvalue()[0:10],
                     b'CDF\x02\x00\x00\x00\x00\x00\x00')

  def test_WCSGetCoverageNetCDF4_testdatanc(self):
    """
    Check if WCS GetCoverage for testdata.nc as NetCDF4 file is OK
    """
    AdagucTestTools().cleanTempDir()
    status, data, headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=NetCDF4&BBOX=-180,-90,180,90&RESX=1&RESY=1",
                                                              env=self.env, args=["--report"])
    self.assertEqual(status, 0)
    self.assertEqual(data.getvalue()[0:6], b'\x89HDF\r\n')

  def test_WCSGetCoverageGeoTiff_testdatanc(self):
    """
    Check if WCS GetCoverage for testdata.nc as TIFF file is OK
    """
    AdagucTestTools().cleanTempDir()
    filename = "test_WCSGetCoverageGeoTiff_testdatanc.tiff"
    status, data, headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=geotiff&BBOX=-180,-90,180,90&RESX=1&RESY=1",
                                                              env=self.env, args=["--report"])
    self.assertEqual(status, 0)
    self.assertEqual(data.getvalue()[0:10],
                     b'II*\x00\x08\x00\x00\x00\x12\x00')

  def test_WCSGetCoverageAAIGRID_NATIVE_testdatanc(self):
    """
    Check if WCS GetCoverage for testdata.nc as Native grid is OK
    """
    AdagucTestTools().cleanTempDir()
    filename = "test_WCSGetCoverageAAIGRID_NATIVE_testdatanc.grd"
    status, data, headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&FORMAT=aaigrid&",
                                                              env=self.env, args=["--report"])
    AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
    self.assertEqual(status, 0)
    self.assertEqual(data.getvalue(), AdagucTestTools(
    ).readfromfile(self.expectedoutputsspath + filename))
