# pylint: disable=invalid-name,missing-function-docstring
"""
This class contains tests to test the adaguc-server binary executable file. This is similar to black box testing, it tests the behaviour of the server software. It configures the server and checks if the response is OK.
"""
import os
import os.path
import unittest
import json
import re
import datetime
from adaguc.AdagucTestTools import AdagucTestTools
from lxml import etree, objectify
import pytest

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestWMS(unittest.TestCase):
    """
    TestWMS class to thest Web Map Service behaviour of adaguc-server.
    """

    testresultspath = "testresults/TestWMS/"
    expectedoutputsspath = "expectedoutputs/TestWMS/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def checkreport(self, report_filename="", expected_report_filename=""):
        """
        Tests file check reporting functionality
        """
        self.assertTrue(os.path.exists(report_filename))
        self.assertEqual(
            AdagucTestTools().readfromfile(report_filename),
            AdagucTestTools().readfromfile(
                self.expectedoutputsspath + expected_report_filename
            ),
        )
        os.remove(report_filename)

    def test_WMSGetCapabilities_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetCapabilities_testdatanc.xml"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&request=getcapabilities", env=self.env
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSGetMap_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_testdatanc.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_testdatanc_withplus(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_testdatanc_plus.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata%2B001.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_geos(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_testgeosnc.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testgeos.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=ct&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=40,-3,45,3&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())

        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_testdatanc_autoheight(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_testdatanc.png_autoheight.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&service=WMS&request=getmap&format=image/png&layers=testdata&width=256&CRS=EPSG:4326&STYLES=&EXCEPTIONS=INIMAGE&showlegend=true&",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetLegendGraphic_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetLegendGraphic_testdatanc.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&VERSION=1.3.0&request=GetLegendGraphic&layer=testdata&format=image/png&STYLE=auto/nearest&layers=testdata&transparent=true",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_Report_env(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_Report_env"
        reportfilename = "./env_checker_report.txt"
        self.env["ADAGUC_CHECKER_FILE"] = reportfilename
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(os.path.exists(reportfilename))
        self.env.pop("ADAGUC_CHECKER_FILE", None)
        if os.path.exists(reportfilename):
            os.remove(reportfilename)

    def test_WMSGetMap_testdatanc_customprojectionstring(self):
        AdagucTestTools().cleanTempDir()

        # https://geoservices.knmi.nl/cgi-bin/RADNL_OPER_R___25PCPRR_L3.cgi?SERVICE=WMS&REQUEST=GETMAP&VERSION=1.1.1&SRS%3DPROJ4%3A%2Bproj%3Dstere%20%2Bx_0%3D0%20%2By_0%3D0%20%2Blat_ts%3D60%20%2Blon_0%3D0%20%2Blat_0%3D90%20%2Ba%3D6378140%20%2Bb%3D6356750%20%2Bunits%3Dm&FORMAT=image/png&TRANSPARENT=true&WIDTH=750&HEIGHT=660&BBOX=100000,-4250000,600000,-3810000&LAYERS=RADNL_OPER_R___25PCPRR_L3_KNMI&TIME=2018-03-12T12:40:00

        filename = "test_WMSGetMap_testdatanc_customprojectionstring.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=%2Bproj%3Dstere%20%2Bx_0%3D0%20%2By_0%3D0%20%2Blat_ts%3D60%20%2Blon_0%3D0%20%2Blat_0%3D90%20%2Ba%3D6378140%20%2Bb%3D6356750%20%2Bunits%3Dm&BBOX=100000,-4250000,600000,-3810000&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_testdatanc_customprojectionstring_proj4namespace(self):
        AdagucTestTools().cleanTempDir()

        filename = "test_WMSGetMap_testdatanc_customprojectionstring_proj4namespace.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=PROJ4%3A%2Bproj%3Dstere%20%2Bx_0%3D0%20%2By_0%3D0%20%2Blat_ts%3D60%20%2Blon_0%3D0%20%2Blat_0%3D90%20%2Ba%3D6378140%20%2Bb%3D6356750%20%2Bunits%3Dm&BBOX=100000,-4250000,600000,-3810000&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetCapabilitiesGetMap_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetCapabilitiesGetMap_testdatanc_WMSGetCapabilities_testdatanc.xml"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&request=getcapabilities", env=self.env
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )
        filename = "test_WMSGetCapabilitiesGetMap_testdatanc.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMapGetCapabilities_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapGetCapabilities_testdatanc.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )
        filename = "test_WMSGetMapGetCapabilities_testdatanc_WMSGetCapabilities_testdatanc.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&request=getcapabilities", env=self.env
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSGetMap_getmap_3dims_singlefile(self):
        dims = {
            "time": {
                "vartype": "d",
                "units": "seconds since 1970-01-01 00:00:00",
                "standard_name": "time",
                "values": [
                    "2017-01-01T00:00:00Z",
                    "2017-01-01T00:05:00Z",
                    "2017-01-01T00:10:00Z",
                ],
                "wmsname": "time",
            },
            "elevation": {
                "vartype": "d",
                "units": "meters",
                "standard_name": "height",
                "values": [7000, 8000, 9000],
                "wmsname": "elevation",
            },
            "member": {
                "vartype": str,
                "units": "member number",
                "standard_name": "member",
                "values": ["member5", "member4"],
                "wmsname": "DIM_member",
            },
        }

        AdagucTestTools().cleanTempDir()

        def Recurse(dims, number, l):
            for value in range(len(dims[list(dims.keys())[number - 1]]["values"])):
                l[number - 1] = value
                if number > 1:
                    Recurse(dims, number - 1, l)
                else:
                    kvps = ""
                    for i in reversed(range(len(l))):
                        key = dims[list(dims)[i]]["wmsname"]
                        value = (dims[list(dims)[i]]["values"])[l[i]]
                        kvps += "&" + key + "=" + str(value)
                    # print("Checking dims" + kvps)
                    filename = "test_WMSGetMap_getmap_3dims_" + kvps + ".png"
                    filename = (
                        filename.replace("&", "_").replace(":", "_").replace("=", "_")
                    )
                    # print filename
                    url = "source=netcdf_5dims%2Fnetcdf_5dims_seq1%2Fnc_5D_20170101000000-20170101001000.nc&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&"
                    url += kvps
                    # pylint: disable=unused-variable
                    status, data, headers = AdagucTestTools().runADAGUCServer(
                        url, env=self.env
                    )
                    AdagucTestTools().writetofile(
                        self.testresultspath + filename, data.getvalue()
                    )
                    self.assertEqual(status, 0)
                    self.assertEqual(
                        data.getvalue(),
                        AdagucTestTools().readfromfile(
                            self.expectedoutputsspath + filename
                        ),
                    )

        l = []

        # pylint: disable=unused-variable
        for i in range(len(dims)):
            l.append(0)
        Recurse(dims, len(dims), l)

    def test_WMSCMDUpdateDBNoConfig(self):
        AdagucTestTools().cleanTempDir()
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb"], env=self.env, isCGI=False, showLogOnError=False
        )
        self.assertEqual(status, 1)

    def test_WMSCMDUpdateDB(self):
        AdagucTestTools().cleanTempDir()
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.timeseries.xml",
            ],
            isCGI=False,
            showLogOnError=False,
        )
        self.assertEqual(status, 0)

        filename = "test_WMSCMDUpdateDB_WMSGetCapabilities_timeseries_twofiles.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&request=getcapabilities",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.timeseries.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSCMDUpdateDBTailPath(self):
        AdagucTestTools().cleanTempDir()
        AdagucTestTools().cleanPostgres()
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.timeseries.xml",
                "--tailpath",
                "netcdf_5dims_seq1",
            ],
            isCGI=False,
            showLogOnError=False,
        )
        self.assertEqual(status, 0)

        filename = "test_WMSCMDUpdateDBTailPath_WMSGetCapabilities_timeseries_tailpath_netcdf_5dims_seq1.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&request=getcapabilities",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.timeseries.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.timeseries.xml",
                "--tailpath",
                "netcdf_5dims_seq2",
            ],
            isCGI=False,
            showLogOnError=False,
        )
        self.assertEqual(status, 0)

        filename = (
            "test_WMSCMDUpdateDBTailPath_WMSGetCapabilities_timeseries_tailpath_netcdf_5dims_seq1_and_seq2.xml"
        )
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&request=getcapabilities",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.timeseries.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSCMDUpdateDBPath(self):
        AdagucTestTools().cleanTempDir()
        AdagucTestTools().cleanPostgres()
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.timeseries.xml",
                "--path",
                ADAGUC_PATH
                + "/data/datasets/netcdf_5dims/netcdf_5dims_seq1/nc_5D_20170101000000-20170101001000.nc",
            ],
            isCGI=False,
            showLogOnError=False,
            showLog=False,
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetCapabilities_timeseries_path_netcdf_5dims_seq1.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&request=getcapabilities",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.timeseries.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.timeseries.xml",
                "--path",
                ADAGUC_PATH
                + "/data/datasets/netcdf_5dims/netcdf_5dims_seq2/nc_5D_20170101001500-20170101002500.nc",
            ],
            isCGI=False,
            showLogOnError=False,
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetCapabilities_timeseries_path_netcdf_5dims_seq2.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&request=getcapabilities",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.timeseries.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSGetFeatureInfo_forecastreferencetime_texthtml(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetFeatureInfo_forecastreferencetime.html"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=forecast_reference_time%2FHARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=air_temperature__at_2m&QUERY_LAYERS=air_temperature__at_2m&CRS=EPSG%3A4326&BBOX=49.55171074378079,1.4162628389784275,54.80328142582087,9.526486675156528&WIDTH=1515&HEIGHT=981&I=832&J=484&FORMAT=image/gif&INFO_FORMAT=text/html&STYLES=&&time=2017-12-17T09%3A00%3A00Z&DIM_reference_time=2017-12-15T09%3A00%3A00Z",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )
        filename = "test_WMSGetFeatureInfo_forecastreferencetime_texthtml_TestDataGetCapabilities.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&request=getcapabilities", env=self.env
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSGetMap_Report_nounits(self):
        AdagucTestTools().cleanTempDir()
        if os.path.exists(os.environ["ADAGUC_LOGFILE"]):
            os.remove(os.environ["ADAGUC_LOGFILE"])
        filename = "test_WMSGetMap_Report_nounits"
        reportfilename = "./nounits_checker_report.txt"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=test/testdata_report_nounits.nc&service=WMS&request=GetMap&version=1.3.0&layers=sow_a1&crs=EPSG%3A4326&bbox=47.80599631376197%2C1.4162628389784275%2C56.548995855839685%2C9.526486675156528&width=863&height=981&format=image%2Fpng&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_reference_time=2017-12-15T09%3A00%3A00Z",
            env=self.env,
            args=[f"--report={reportfilename}"],
            isCGI=False,
            showLogOnError=False,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 404)
        self.assertTrue(os.path.exists(reportfilename))
        self.assertTrue(os.path.exists(os.environ["ADAGUC_LOGFILE"]))

        reportfile = open(reportfilename, "r", encoding="UTF-8")
        report = json.load(reportfile)
        reportfile.close()
        os.remove(reportfilename)
        self.assertTrue("messages" in report)
        # add more errors to this list if we expect more.
        expectedErrors = ["No time units found for variable time"]
        foundErrors = []
        # self.assertIsNone("TODO: test if error messages end up in normale log file as well as report.")
        for message in report["messages"]:
            self.assertTrue("category" in message)
            self.assertTrue("documentationLink" in message)
            self.assertTrue("message" in message)
            self.assertTrue("severity" in message)
            if message["severity"] == "ERROR":
                foundErrors.append(message["message"])
                self.assertIn(message["message"], expectedErrors)
        self.assertEqual(len(expectedErrors), len(foundErrors))

        expectedErrors.append("WMS GetMap Request failed")
        foundErrors = []
        with open(os.environ["ADAGUC_LOGFILE"], encoding="UTF-8") as logfile:
            for line in logfile.readlines():
                if "E:" in line:
                    for error in expectedErrors:
                        if error in line:
                            foundErrors.append(error)
        logfile.close()
        self.assertEqual(len(expectedErrors), len(foundErrors))

    def test_WMSGetMap_NoReport_nounits(self):
        AdagucTestTools().cleanTempDir()
        if os.path.exists(os.environ["ADAGUC_LOGFILE"]):
            os.remove(os.environ["ADAGUC_LOGFILE"])
        filename = "test_WMSGetMap_Report_nounits"

        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=test/testdata_report_nounits.nc&service=WMS&request=GetMap&version=1.3.0&layers=sow_a1&crs=EPSG%3A4326&bbox=47.80599631376197%2C1.4162628389784275%2C56.548995855839685%2C9.526486675156528&width=863&height=981&format=image%2Fpng&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_reference_time=2017-12-15T09%3A00%3A00Z",
            env=self.env,
            isCGI=False,
            showLogOnError=False,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 404)
        self.assertTrue(os.path.exists(os.environ["ADAGUC_LOGFILE"]))
        expectedErrors = [
            "No time units found for variable time",
            "Exception in DBLoopFiles",
            "Exception in setDimValuesForDataSource for layer sow_a1 with ServiceExceptionCode=1",
            "WMS GetMap Request failed",
        ]
        foundErrors = []
        with open(os.environ["ADAGUC_LOGFILE"], encoding="UTF-8") as logfile:
            for line in logfile.readlines():
                if "E:" in line:
                    for error in expectedErrors:
                        if error in line:
                            foundErrors.append(error)
        logfile.close()
        self.assertEqual(len(expectedErrors), len(foundErrors))

    def test_WMSGetMap_worldmap_latlon_PNGFile_withoutinfofile(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_worldmap_latlon_PNGFile_withoutinfofile.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=worldmap_latlon.png&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=pngdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=rgba%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_worldmap_mercator_PNGFile_withinfofile(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_worldmap_mercator_PNGFile_withinfofile.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=worldmap_mercator.png&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=pngdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=rgba%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetCapabilities_testdatanc_autostyle(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetCapabilities_testdatanc_autostyle.xml"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&request=getcapabilities",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSGetCapabilities_multidimnc_autostyle(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetCapabilities_multidimnc_autostyle.xml"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=netcdf_5dims/netcdf_5dims_seq1/nc_5D_20170101000000-20170101001000.nc&SERVICE=WMS&request=getcapabilities",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSGetCapabilities_multidimncdataset_autostyle(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.testmultidimautostyle.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetCapabilities_multidimncdataset_autostyle.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testmultidimautostyle&SERVICE=WMS&request=getcapabilities",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSGetMapWithShowLegendTrue_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapWithShowLegendTrue_testdatanc.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=geojsonbaselayer,testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata_style_2/shadedcontour&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=true",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMapWithManyContourDefinitions_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapWithManyContourDefinitions_testdatanc.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.tests.manycontours.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata_style_manycontours/contour&FORMAT=image/png&TRANSPARENT=FALSE&",
            {
                "ADAGUC_CONFIG": ADAGUC_PATH
                + "/data/config/adaguc.tests.manycontours.xml"
            },
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                19,
                0.6,
            )
        )

    def test_WMSGetMapWithShowLegendFalse_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapWithShowLegendFalse_testdatanc.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=geojsonbaselayer,testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata_style_2/shadedcontour&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=false",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMapWithShowLegendNothing_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapWithShowLegendNothing_testdatanc.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=geojsonbaselayer,testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata_style_2/shadedcontour&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
            showLog=False,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMapWithShowLegendSecondLayer_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapWithShowLegendSecondLayer_testdatanc.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=geojsonbaselayer,testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata_style_2/shadedcontour&FORMAT=image/png32&TRANSPARENT=FALSE&showlegend=testdata",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename, self.testresultspath + filename
            )
        )

    def test_WMSGetMapWithShowLegendAllLayers_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapWithShowLegendAllLayers_testdatanc.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=geojsonbaselayer,testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata_style_2/shadedcontour&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=geojsonbaselayer,testdata",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMapRobinsonProjection_sample_tas_cmip6_ssp585_preIndustrial_warming2_year(
        self,
    ):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapRobinsonProjection_sample_tas_cmip6_ssp585_preIndustrial_warming2_year.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=test/sample_tas_cmip6_ssp585_preIndustrial_warming2_year.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=tas&WIDTH=600&HEIGHT=300&CRS=EPSG%3A54030&BBOX=-17002000,-8700000,17002000,8700000&STYLES=auto/nearest&FORMAT=image/png32&TRANSPARENT=FALSE",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMapCustomCRSEPSG3412Projection_sample_tas_cmip6_ssp585_preIndustrial_warming2_year(
        self,
    ):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapCustomCRSEPSG3412Projection_sample_tas_cmip6_ssp585_preIndustrial_warming2_year.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=test/sample_tas_cmip6_ssp585_preIndustrial_warming2_year.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=tas,geojsonoverlay&&format=image%2Fpng32&crs=%2Bproj%3Dstere+%2Blat_0%3D-90+%2Blat_ts%3D-70+%2Blon_0%3D0+%2Bk%3D1+%2Bx_0%3D0+%2By_0%3D0+%2Ba%3D6378273+%2Bb%3D6356889.449+%2Bunits%3Dm+%2Bno_defs&width=800&height=600&BBOX=-4630165.372231959,-4523993.082972504,5384973.558397711,4717659.691530302&",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename, self.testresultspath + filename
            )
        )

    def test_WMSGetMapCustomCRSEPSG3413Projection_sample_tas_cmip6_ssp585_preIndustrial_warming2_year(
        self,
    ):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapCustomCRSEPSG3413Projection_sample_tas_cmip6_ssp585_preIndustrial_warming2_year.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=test/sample_tas_cmip6_ssp585_preIndustrial_warming2_year.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=tas,geojsonoverlay&&format=image%2Fpng32&crs=%2Bproj%3Dstere%20%2Blat_0%3D90%20%2Blat_ts%3D70%20%2Blon_0%3D-45%20%2Bk%3D1%20%2Bx_0%3D0%20%2By_0%3D0%20%2Bdatum%3DWGS84%20%2Bunits%3Dm%20%2Bno_defs&width=800&height=600&BBOX=-4630165.372231959,-4523993.082972504,5384973.558397711,4717659.691530302&",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename, self.testresultspath + filename, 8
            )
        )

    def test_WMSGetMapRobinsonProjection_ipcc_cmip5_tas_historical_subset_nc(self):
        AdagucTestTools().cleanTempDir()
        filename = (
            "test_WMSGetMapRobinsonProjection_ipcc_cmip5_tas_historical_subset.nc.png"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=test/ipcc_cmip5_tas_historical_subset.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=tas&WIDTH=600&HEIGHT=300&CRS=EPSG%3A54030&BBOX=-17002000,-8700000,17002000,8700000&STYLES=auto/nearest&FORMAT=image/png32&TRANSPARENT=FALSE",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMapCustomCRSEPSG3412Projection_ipcc_cmip5_tas_historical_subset_nc(
        self,
    ):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapCustomCRSEPSG3412Projection_ipcc_cmip5_tas_historical_subset.nc.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=test/ipcc_cmip5_tas_historical_subset.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=tas,geojsonoverlay&&format=image%2Fpng32&crs=%2Bproj%3Dstere+%2Blat_0%3D-90+%2Blat_ts%3D-70+%2Blon_0%3D0+%2Bk%3D1+%2Bx_0%3D0+%2By_0%3D0+%2Ba%3D6378273+%2Bb%3D6356889.449+%2Bunits%3Dm+%2Bno_defs&width=800&height=600&BBOX=-4630165.372231959,-4523993.082972504,5384973.558397711,4717659.691530302&",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename, self.testresultspath + filename
            )
        )

    def test_WMSGetMapCustomCRSEPSG3413Projection_ipcc_cmip5_tas_historical_subset_nc(
        self,
    ):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapCustomCRSEPSG3413Projection_ipcc_cmip5_tas_historical_subset.nc.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=test/ipcc_cmip5_tas_historical_subset.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=tas,geojsonoverlay&&format=image%2Fpng32&crs=%2Bproj%3Dstere%20%2Blat_0%3D90%20%2Blat_ts%3D70%20%2Blon_0%3D-45%20%2Bk%3D1%20%2Bx_0%3D0%20%2By_0%3D0%20%2Bdatum%3DWGS84%20%2Bunits%3Dm%20%2Bno_defs&width=800&height=600&BBOX=-4630165.372231959,-4523993.082972504,5384973.558397711,4717659.691530302&",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename, self.testresultspath + filename, 8
            )
        )

    # def test_WMSGetMapCustomCRSClippedRobinsonProjection_ipcc_cmip5_tas_historical_subset_nc(self):
    #     AdagucTestTools().cleanTempDir()
    #     filename="test_WMSGetMapCustomCRSClippedRobinsonProjection_ipcc_cmip5_tas_historical_subset_nc.nc.png"

    #     status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config',  ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'], env = self.env, isCGI = False)
    #     self.assertEqual(status, 0)
    #     status,data,headers = AdagucTestTools().runADAGUCServer("source=test/ipcc_cmip5_tas_historical_subset.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=tas&format=image%2Fpng32&crs=%2Bproj%3Drobin+%2Blon_0%3D-150+%2Bx_0%3D0+%2By_0%3D0+%2Bellps%3DWGS84+%2Bdatum%3DWGS84+%2Bunits%3Dm+%2Bno_defs&width=800&height=600&BBOX=-17002000,-8700000,17002000,8700000"
    #                                                              , {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'})
    #     AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
    #     self.assertEqual(status, 0)
    #     self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_WMSGetFeatureInfo_timeseries_KNMIHDF5_json(self):
        AdagucTestTools().cleanTempDir()
        env = {
            "ADAGUC_CONFIG": ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.KNMIHDF5.test.xml"
        }
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.KNMIHDF5.test.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetFeatureInfo_timeseries_KNMIHDF5_json.json"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.KNMIHDF5.test&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=RAD_NL25_PCP_CM&query_layers=RAD_NL25_PCP_CM&crs=EPSG%3A3857&bbox=467411.5837657447%2C5796421.971094566%2C889884.3758374067%2C7834481.671540775&width=199&height=960&i=103&j=501&format=image%2Fgif&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMapWithBilinearRendering(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapWithBilinearRendering_gsie-klimaatatlas2020-ev4-resampled.nc.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=gsie-klimaatatlas2020-ev4-resampled.nc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=interpolatedObs&WIDTH=350&HEIGHT=400&CRS=EPSG%3A3857&BBOX=310273.981651517,6517666.437519898,896694.2006277166,7153301.592131215&STYLES=autobilinear&FORMAT=image/png&TRANSPARENT=TRUE&&time=2020-01-01T00%3A00%3A00Z",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.autostyle.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_KNMIHDF5_echotops_RAD_NL25_ETH_NA_GRID(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.KNMIHDF5.test.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_KNMIHDF5_echotops_RAD_NL25_ETH_NA_GRID.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.KNMIHDF5.test&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=RAD_NL25_ETH_NA_GRID&WIDTH=300&HEIGHT=300&CRS=EPSG%3A3857&BBOX=180000,6300000,1000000,7200000&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&time=2020-04-30T13%3A15%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    # FIXME: these tests all use style=auto, they fail without <RenderMethod>point</RenderMethod>
    def test_WMSGetMap_KNMIHDF5_echotops_RAD_NL25_ETH_NA_TOPS(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.KNMIHDF5.test.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_KNMIHDF5_echotops_RAD_NL25_ETH_NA_TOPS.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.KNMIHDF5.test&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=RAD_NL25_ETH_NA_TOPS&WIDTH=300&HEIGHT=300&CRS=EPSG%3A3857&BBOX=180000,6300000,1000000,7200000&STYLES=auto&FORMAT=image/png&TRANSPARENT=TRUE&time=2020-04-30T13%3A15%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

        filename = "test_WMSGetMap_KNMIHDF5_echotops_RAD_NL25_ETH_NA_TOPS_MAX_TEXT.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.KNMIHDF5.test&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=RAD_NL25_ETH_NA_TOPS_MAX&WIDTH=300&HEIGHT=300&CRS=EPSG%3A3857&BBOX=180000,6300000,1000000,7200000&STYLES=auto&FORMAT=image/png&TRANSPARENT=TRUE&time=2020-04-30T13%3A15%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

        #Tops Outline
        filename = "test_WMSGetMap_KNMIHDF5_echotops_RAD_NL25_ETH_NA_TOPS_MAX_OUTLINE.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.KNMIHDF5.test&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=RAD_NL25_ETH_NA_TOPS_MAX&WIDTH=300&HEIGHT=300&CRS=EPSG%3A3857&BBOX=180000,6300000,1000000,7200000&STYLES=echotopsmax_outline&FORMAT=image/png&TRANSPARENT=TRUE&time=2020-04-30T13%3A15%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetCapabilities_KMDS_PointNetCDF_pointstylepoint(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.testKMDS_PointNetCDF.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetCapabilities_KMDS_PointNetCDF_pointstylepoint.xml"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testKMDS_PointNetCDF.xml&SERVICE=WMS&request=getcapabilities",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSGetMap_KMDS_PointNetCDF_pointstylepoint(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.testKMDS_PointNetCDF.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_KMDS_PointNetCDF_pointstylepoint.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testKMDS_PointNetCDF.xml&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=ta&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=294179.7001580532,6411290.650918596,901204.9572071509,7199735.637765654&STYLES=temperature&FORMAT=image/png&TRANSPARENT=TRUE",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

        filename = "test_WMSGetMap_KMDS_PointNetCDF_ffdd_windspeed_arrow_barb.png"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testKMDS_PointNetCDF.xml&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=ff_dd&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=294179.7001580532,6411290.650918596,901204.9572071509,7199735.637765654&STYLES=windspeed_arrow%2Fbarb&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

        filename = "test_WMSGetMap_KMDS_PointNetCDF_ffdd_windspeed_barb_barb.png"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testKMDS_PointNetCDF.xml&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=ff_dd&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=294179.7001580532,6411290.650918596,901204.9572071509,7199735.637765654&STYLES=windspeed_barb%2Fbarb&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

        # Test no outline
        filename = (
            "test_WMSGetMap_KMDS_PointNetCDF_ffdd_windspeed_barb_barb_no_outline.png"
        )

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testKMDS_PointNetCDF.xml&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=ff_dd&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=294179.7001580532,6411290.650918596,901204.9572071509,7199735.637765654&STYLES=windspeed_barb_no_outline%2Fbarb&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

        filename = "test_WMSGetMap_KMDS_PointNetCDF_ta_volume.png"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testKMDS_PointNetCDF.xml&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=ta&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=294179.7001580532,6411290.650918596,901204.9572071509,7199735.637765654&STYLES=temperature-volume&FORMAT=image/png32&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

        filename = "test_WMSGetMap_KMDS_PointNetCDF_ta_disc.png"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testKMDS_PointNetCDF.xml&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=ta&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=294179.7001580532,6411290.650918596,901204.9572071509,7199735.637765654&STYLES=temperature-disc&FORMAT=image/png32&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetCapabilities_KMDS_PointNetCDF_dimension_filetimedate(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.testKMDS_PointNetCDF_filetimedate.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testKMDS_PointNetCDF_filetimedate.xml&SERVICE=WMS&request=getcapabilities",
            env=env,
        )
        obj1 = objectify.fromstring(
            re.sub(b' xmlns="[^"]+"', b"", data.getvalue(), count=1)
        )
        foundTimeFromXML = obj1.findall("Capability/Layer/Layer/Dimension")[0]

        fileToCheck = f"{ADAGUC_PATH}/data/datasets/test/netcdfpointtimeseries/Actuele10mindataKNMIstations_20201220123000.nc"
        foundTimeFromFile = datetime.datetime.utcfromtimestamp(
            os.path.getmtime(fileToCheck)
        ).strftime("%Y-%m-%dT%H:%M:%SZ")

        self.assertEqual(foundTimeFromXML, foundTimeFromFile)

        filename = "test_WMSGetCapabilities_KMDS_PointNetCDF_dimension_filetimedate.png"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testKMDS_PointNetCDF_filetimedate&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=stationname&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=294179.7001580532,6411290.650918596,901204.9572071509,7199735.637765654&STYLES=&FORMAT=image/png32&TRANSPARENT=FALSE&BGCOLOR=0xFFFFA0",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetLegendGraphic_adaguc_scaling_dataset_scaling1x(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.scaling.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetLegendGraphic_adaguc_scaling_dataset_scaling1x.png"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.scaling&SERVICE=WMS&&version=1.1.1&service=WMS&request=GetLegendGraphic&layer=testdata&format=image/png&STYLE=testdata/nearest&layers=testdata&&&transparent=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMapWithGetLegendGraphic_adaguc_scaling_dataset_scaling1x(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.scaling.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = (
            "test_WMSGetMapWithGetLegendGraphic_adaguc_scaling_dataset_scaling1x.png"
        )

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.scaling&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=1024&HEIGHT=1024&CRS=EPSG%3A3857&BBOX=-3099408.36963744,3701316.1668297593,3704230.74564144,10633468.46539724&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&showlegend=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMapWithGetLegendGraphic_adaguc_scaling_dataset_scaling4x(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.scaling.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = (
            "test_WMSGetMapWithGetLegendGraphic_adaguc_scaling_dataset_scaling4x.png"
        )

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.scaling&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=1024&HEIGHT=1024&CRS=EPSG%3A3857&BBOX=-3099408.36963744,3701316.1668297593,3704230.74564144,10633468.46539724&STYLES=testdatascaling4x%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&showlegend=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetLegendGraphic_inverted_min_max(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ["ADAGUC_PATH"]
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.invertedlegend.xml"
        )
        env = {"ADAGUC_CONFIG": config}

        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetLegendGraphic_inverted_min_max.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.invertedlegend&SERVICE=WMS&&version=1.1.1&service=WMS&request=GetLegendGraphic&layer=testdata&format=image/png&STYLE=testdatainverted/nearest&layers=testdata&&&transparent=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetLegendGraphic_log10(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ["ADAGUC_PATH"]
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.invertedlegend.xml"
        )
        env = {"ADAGUC_CONFIG": config}

        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetLegendGraphic_log10.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.invertedlegend&SERVICE=WMS&&version=1.1.1&service=WMS&request=GetLegendGraphic&layer=testdata&format=image/png&STYLE=testdatalog10/nearest&layers=testdata&&&transparent=false",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMapQuantizeLow(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ["ADAGUC_PATH"]
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.quantizelow.xml"
        )
        env = {"ADAGUC_CONFIG": config}

        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "testWMSGetCapabilities_quantizelow.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.quantizelow&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )
        self.assertEqual(status, 0)

    def test_WMSGetMapQuantizeHigh(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ["ADAGUC_PATH"]
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.quantizehigh.xml"
        )
        env = {"ADAGUC_CONFIG": config}

        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "testWMSGetCapabilities_quantizehigh.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.quantizehigh&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )
        self.assertEqual(status, 0)

    def test_WMSGetMapQuantizeRound(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ["ADAGUC_PATH"]
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.quantizeround.xml"
        )
        env = {"ADAGUC_CONFIG": config}

        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "testWMSGetCapabilities_quantizeround.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.quantizelow&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )
        self.assertEqual(status, 0)

    def test_WMSGetMap_ODIMHDF5_RAD_CU21_PPZ_E05(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.ODIMHDF5.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_ODIMHDF5_RAD_CU21_PPZ_E05.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.ODIMHDF5&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=CAPPI&WIDTH=746&HEIGHT=927&CRS=EPSG%3A3857&BBOX=-8153802.59473168,773907.3916620318,-7198456.3109903205,1961046.808589968&STYLES=radar%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&time=2022-08-30T22%3A35%3A00Z",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_DBScannerCleanFiles(self):
        """Testing cleanup system of filescanner using the retentionperiod function."""
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ["ADAGUC_PATH"]
        ADAGUC_TMP = os.environ["ADAGUC_TMP"]

        config = ADAGUC_PATH + "/data/config/adaguc.tests.datasetsmall.xml"
        env = {"ADAGUC_CONFIG": config}

        # Setup directories
        AdagucTestTools().cleanTempDir()
        AdagucTestTools().mkdir_p(f"{ADAGUC_TMP}/cleandb")

        # Make a time recent to now (less then 7 days ago)
        def roundSeconds(dateTimeObject):
            newDateTime = dateTimeObject
            newDateTime = newDateTime.replace(second=0)
            return newDateTime.replace(microsecond=0)

        recenttimesteptowrite = (
            roundSeconds(datetime.datetime.utcnow()).isoformat() + "Z"
        )

        # Make the three filenames
        oldfile1 = f"{ADAGUC_TMP}/cleandb/csv-20200601T000000.csv"
        oldfile2 = f"{ADAGUC_TMP}/cleandb/csv-20200602T000000.csv"
        newfile1 = f"{ADAGUC_TMP}/cleandb/csv-{recenttimesteptowrite}.csv"

        # Make sure the files are not there.
        self.assertEqual(os.path.exists(oldfile1), False)
        self.assertEqual(os.path.exists(oldfile2), False)
        self.assertEqual(os.path.exists(newfile1), False)

        # print(f'Writing to {newfile1}')

        # Write files to disk ready to scan without cleanup
        with open(oldfile1, "w") as f:
            f.write(
                '# time=2020-06-01T00:00:00Z\nlat,lon,Name,test\n48.1,0.25,"Station",60'
            )
        with open(oldfile2, "w") as f:
            f.write(
                '# time=2020-06-02T00:00:00Z\nlat,lon,Name,test\n48.1,0.25,"Station",60'
            )
        with open(newfile1, "w") as f:
            f.write(
                f'# time={recenttimesteptowrite}\nlat,lon,Name,test\n48.1,0.25,"Station",60'
            )

        ### Step 1 ###

        # Scan with adaguc.tests.cleandb-step1, cleanup DISABLED. All three timesteps should be found.
        DATASET = "adaguc.tests.cleandb-step1"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                config + f",{ADAGUC_PATH}/data/config/datasets/{DATASET}.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

        # Make sure the files are still there.
        self.assertEqual(os.path.exists(oldfile1), True)
        self.assertEqual(os.path.exists(oldfile2), True)
        self.assertEqual(os.path.exists(newfile1), True)

        # Read getcapabilities and check the dimensions for the three expected values
        status, data, headers = AdagucTestTools().runADAGUCServer(
            f"DATASET={DATASET}&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities",
            env=env,
        )
        xslt_root = etree.XML(
            re.sub(
                ' xmlns="[^"]+"', "", data.getvalue().decode("UTF-8"), count=1
            ).encode("ascii")
        )
        dimvalues = xslt_root.findall("Capability/Layer/Layer/Dimension")[0].text
        expecteddimensionvalues = (
            f"2020-06-01T00:00:00Z,2020-06-02T00:00:00Z,{recenttimesteptowrite}"
        )
        self.assertEqual(expecteddimensionvalues, dimvalues)

        ### Step 2 ###

        # Scan with adaguc.tests.cleandb-step1, cleanup ENABLED, only the recent timestep should be left
        DATASET = "adaguc.tests.cleandb-step2"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                config + f",{ADAGUC_PATH}/data/config/datasets/{DATASET}.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

        # Make sure the files are gone.
        self.assertEqual(os.path.exists(oldfile1), False)
        self.assertEqual(os.path.exists(oldfile2), False)
        self.assertEqual(os.path.exists(newfile1), True)

        # Read getcapabilities and check the dimensions
        status, data, headers = AdagucTestTools().runADAGUCServer(
            f"DATASET={DATASET}&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities",
            env=env,
        )
        xslt_root = etree.XML(
            re.sub(
                ' xmlns="[^"]+"', "", data.getvalue().decode("UTF-8"), count=1
            ).encode("ascii")
        )
        dimvalues = xslt_root.findall("Capability/Layer/Layer/Dimension")[0].text
        self.assertEqual(recenttimesteptowrite, dimvalues)

    def test_WMSGetMap_PNGFile_colortype2_bitdepth8(self):
        AdagucTestTools().cleanTempDir()
        # convert png-100x100.png  -define png:color-type=2 -define png:bit-depth=8  png-100x100-ct2_bd8.png
        filename = "test_WMSGetMap_PNGFile_colortype2_bitdepth8.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=png-100x100-ct2_bd8.png&&service=WMS&request=getmap&format=image/png32&layers=pngdata&width=100&height=100&CRS=EPSG:4326&STYLES=&EXCEPTIONS=INIMAGE&bbox=-90,-180,90,180",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_PNGFile_colortype2_bitdepth16(self):
        AdagucTestTools().cleanTempDir()
        # convert png-100x100.png  -define png:color-type=2 -define png:bit-depth=16  png-100x100-ct2_bd16.png
        filename = "test_WMSGetMap_PNGFile_colortype2_bitdepth16.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=png-100x100-ct2_bd16.png&&service=WMS&request=getmap&format=image/png32&layers=pngdata&width=100&height=100&CRS=EPSG:4326&STYLES=&EXCEPTIONS=INIMAGE&bbox=-90,-180,90,180",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_PNGFile_colortype3_bitdepth1(self):
        AdagucTestTools().cleanTempDir()
        # convert png-100x100-boolean.png  -type palette  -colors 2  -define png:color-type=3 -define png:bit-depth=1 png-100x100-ct3_bd1.png
        filename = "test_WMSGetMap_PNGFile_colortype3_bitdepth1.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=png-100x100-ct3_bd1.png&&service=WMS&request=getmap&format=image/png32&layers=pngdata&width=100&height=100&CRS=EPSG:4326&STYLES=&EXCEPTIONS=INIMAGE&bbox=-90,-180,90,180",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_PNGFile_colortype3_bitdepth4(self):
        AdagucTestTools().cleanTempDir()
        # convert png-100x100.png  -type palette  -colors 16 -define png:color-type=3 -define png:bit-depth=4 png-100x100-ct3_bd4.png
        filename = "test_WMSGetMap_PNGFile_colortype3_bitdepth4.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=png-100x100-ct3_bd4.png&&service=WMS&request=getmap&format=image/png32&layers=pngdata&width=100&height=100&CRS=EPSG:4326&STYLES=&EXCEPTIONS=INIMAGE&bbox=-90,-180,90,180",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_PNGFile_colortype3_bitdepth8(self):
        AdagucTestTools().cleanTempDir()
        # convert png-100x100.png  -type palette -colors 256 -define png:color-type=3 -define png:bit-depth=8  png-100x100-ct3_bd8.png
        filename = "test_WMSGetMap_PNGFile_colortype3_bitdepth8.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=png-100x100-ct3_bd8.png&&service=WMS&request=getmap&format=image/png32&layers=pngdata&width=100&height=100&CRS=EPSG:4326&STYLES=&EXCEPTIONS=INIMAGE&bbox=-90,-180,90,180",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_PNGFile_colortyp6_bitdepth8(self):
        AdagucTestTools().cleanTempDir()
        # convert png-100x100.png  -define png:color-type=6 -define png:bit-depth=8  png-100x100-ct6_bd8.png
        filename = "test_WMSGetMap_PNGFile_colortype6_bitdepth8.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=png-100x100-ct6_bd8.png&&service=WMS&request=getmap&format=image/png32&layers=pngdata&width=100&height=100&CRS=EPSG:4326&STYLES=&EXCEPTIONS=INIMAGE&bbox=-90,-180,90,180",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_PNGFile_colortyp6_bitdepth16(self):
        AdagucTestTools().cleanTempDir()
        # convert png-100x100.png  -define png:color-type=6 -define png:bit-depth=16  png-100x100-ct6_bd16.png
        filename = "test_WMSGetMap_PNGFile_colortype6_bitdepth16.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=png-100x100-ct6_bd16.png&&service=WMS&request=getmap&format=image/png32&layers=pngdata&width=100&height=100&CRS=EPSG:4326&STYLES=&EXCEPTIONS=INIMAGE&bbox=-90,-180,90,180",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_NearestRenderWithShadeIntervalFast(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.nearestshadeinterval.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_NearestRenderWithShadeIntervalFast.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=nearestshadeinterval&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=shadedstylefast%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_NearestRenderWithShadeIntervalPrecise(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.nearestshadeinterval.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_NearestRenderWithShadeIntervalPrecise.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=nearestshadeinterval&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=shadedstyleprecise%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetLegendGraphic_NearestRenderWithShadeInterval(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.nearestshadeinterval.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetLegendGraphic_NearestRenderWithShadeInterval.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=nearestshadeinterval&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetLegendGraphic&LAYER=testdata&WIDTH=150&HEIGHT=800&STYLES=shadedstyleprecise%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_PNG_with_time(self):
        ### This tests a PNG file with a time dim set ###
        AdagucTestTools().cleanTempDir()

        # Read getcapabilities and check the dimensions
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            f"SOURCE=pngfile_with_time_dim.png&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities",
            env=self.env,
        )
        xslt_root = etree.XML(
            re.sub(
                ' xmlns="[^"]+"', "", data.getvalue().decode("UTF-8"), count=1
            ).encode("ascii")
        )
        dimvalues = xslt_root.findall("Capability/Layer/Layer/Dimension")[0].text
        self.assertEqual("2023-10-27T06:00:00Z", dimvalues)

    def test_WMSGetMap_PNG_with_time_and_reftime(self):
        ### This tests a PNG file with time and reference_time set ###
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_PNG_with_time_and_reftime.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            f"SOURCE=pngfile_with_time_and_reftime_dim.png&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities",
            env=self.env,
        )
        xslt_root = etree.XML(
            re.sub(
                ' xmlns="[^"]+"', "", data.getvalue().decode("UTF-8"), count=1
            ).encode("ascii")
        )
        dimvalues = xslt_root.findall("Capability/Layer/Layer/Dimension")[0].text
        reftime_dimvalues = xslt_root.findall("Capability/Layer/Layer/Dimension")[
            1
        ].text

        self.assertEqual("2023-10-28T00:00:00Z", dimvalues)
        self.assertEqual("2023-10-27T12:00:00Z", reftime_dimvalues)

    def test_WMSGetMap_GOES16_bes_geos_500m_airmass(self):
        ### This tests the geos projection ###
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_GOES16_bes_geos_500m_airmass.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=GOES16_bes-geos-500m_airmass_202305160100-100x100.png&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=pngdata&WIDTH=100&HEIGHT=100&CRS=EPSG%3A3857&BBOX=-8130499.94291223,258165.08437911526,-4701559.135087881,3327755.582274761&STYLES=auto%2Frgba&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())

        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_rotated_pole_RACMO(self):
        ### This tests the rotated_pole projection ###
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_rotated_pole_RACMO.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.autoresource.xml",
            ],
            isCGI=False,
            showLogOnError=False,
        )
        self.assertEqual(status, 0)

        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=test_rotated_pole_RACMO.nc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=pr_adjust,overlay&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=333149.64940493606,6414453.750108542,881082.573419756,7153112.588677192&STYLES=auto%2Frgba&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())

        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_dashed_contour_lines(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.dashedcontourlines.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)
        filename = "test_WMSGetMap_dashed_contour_lines.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.dashedcontourlines&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,dashed_contour_lines,overlay&WIDTH=773&HEIGHT=927&CRS=EPSG%3A3857&BBOX=-1572926.437674431,4261090.143221738,2101038.6845761645,8666996.570552012&STYLES=testdata_style_manycontours%2Fcontour&FORMAT=image/png32&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                4,
                0.02,
            )
        )  # Allowed pixel difference is huge, but only for very small number of pixels

    def test_WMSGetMap_dashed_contour_lines_arial(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.dashedcontourlines-arial.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)
        filename = "test_WMSGetMap_dashed_contour_lines_arial.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.dashedcontourlines&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,dashed_contour_lines,overlay&WIDTH=773&HEIGHT=927&CRS=EPSG%3A3857&BBOX=-1572926.437674431,4261090.143221738,2101038.6845761645,8666996.570552012&STYLES=testdata_style_manycontours%2Fcontour&FORMAT=image/png32&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                4,
                0.02,
            )
        )  # Allowed pixel difference is huge, but only for very small number of pixels

    def test_WMSGetMap_allow_encoded_proj4_params_attribute(self):
        ### This tests the rotated_pole projection ###
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_allow_encoded_proj4_params_attribute.png"

        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=issue-279/279-allow_encoded_proj4_params_attribute.nc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=prediction&WIDTH=150&HEIGHT=175&CRS=EPSG%3A3857&BBOX=333250.30526052,6493136.144375306,849393.56753148,7176591.901449695&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&time=2023-08-18T08%3A00%3A00Z",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())

        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSCMDUpdateDBPathFileInSubfolders(self):
        """
        This tests if the autofinddataset option correctly finds the file if it is in a subfolder
        """
        AdagucTestTools().cleanTempDir()
        # pylint: disable=unused-variable

        config = ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"
        args = [
            "--updatedb",
            "--autofinddataset",
            "--verboseoff",
            "--config",
            config,
            "--path",
            ADAGUC_PATH + "/data/datasets/test/RAD_NL25_PCP_CM_202106222000.h5",
        ]
        print(args)
        env = {"ADAGUC_CONFIG": config}
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=args, env=env, isCGI=False, showLogOnError=True, showLog=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSCMDUpdateDBPathFileInSubfoldersGetCapabilities.xml"
        env = {"ADAGUC_CONFIG": config}
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testScanPathSubfolder&SERVICE=WMS&request=getcapabilities",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSCMDUpdateDBPathFileWithNonMatchingPath(self):
        """
        This tests if the autofinddataset option correctly finds the file if it is in a subfolder
        """
        AdagucTestTools().cleanTempDir()
        AdagucTestTools().cleanPostgres()
        # pylint: disable=unused-variable

        config = ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"
        args = [
            "--updatedb",
            "--autofinddataset",
            "--verboseoff",
            "--config",
            config,
            "--path",
            ADAGUC_PATH
            + "data/datasets/test_suffix/RAD_NL25_PCP_CM_202106222000_suffix.h5",
        ]
        print(args)
        env = {"ADAGUC_CONFIG": config}
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=args, env=env, isCGI=False, showLogOnError=True, showLog=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSCMDUpdateDBPathFileWithNonMatchingPathGetCapabilities.xml"
        env = {"ADAGUC_CONFIG": config}
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testScanPathSubfolderSuffix&SERVICE=WMS&request=getcapabilities",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSGetMapWithHarmWindBarbs(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapWithHarmWindBarbs_without_outline.png"
        config = ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                config + ",adaguc.tests.harm_windbarbs.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.harm_windbarbs&SERVICE=WMS&SERVICE=WMS&=&=&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind__at_10m&WIDTH=914&HEIGHT=966&CRS=EPSG:3857&BBOX=10144.960912989336,6256275.017522922,1229386.3384520854,7544882.425294002&STYLES=Windbarbs/barb&FORMAT=image/png&TRANSPARENT=FALSE&time=2023-09-30T06:00:00Z&DIM_reference_time=2023-09-28T06:00:00Z&BGCOLOR=0x000000&",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                37,
                0.1,
            )
        )

    def test_WMSGetMapWithHarmWindBarbsWithText(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapWithHarmWindBarbs_without_outline_with_text.png"
        config = ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                config + ",adaguc.tests.harm_windbarbs.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.harm_windbarbs&SERVICE=WMS&SERVICE=WMS&=&=&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind__at_10m&WIDTH=914&HEIGHT=966&CRS=EPSG:3857&BBOX=10144.960912989336,6256275.017522922,1229386.3384520854,7544882.425294002&STYLES=Windbarbwithnumbers/barb&FORMAT=image/png&TRANSPARENT=FALSE&time=2023-09-30T06:00:00Z&DIM_reference_time=2023-09-28T06:00:00Z&BGCOLOR=0x000000&",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                37,
                0.1,
            )
        )

    def test_WMSGetMapWithHarmShadedWindSpeed(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapWithHarmShadedWindSpeed.png"
        config = ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                config + ",adaguc.tests.harm_windbarbs.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.harm_windbarbs&SERVICE=WMS&SERVICE=WMS&=&=&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind__at_10m&WIDTH=914&HEIGHT=966&CRS=EPSG:3857&BBOX=10144.960912989336,6256275.017522922,1229386.3384520854,7544882.425294002&STYLES=windbarbs_kts/barbshaded&FORMAT=image/png&TRANSPARENT=FALSE&time=2023-09-30T06:00:00Z&DIM_reference_time=2023-09-28T06:00:00Z&BGCOLOR=0x000000&",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                37,
                0.1,
            )
        )

    def test_WMSGetMapWindComponentsAllNorthEuropeStereoGraphic(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapWindComponentsAllNorth.png"
        config = ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                config + ",adaguc_tests_xycomponents_all_north.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc_tests_xycomponents_all_north&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind-hagl&WIDTH=512&HEIGHT=512&CRS=EPSG%3A3575&BBOX=-4094182.3312461236,-6387609.657566892,3335917.464195122,23694.56643311167&STYLES=windbarbs_kts%2Fbarbshaded&FORMAT=image/png&TRANSPARENT=TRUE&&time=2017-10-15T07%3A31%3A52Z",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                37,
                0.1,
            )
        )

    def test_WMSGetMapWindComponentsAllNorthMercator(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMapWindComponentsAllNorthMercator.png"
        config = ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                config + ",adaguc_tests_xycomponents_all_north.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc_tests_xycomponents_all_north&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind-hagl&WIDTH=512&HEIGHT=512&CRS=EPSG%3A3857&BBOX=-9704794.415410386,1643015.5210643038,11614624.422801377,20039172.20990392&STYLES=windbarbs_kts%2Fbarbshaded&FORMAT=image/png&TRANSPARENT=TRUE&&time=2017-10-15T07%3A31%3A52Z",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                37,
                0.1,
            )
        )

    def test_WMSGetMap_EPSG3067(self):
        AdagucTestTools().cleanTempDir()

        filename = "test_WMSGetMap_EPSG3067.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=910&HEIGHT=616&CRS=EPSG%3A3067&BBOX=-2516792.8416,5070666.2762,1222957.3650,8248146.2901&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_Issue311_WMSGetCapabilities_DimensionUnits(self):
        """
        Tests if the right units are propagated from the layer configuration to the GetCapabilities file
        See https://github.com/KNMI/adaguc-server/issues/311
        """
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.311-getcapabilities-dimension-units.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetCapabilities_DimensionUnits.xml"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.311-getcapabilities-dimension-units&service=WMS&request=GetCapabilities",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSGetMap_WithCaching(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.cacheheader.xml"
        )

        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&request=getcapabilities", {"ADAGUC_CONFIG": config}
        )
        self.assertEqual(headers, ["Content-Type:text/xml", "Cache-Control:max-age=60"])

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=60"]
        )

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=2017-01-01T00:05:00",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=60"]
        )

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=2017-01-01T00:05:00&DIM_member=member3",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=60"]
        )

        # If some dims are specfied with current, we expect a shorter caching interval.
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=current&DIM_member=member3&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=60"]
        )

        # If some dims are specfied incorrectly, we expect a shorter caching interval.
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=2017-01-01T00:05:00.000Z&DIM_member=member3&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=60"]
        )

        # If all dims are specfied, we expect a longer caching interval.
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=2017-01-01T00:05:00Z&DIM_member=member3&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=7200"]
        )

        # If all dims are specfied, but time is without zulue, we expect a shorter caching interval.
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=2017-01-01T00:05:00&DIM_member=member3&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=60"]
        )

        # If all dims are specfied, but time contains millieseconds, we expect a shorter caching interval.
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=2017-01-01T00:05:00.000Z&DIM_member=member3&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=60"]
        )

        # If all dims are specfied, but time is current, we expect a shorter caching interval.
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=current&DIM_member=member3&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=60"]
        )

        # If all dims are specfied except time, we expect a shorter caching interval.
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&DIM_member=member3&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=60"]
        )

        # If all dims are specfied except member, we expect a shorter caching interval.
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=2017-01-01T00:05:00Z&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=60"]
        )

        # If all dims are specfied except height, we expect a shorter caching interval.
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=2017-01-01T00:05:00Z&DIM_member=member3&",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=60"]
        )

        #### From here, dimension 'member' is fixed to 'member4'
        # If we query member with same value as what it is fixed to, we get long cache
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data_with_fixed&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=2017-01-01T00:05:00Z&DIM_member=member4&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=7200"]
        )

        # If we query member with different value as what it is fixed to, we get long cache. Fixed values wins from queried value.
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data_with_fixed&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=2017-01-01T00:05:00Z&DIM_member=member2&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=7200"]
        )

        # If we don't provide query at all for member, we should get long cache
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data_with_fixed&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=2017-01-01T00:05:00Z&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=7200"]
        )

        # Unfixed, unprovided dimension should still result in short cache
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data_with_fixed&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&time=2017-01-01T00:05:00Z",
            {"ADAGUC_CONFIG": config},
        )
        self.assertEqual(
            headers, ["Content-Type:image/png", "Cache-Control:max-age=60"]
        )

    def test_WMSGetMap_IrregularGrid_1Dimensional_latlon(self):
        """
        This will test  a file where lon and lat variables have irregular spacing. These variables are still 1 dimensional.
        """
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"

        filename = "test_WMSGetMap_IrregularGrid_1Dimensional_latlon.png"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=example_file_irregular_1Dlat1Dlon_grid.nc&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data_var_1&WIDTH=512&HEIGHT=320&CRS=EPSG:3857&BBOX=-1612507.3080954754,5830283.642777597,1616299.7724732205,7796173.721992844&STYLES=auto/nearest&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0x0000FF&DIM_extra=0&time=2017-01-01T00:10:00Z&colorscalerange=0,2&",
            {"ADAGUC_CONFIG": config},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_IrregularGrid_1Dimensional_latlon_nextdimensionstep(self):
        """
        This will test  a file where lon and lat variables have irregular spacing. These variables are still 1 dimensional. Dimensions have been changed to allow for generating another new image.
        """
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"

        filename_getcapabilities = "test_WMSGetMap_IrregularGrid_1Dimensional_latlon_nextdimensionstep_getcapabilities.xml"

        filename = (
            "test_WMSGetMap_IrregularGrid_1Dimensional_latlon_nextdimensionstep.png"
        )

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=example_file_irregular_1Dlat1Dlon_grid.nc&SERVICE=WMS&request=GetCapabilities",
            {"ADAGUC_CONFIG": config},
        )
        AdagucTestTools().writetofile(
            self.testresultspath + filename_getcapabilities, data.getvalue()
        )
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename_getcapabilities,
                self.expectedoutputsspath + filename_getcapabilities,
            )
        )

        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=example_file_irregular_1Dlat1Dlon_grid.nc&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data_var_1&WIDTH=512&HEIGHT=320&CRS=EPSG%3A3857&BBOX=-1612507.3080954754,5830283.642777597,1616299.7724732205,7796173.721992844&STYLES=auto/nearest&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xFF0000&DIM_extra=0.8&time=2017-01-01T00:11:00Z&colorscalerange=0,2&",
            {"ADAGUC_CONFIG": config},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_IrregularGrid_2Dimensional_latlon_nextdimensionstep(self):
        """
        This will test  a file where lon and lat variables have irregular spacing. These lat/lon variables are 2D. Dimensions have been changed to allow for generating another new image.
        """
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"

        filename_getcapabilities = "test_WMSGetMap_IrregularGrid_2Dimensional_latlon_nextdimensionstep_getcapabilities.xml"

        filename = (
            "test_WMSGetMap_IrregularGrid_2Dimensional_latlon_nextdimensionstep.png"
        )

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=example_file_irregular_2Dlat2Dlon_grid.nc&SERVICE=WMS&request=GetCapabilities",
            {"ADAGUC_CONFIG": config},
        )
        AdagucTestTools().writetofile(
            self.testresultspath + filename_getcapabilities, data.getvalue()
        )
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename_getcapabilities,
                self.expectedoutputsspath + filename_getcapabilities,
            )
        )

        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=example_file_irregular_2Dlat2Dlon_grid.nc&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data_var_1&WIDTH=512&HEIGHT=320&CRS=EPSG%3A3857&BBOX=-162669.9922884648,6411680.00083944,1827705.159150465,10780913.16460056&STYLES=auto/nearest&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xFF0000&DIM_extra=0.8&time=2017-01-01T00:11:00Z&colorscalerange=0,2&",
            {"ADAGUC_CONFIG": config},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_IrregularGrid_issue_316_double_precision_type(self):
        """
        This will test  a file where lon and lat variables have irregular spacing. These lat/lon variables are 2D. Dimensions have been changed to allow for generating another new image.
        """
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"

        filename = "test_WMSGetMap_IrregularGrid_issue_316_double_precision_type.png"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=example_file_irregular_2Dlat2Dlon_grid_doubleprecision.nc&SERVICE=WMS&request=GetCapabilities",
            {"ADAGUC_CONFIG": config},
        )

        self.assertEqual(status, 0)
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=example_file_irregular_2Dlat2Dlon_grid_doubleprecision.nc&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data_var_1&WIDTH=512&HEIGHT=320&CRS=EPSG%3A3857&BBOX=-162669.9922884648,6411680.00083944,1827705.159150465,10780913.16460056&STYLES=auto/nearest&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xFF0000&DIM_extra=0.8&time=2017-01-01T00:11:00Z&colorscalerange=0,2&",
            {"ADAGUC_CONFIG": config},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_error_on_wrong_dataset(self):
        AdagucTestTools().cleanTempDir()
        status, data, headers = AdagucTestTools().runADAGUCServer(
            url="DATASET=nonexisting&&SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&LAYERS=countryborders&WIDTH=910&HEIGHT=562&SRS=EPSG%3A3857&BBOX=-30765124.555160142,-19000000,30765124.555160142,19000000&STYLES=&FORMAT=image/png&TRANSPARENT=TRUE&",
            showLogOnError=False,
            env={"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.dataset.xml"},
        )
        # Should return 404 Not Found
        self.assertEqual(status, 404)

    def test_WMSGetMap_error_on_existing_dataset_wrong_layer(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.nearestshadeinterval.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.nearestshadeinterval&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=shadedstylefast%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=env,
        )
        # Layer exists, should return 0
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.nearestshadeinterval&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdatanonexisting&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=shadedstylefast%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            showLogOnError=False,
            env=env,
        )
        # Layer does not exist, should return 404
        self.assertEqual(status, 404)

    def test_WMSGetCapabilities_no_error_on_existing_dataset_misconfigured_layer(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.datasetwithmisconfiguredlayer.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            showLogOnError=False,
            args=["--updatedb", "--config", config],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 1)

        filename = "test_WMSGetCapabilities_no_error_on_existing_dataset_misconfigured_layer.xml"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.datasetwithmisconfiguredlayer&SERVICE=WMS&request=GetCapabilities",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        # should return 0
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_WMSGetMap_SolarTerminatorEquinox(self):
        # Testing the solar terminator on March 21, 2023
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.solarterminator.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_SolarTerminatorEquinox.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=solarterminator&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=solarterminator&WIDTH=200&HEIGHT=200&CRS=EPSG%3A3857&BBOX=-27591378.677139122,-15819675.465716192,24482445.32432534,23920874.430138264&STYLES=solarterminator%2Fshadedcontour&FORMAT=image/png&TRANSPARENT=TRUE&&time=2023-03-21T00%3A00%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_SolarTerminatorSolstice(self):
        # Testing the solar terminator on December 21, 2022
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.solarterminator.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_SolarTerminatorSolstice.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=solarterminator&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=solarterminator&WIDTH=200&HEIGHT=200&CRS=EPSG%3A3857&BBOX=-27591378.677139122,-15819675.465716192,24482445.32432534,23920874.430138264&STYLES=solarterminator%2Fshadedcontour&FORMAT=image/png&TRANSPARENT=TRUE&&time=2022-12-21T00%3A00%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_SolarTerminatorQuarterPoint(self):
        # Testing the solar terminator on August 7, 2000
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.solarterminator.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_SolarTerminatorQuarterPoint.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=solarterminator&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=solarterminator&WIDTH=200&HEIGHT=200&CRS=EPSG%3A3857&BBOX=-27591378.677139122,-15819675.465716192,24482445.32432534,23920874.430138264&STYLES=solarterminator%2Fshadedcontour&FORMAT=image/png&TRANSPARENT=TRUE&&time=2000-08-07T00%3A00%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetLegendGraphic_SolarTerminator(self):
        # Testing the solar terminator legend
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.solarterminator.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetLegendGraphic_SolarTerminator.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=solarterminator&SERVICE=WMS&&version=1.3.0&service=WMS&request=GetLegendGraphic&layer=solarterminator&format=image/png&STYLE=auto/nearest&layers=solarterminator&&&transparent=true&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetFeatureInfo_SolarTerminator(self):
        # Testing the solar terminator feature info
        AdagucTestTools().cleanTempDir()
        config = (ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," +
                  ADAGUC_PATH +
                  "/data/config/datasets/adaguc.tests.solarterminator.xml")
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False)

        filename = "test_WMSGetFeatureInfo_SolarTerminator.html"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=solt&&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=solarterminator&QUERY_LAYERS=solarterminator&CRS=EPSG%3A3857&BBOX=-48040284.36018957,-50974535.88541803,-10040284.360189572,62500602.23612894&WIDTH=362&HEIGHT=1081&I=354&J=481&FORMAT=image/gif&INFO_FORMAT=text/html&STYLES=&&time=2026-01-05T16%3A40%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename,
                                      data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath +
                                           filename),
        )

    def test_WMSGetCapabilities_403_default_time_referenced_to_forecast_time(self):
        """
        See https://github.com/KNMI/adaguc-server/issues/403
        """
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.403_default_time_referenced_to_forecast_time.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = (
            "test_WMSGetCapabilities_403_default_time_referenced_to_forecast_time.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.403_default_time_referenced_to_forecast_time&service=WMS&request=GetCapabilities",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(
            self.testresultspath + filename,
            self.expectedoutputsspath + filename))

    def test_WMSGetLegendGraphic_LongTempLegend(self):
        """Skip some labels when legend is too long"""
        """Corresponds to the special case for definedLegendOnShadeClasses case in CCreateLegendRenderDiscreteLegend """
        AdagucTestTools().cleanTempDir()
        config = (ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," +
                  ADAGUC_PATH +
                  "/data/config/datasets/test.uwcw_ha43_dini_5p5km_10x8.xml")
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename = "test_WMSGetLegendGraphic_LongTempLegend.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=test.uwcw_ha43_dini_5p5km_10x8&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetLegendGraphic&LAYER=air_temperature_pl&WIDTH=100&HEIGHT=400&STYLE=temperature%2Fshaded&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename,
                                      data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath +
                                           filename),
        )


    def test_WMSGetLegendGraphic_LongTempLegend_height260(self):
        """Skip some labels when legend is too long, making sure the whole range is contained"""
        """Corresponds to the special case for definedLegendOnShadeClasses case in CCreateLegendRenderDiscreteLegend """
        AdagucTestTools().cleanTempDir()
        config = (ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," +
                  ADAGUC_PATH +
                  "/data/config/datasets/test.uwcw_ha43_dini_5p5km_10x8.xml")
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename = "test_WMSGetLegendGraphic_LongTempLegend_height260.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=test.uwcw_ha43_dini_5p5km_10x8&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetLegendGraphic&LAYER=air_temperature_pl&WIDTH=260&HEIGHT=260&STYLE=temperature%2Fshaded&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename,
                                      data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath +
                                           filename),
        )


    def test_WMSGetLegendGraphic_LongTempLegend_clipping(self):
        """Skip some labels when legend is too long and clip"""
        """Corresponds to the special case for definedLegendOnShadeClasses case in CCreateLegendRenderDiscreteLegend """
        AdagucTestTools().cleanTempDir()
        config = (ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," +
                  ADAGUC_PATH +
                  "/data/config/datasets/test.uwcw_ha43_dini_5p5km_10x8.xml")
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename = "test_WMSGetLegendGraphic_LongTempLegend_clipping.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=test.uwcw_ha43_dini_5p5km_10x8&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetLegendGraphic&LAYER=air_temperature_pl&WIDTH=100&HEIGHT=400&STYLE=temperature_clip%2Fshaded&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename,
                                      data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath +
                                           filename),
        )

    def test_WMSGetLegendGraphic_LongTempLegend2(self):
        """Another test showing another type of legend that skips classes without data"""
        """Corresponds to the discreteLegendOnInterval case in CCreateLegendRenderDiscreteLegend """
        AdagucTestTools().cleanTempDir()
        config = (ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," +
                  ADAGUC_PATH +
                  "/data/config/datasets/test.uwcw_ha43_dini_5p5km_10x8.xml")
        env = {"ADAGUC_CONFIG": config}
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename = "test_WMSGetLegendGraphic_LongTempLegend2.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=test.uwcw_ha43_dini_5p5km_10x8&SERVICE=WMS&&version=1.3.0&service=WMS&request=GetLegendGraphic&layer=air_temperature_pl&format=image/png&STYLE=temperature_wow/shaded&TRANSPARENT=TRUE&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename,
                                      data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath +
                                           filename),
        )

    def test_WMS_MultiDim_PreconfiguredDataBaseTable(self):
        """A dataset configuration with multiple dimensions and a DataBaseTable should work"""

        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/multi_dim.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMS_MultiDim_PreconfiguredDataBaseTable.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=multi_dim&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=my_testdata&WIDTH=193&HEIGHT=333&CRS=EPSG%3A3857&BBOX=521390.29046548845,6703587.877386019,789268.3657551267,7165781.654958297&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&time=2024-06-01T02%3A00%3A00Z&DIM_reference_time=2024-06-01T00%3A00%3A00Z&DIM_height=40",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_PASCAL_probabilities_manydims_timewindow(self):

        AdagucTestTools().cleanTempDir()
        filename_getcapabilities = "test_WMSGetCapabilities_PASCAL_probabilities_manydims_timewindow.xml"
        filename_getmap = "test_WMSGetMap_PASCAL_probabilities_manydims_timewindow.png"

        # pylint: disable=unused-variable
        status_getcapabilities, data_getcapabilities, _headers = AdagucTestTools().runADAGUCServer(
            "source=pascal/NetCDF4_probabilities_greater_than_or_equal_to_36_1_0_12_2025-07-02T10_00_00Z_2025-07-02T00_00_00Z%2Enc&SERVICE=WMS&&REQUEST=GetCapabilities&version=1.3.0",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename_getcapabilities, data_getcapabilities.getvalue())


        # pylint: disable=unused-variable
        status_getmap, data_getmap, _headers = AdagucTestTools().runADAGUCServer(
            "source=pascal/NetCDF4_probabilities_greater_than_or_equal_to_36_1_0_12_2025-07-02T10_00_00Z_2025-07-02T00_00_00Z%2Enc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=probabilities&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=-185283.26353977562,6054605.908001992,1548392.0145409694,7814973.6460993765&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&DIM_time_window=1&time=2025-07-02T10%3A00%3A00Z&DIM_reference_time=2025-07-02T00%3A00%3A00Z&DIM_radius=0.12&DIM_condition=greater_than_or_equal_to_36&0.5572985210530884",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename_getmap, data_getmap.getvalue())


        self.assertEqual(status_getcapabilities, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename_getcapabilities, self.expectedoutputsspath + filename_getcapabilities
            )
        )

        self.assertEqual(status_getmap, 0)
        self.assertEqual(
            data_getmap.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename_getmap),
        )

    def test_WMSGetMap_Barbs_example_windbarbs_from_pointdata_csv(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.vectorrendering.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_Barbs_example_windbarbs_from_pointdata_csv.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.vectorrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_from_pointdata_csv&WIDTH=1024&HEIGHT=1024&CRS=EPSG%3A3857&BBOX=-577014.3843391966,-2101256.2330806395,4248210.613219857,2100649.425887959&STYLES=windbarbs_kts_shaded_withbarbs_for_points%2Fpoint&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xA0F080&&time=2018-12-04T12%3A00%3A00Z&title=How%20to%20render%20windbarbs%20from%20point%20data,%20like%20CSV,%20NetCDF%20or%20GeoJSON&showscalebar=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                10,
            )
        )

    def test_WMSGetMap_Discs_example_windbarbs_from_pointdata_csv(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.vectorrendering.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_Discs_example_windbarbs_from_pointdata_csv.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.vectorrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_from_pointdata_csv&WIDTH=1024&HEIGHT=1024&CRS=EPSG%3A3857&BBOX=0.3843391966,-2101256.2330806395,2248210.613219857,2100649.425887959&STYLES=winddiscs_for_points%2Fpoint&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xA0F080&&time=2018-12-04T12%3A00%3A00Z&showscalebar=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                10,
            )
        )


    def test_WMSGetMap_Barbs_example_windbarbs_from_pointdata_csv_different_style_options(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.vectorrendering.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_Barbs_example_windbarbs_from_pointdata_csv_different_style_options.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.vectorrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_from_pointdata_csv&WIDTH=1024&HEIGHT=1024&CRS=EPSG%3A3857&BBOX=-577014.3843391966,-2101256.2330806395,4248210.613219857,2100649.425887959&STYLES=windbarbs_kts_shaded_withbarbs_for_points_differentoptions&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xA0F080&&time=2018-12-04T12%3A00%3A00Z&title=How%20to%20render%20windbarbs%20from%20point%20data,%20like%20CSV,%20NetCDF%20or%20GeoJSON&showscalebar=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_Barbs_example_windbarbs_on_gridded_netcdf(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.vectorrendering.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_Barbs_example_windbarbs_on_gridded_netcdf.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.vectorrendering.xml&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_on_gridded_netcdf&WIDTH=1352&HEIGHT=1319&CRS=EPSG%3A3857&BBOX=-65800.30054674105,6226801.574078708,1326094.9902426978,7584723.089279351&STYLES=windbarbs_kts_shaded_withbarbs_for_grids%2Fnearestpoint&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xA0F080&&time=2023-09-30T06%3A00%3A00Z&DIM_reference_time=2023-09-28T06%3A00%3A00Z&0.26085315983231405&showlegend=true&title=Wind%20barb%20style%20demo!&showscalebar=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_Discs_example_windbarbs_on_gridded_netcdf(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.vectorrendering.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_Discs_example_windbarbs_on_gridded_netcdf.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.vectorrendering.xml&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_on_gridded_netcdf&WIDTH=1352&HEIGHT=1319&CRS=EPSG%3A3857&BBOX=-65800.30054674105,6226801.574078708,1326094.9902426978,7584723.089279351&STYLES=winddiscs_for_grids%2Fpoint&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xA0F080&&time=2023-09-30T06%3A00%3A00Z&DIM_reference_time=2023-09-28T06%3A00%3A00Z&0.26085315983231405&showlegend=false&title=Wind disc demo&showscalebar=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename, self.testresultspath + filename, 30
            )
        )

    # FIXME: test fails because cell background is black without <RenderMethod>point</RenderMethod>
    def test_WMSGetMap_Arrows_example_windbarbs_on_gridded_netcdf(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.vectorrendering.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_Arrows_example_windbarbs_on_gridded_netcdf.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.vectorrendering.xml&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_on_gridded_netcdf&WIDTH=1352&HEIGHT=1319&CRS=EPSG%3A3857&BBOX=-65800.30054674105,6226801.574078708,1326094.9902426978,7584723.089279351&STYLES=windarrows_for_grids%2Fpoint&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0x80F0F0&&time=2023-09-30T06%3A00%3A00Z&DIM_reference_time=2023-09-28T06%3A00%3A00Z&0.26085315983231405&showlegend=false&title=Wind arrows demo&showscalebar=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_KNMI_WebSite_AnimatedGifImagery_temperature_styledisc(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.pointrendering.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_KNMI_WebSite_AnimatedGifImagery_temperature_styledisc.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.pointrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=overlaymask,ta&WIDTH=512&HEIGHT=512&CRS=EPSG%3A3857&BBOX=288069.7108512885,6471755.331201249,894206.0083504317,7141909.376801726&STYLES=filledcountries%2Fnearest,discs&FORMAT=image/png&TRANSPARENT=TRUE&&0.8777664780631963",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )


    def test_WMSGetMap_KNMI_WebSite_AnimatedGifImagery_temperature_style_temperature(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.pointrendering.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_KNMI_WebSite_AnimatedGifImagery_temperature_style_temperature.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.pointrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=overlaymask,ta&WIDTH=512&HEIGHT=512&CRS=EPSG%3A3857&BBOX=288069.7108512885,6471755.331201249,894206.0083504317,7141909.376801726&STYLES=filledcountries%2Fnearest,temperature&FORMAT=image/png&TRANSPARENT=TRUE&&0.8777664780631963",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )



    def test_WMSGetMap_KNMI_WebSite_AnimatedGifImagery_temperature_style_temperature_thinned(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.pointrendering.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_KNMI_WebSite_AnimatedGifImagery_temperature_style_temperature_thinned.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.pointrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=overlaymask,ta&WIDTH=512&HEIGHT=512&CRS=EPSG%3A3857&BBOX=288069.7108512885,6471755.331201249,894206.0083504317,7141909.376801726&STYLES=filledcountries%2Fnearest,temperature_thinned&FORMAT=image/png&TRANSPARENT=TRUE&&0.8777664780631963",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_select_temperature_as_point_from_grid(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.pointrendering.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_select_temperature_as_point_from_grid.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.pointrendering&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=air_temperature_hagl&WIDTH=654&HEIGHT=513&CRS=EPSG:3857&BBOX=-127880.43405139455,6311494.158487529,1447830.4566477134,7547487.563577196&STYLES=temperature_selectpoint%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&time=2024-05-23T01:00:00Z&DIM_reference_time=2024-05-23T00%3A00%3A00Z&DIM_member=1",
            env=env,showLog=True
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_windbarbs_on_gridded_netcdf_striding_offset(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.vectorrendering.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_windbarbs_on_gridded_netcdf_striding_offset.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.vectorrendering&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_on_gridded_netcdf&WIDTH=443&HEIGHT=493&CRS=EPSG%3A3857&BBOX=-327757.8031974508,5679892.056878187,1599560.8554856647,7824741.038211767&STYLES=vectors_with_striding%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&time=2023-09-30T06%3A00%3A00Z&DIM_reference_time=2023-09-28T06%3A00%3A00Z",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
            )
        )

    def test_WMSGetMap_windbarbs_kts_selectpoint_for_grids(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.pointrendering.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetMap_windbarbs_kts_selectpoint_for_grids.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.pointrendering&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=windbarbs_kts_selectpoint_for_grids&WIDTH=654&HEIGHT=513&CRS=EPSG:3857&BBOX=-127880.43405139455,6311494.158487529,1447830.4566477134,7547487.563577196&STYLES=windbarbs_kts_selectpoint_for_grids&FORMAT=image/png&TRANSPARENT=TRUE&time=2023-09-30T06:00:00Z&DIM_reference_time=2023-09-28T06:00:00Z",
            env=env,showLog=True
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )