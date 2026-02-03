import os
import pytest

from adaguc.AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestWMSVolScan:
    testresultspath = "testresults/TestWMSVolScan/"
    expectedoutputsspath = "expectedoutputs/TestWMSVolScan/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    @pytest.mark.parametrize(
        "file_type",
        [
            ("ODIM"),
            ("KNMI"),
        ],
    )
    def test_WMSGetCapabilities_VolScan(self, file_type: str):
        AdagucTestTools().cleanTempDir()

        filename = f"test_WMSGetCapabilities_{file_type}_VolScan.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            f"source=test/volscan/{file_type}_RAD_NL62_VOL_NA_202106181850.h5&SERVICE=WMS&request=getcapabilities",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())

        assert status == 0
        assert AdagucTestTools().compareGetCapabilitiesXML(
            self.testresultspath + filename,
            self.expectedoutputsspath + filename,
        )

    @pytest.mark.parametrize(
        ("file_type", "layer", "elevation"),
        [
            ("ODIM", "ZDR", "0.3l"),
            ("ODIM", "ZDR", "0.3"),
            ("ODIM", "ZDR", "8"),
            ("ODIM", "KDP", "0.3l"),
            ("ODIM", "KDP", "0.3"),
            ("ODIM", "KDP", "8"),
            ("ODIM", "RHOHV", "0.3l"),
            ("ODIM", "RHOHV", "0.3"),
            ("ODIM", "RHOHV", "8"),
            ("ODIM", "Height", "0.3l"),
            ("ODIM", "Height", "0.3"),
            ("ODIM", "Height", "8"),
            ("KNMI", "ZDR", "0.3l"),
            ("KNMI", "ZDR", "0.3"),
            ("KNMI", "ZDR", "8"),
            ("KNMI", "KDP", "0.3l"),
            ("KNMI", "KDP", "0.3"),
            ("KNMI", "KDP", "8"),
            ("KNMI", "RHOHV", "0.3l"),
            ("KNMI", "RHOHV", "0.3"),
            ("KNMI", "RHOHV", "8"),
            ("KNMI", "Height", "0.3l"),
            ("KNMI", "Height", "0.3"),
            ("KNMI", "Height", "8"),
        ],
    )
    def test_WMSGetMap_VolScan(self, file_type: str, layer: str, elevation: str):
        AdagucTestTools().cleanTempDir()

        filename = f"test_WMSGetMap_VolScan_{layer}_{elevation}.png"
        request_layer = layer
        if file_type == "KNMI" and layer == "RHOHV":
            request_layer = "RhoHV"

        wms_arg = f"source=test/volscan/{file_type}_RAD_NL62_VOL_NA_202106181850.h5&SERVICE=WMS&request=getmap&LAYERS={request_layer}&format=image/png&STYLES=&WMS=1.3.0&CRS=EPSG:4326&BBOX=46,0,58,12&WIDTH=400&HEIGHT=400&SHOWDIMS=true&DIM_scan_elevation={elevation}"
        status, data, _ = AdagucTestTools().runADAGUCServer(wms_arg, env=self.env)
        AdagucTestTools().writetofile(
            self.testresultspath + f"{file_type}_{filename}",
            data.getvalue(),
        )
        assert status == 0
        assert data.getvalue() == AdagucTestTools().readfromfile(self.expectedoutputsspath + f"{file_type}_{filename}")
