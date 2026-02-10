# pylint: disable=line-too-long
# pylint: disable=unused-variable
# pylint: disable=invalid-name

"""
Run test for OpenDap Server system of adaguc-server
"""

import os
import unittest
from adaguc.AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestOpenDAPServer(unittest.TestCase):
    """
    Run test for OpenDap Server system of adaguc-server
    """

    testresultspath = "testresults/TestOpenDAP/"
    expectedoutputsspath = "expectedoutputs/TestOpenDAP/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.opendapserver.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_OpenDAPServer_testdatanc_DDS(self):
        """
        Check if Data Description Service works
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_OpenDAPServer_testdatanc_DDS.txt"
        status, data, headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.dds", env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_OpenDAPServer_testdatanc_DDS_headers(self):
        """
        Check if OpenDAP headers for DDS are OK
        """
        AdagucTestTools().cleanTempDir()
        status, data, headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.dds", env=self.env)
        self.assertEqual(status, 0)

        # print("\nHEADERS\n" + str(headers) + "\n")
        self.assertEqual(headers, ["XDAP: 2.0 ", "Content-Description: dods-dds ", "Content-Type: text/plain; charset=utf-8"])

    def test_OpenDAPServer_testdatanc_DAS(self):
        """
        Check if OpenDAP DAS Data Attribute Service has correct response
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_OpenDAPServer_testdatanc_DAS.txt"
        status, data, headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.das", env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_OpenDAPServer_testdatanc_DODS(self):
        """
        Check if OpenDAP DODS Data Service has correct response
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_OpenDAPServer_testdatanc_DODS.txt"
        status, data, headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.dods", env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_OpenDAPServer_testdatanc_DODS_Query_testdatayxtimeprojection(self):
        """
        Check if OpenDAP DODS Data Service has correct response when querying
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_OpenDAPServer_testdatanc_DODS_Query_testdatayxtimeprojection.txt"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            path="opendap/testdata.nc.dods", url="testdata,y,x,time,projection", env=self.env
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_OpenDAPServer_testdatanc_DODS_Query_testdatay(self):
        """
        Check if OpenDAP DODS Data Service has correct response when querying testdatay
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_OpenDAPServer_testdatanc_DODS_Query_testdatay.txt"
        status, data, headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.dods", url="testdata,y", env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_OpenDAPServer_testdatanc_DODS_Query_projection(self):
        """
        Check if OpenDAP DODS Data Service has correct response when querying projection
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_OpenDAPServer_testdatanc_DODS_Query_projection.txt"
        status, data, headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.dods", url="projection", env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_OpenDAPServer_testdatanc_DODS_Query_testdata_subset(self):
        """
        Check if OpenDAP DODS Data Service has correct response when getting a subset of the data
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_OpenDAPServer_testdatanc_DODS_Query_testdata_subset.txt"
        status, data, headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.dods", url="testdata[1:5][8:17]", env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
