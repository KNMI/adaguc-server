import os
import os.path
import unittest

from adaguc.AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


# TODO: Parametrize
class TestWMSVolScan(unittest.TestCase):
    testresultspath = "testresults/TestWMSVolScan/"
    expectedoutputsspath = "expectedoutputs/TestWMSVolScan/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def checkReport(self, reportFilename="", expectedReportFilename=""):
        self.assertTrue(os.path.exists(reportFilename))
        self.assertEqual(
            AdagucTestTools().readfromfile(reportFilename),
            AdagucTestTools().readfromfile(
                self.expectedoutputsspath + expectedReportFilename
            ),
        )
        os.remove(reportFilename)

    def test_WMSGetCapabilities_VolScan(self):
        AdagucTestTools().cleanTempDir()
        for file_type in ["ODIM", "KNMI"]:
            filename = f"test_WMSGetCapabilities_{file_type}_VolScan.xml"
            status, data, headers = AdagucTestTools().runADAGUCServer(
                f"source=test/volscan/{file_type}_RAD_NL62_VOL_NA_202106181850.h5&SERVICE=WMS&request=getcapabilities",
                env=self.env,
            )
            AdagucTestTools().writetofile(
                self.testresultspath + filename, data.getvalue()
            )
            self.assertEqual(status, 0)
            self.assertTrue(
                AdagucTestTools().compareGetCapabilitiesXML(
                    self.testresultspath + filename,
                    self.expectedoutputsspath + filename,
                )
            )

    def test_WMSGetMap_VolScan(self):
        AdagucTestTools().cleanTempDir()
        for file_type in ["ODIM", "KNMI"]:
            for layer in ["ZDR", "KDP", "RHOHV", "Height"]:
                for elev in ["0.3l", "0.3", "8"]:
                    filename = f"test_WMSGetMap_VolScan_{layer}_{elev}.png"
                    request_layer = layer
                    if file_type == "KNMI" and layer == "RHOHV":
                        request_layer = "RhoHV"
                    wms_arg = f"source=test/volscan/{file_type}_RAD_NL62_VOL_NA_202106181850.h5&SERVICE=WMS&request=getmap&LAYERS={request_layer}&format=image/png&STYLES=&WMS=1.3.0&CRS=EPSG:4326&BBOX=46,0,58,12&WIDTH=400&HEIGHT=400&SHOWDIMS=true&DIM_scan_elevation={elev}"
                    status, data, headers = AdagucTestTools().runADAGUCServer(
                        wms_arg, env=self.env
                    )
                    AdagucTestTools().writetofile(
                        self.testresultspath + f"{file_type}_{filename}",
                        data.getvalue(),
                    )
                    self.assertEqual(status, 0)
                    self.assertEqual(
                        data.getvalue(),
                        AdagucTestTools().readfromfile(
                            self.expectedoutputsspath + f"{file_type}_{filename}",
                        )
                    )
