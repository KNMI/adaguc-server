"""
Test TimeHeight profiles for displaying CHM lidar data
"""

import os
import os.path
import unittest
import re
from lxml import etree, objectify

from adaguc.AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestWMSTimeHeightProfiles(unittest.TestCase):
    """
    All tests for WMSTimeHeight profile functionality
    """

    testresultspath = "testresults/TestWMSTimeHeightProfiles/"
    expectedoutputsspath = "expectedoutputs/TestWMSTimeHeightProfiles/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_wms_getcapabilities_timeheightprofiles(self):
        """
        Check if WMS GetCapabilities on time height profiles is as expected
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetCapabilities_TimeHeightProfiles.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=test/ceilonet/ceilonet_chm15k_backsct_la1_t12s_v1.0_06310_A20231202_extracted_small.nc&SERVICE=WMS&request=getcapabilities",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))

    def test_wms_getmap_timeheightprofiles(self):
        """
        Check if WMS GetMap on time height profiles is as expected
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_TimeHeightProfiles_4326_beta_raw.png"
        wms_arg = "source=test/ceilonet/ceilonet_chm15k_backsct_la1_t12s_v1.0_06310_A20231202_extracted_small.nc&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=beta_raw&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=259793.6688373399,6178346.966712794,1082625.1470173844,7357680.642536524&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&time=2023-12-02T06%3A40%3A00Z"
        status, data, _ = AdagucTestTools().runADAGUCServer(wms_arg, env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
            self.expectedoutputsspath + filename,
        )

    def test_wmsgetfeatureinfo_timeheightprofiles(self):
        """
        Check if WMS GetFeatureInfo on time height profiles is as expected using autowms with source=<file>
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetFeatureInfoAsPng_TimeHeightProfiles.png"
        wms_arg = "source=test/ceilonet/ceilonet_chm15k_backsct_la1_t12s_v1.0_06310_A20231202_extracted_small.nc&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=beta_raw&QUERY_LAYERS=beta_raw&BBOX=29109.947643979103,6500000,1190890.052356021,7200000&CRS=EPSG%3A3857&WIDTH=128&HEIGHT=512&I=707&J=557&INFO_FORMAT=image/png&STYLES=&time=2023-12-02T06:40:00Z/2023-12-02T06:45:00Z&elevation=0/2500"
        # TODO: Check why we first need to do a getcapabilities call?
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=test/ceilonet/ceilonet_chm15k_backsct_la1_t12s_v1.0_06310_A20231202_extracted_small.nc&SERVICE=WMS&request=getcapabilities",
            env=self.env,
        )
        status, data, _ = AdagucTestTools().runADAGUCServer(wms_arg, env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
            self.expectedoutputsspath + filename,
        )

    def test_wmsgetfeatureinfo_png_timeheightprofiles_as_dataset(self):
        """
        Check if WMS GetFeatureInfo on time height profiles is as expected using a dataset configuration with name adaguc.tests.timeheightprofiles
        """
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.timeheightprofiles.xml"
        )

        # Update database, scan the file
        status, data, _ = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        # Check getcapabilities on dataset configuration
        status, data, _ = AdagucTestTools().runADAGUCServer("SERVICE=WMS&request=getcapabilities", {"ADAGUC_CONFIG": config})
        filename = "test_wmsgetcapabilities_timeheightprofiles_as_dataset.xml"
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))

        # Check getfeatureinfo as PNG on dataset configuration
        filename = "test_wmsgetfeatureinfo_png_timeheightprofiles_as_dataset.png"
        wms_arg = "dataset=adaguc.tests.timeheightprofiles.xml&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=beta_raw&QUERY_LAYERS=beta_raw&BBOX=29109.947643979103,6500000,1190890.052356021,7200000&CRS=EPSG%3A3857&WIDTH=128&HEIGHT=512&I=707&J=557&INFO_FORMAT=image/png&STYLES=&time=2023-12-02T06:40:00Z/2023-12-02T06:45:00Z&elevation=0/2500"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            wms_arg,
            {"ADAGUC_CONFIG": config},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                0,
                0,
            )
        )

    def test_wmsgetfeatureinfo_json_timeheightprofiles_as_dataset(self):
        """
        Check if WMS GetFeatureInfo on time height profiles is as expected using dataset
        """
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.timeheightprofiles.xml"
        )

        # Update database, scan the file
        status, data, _ = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        # Check getcapabilities on dataset configuration
        status, data, _ = AdagucTestTools().runADAGUCServer("SERVICE=WMS&request=getcapabilities", {"ADAGUC_CONFIG": config})
        filename = "test_wmsgetcapabilities_timeheightprofiles_as_dataset.xml"
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))

        # Check getfeatureinfo as PNG on dataset configuration
        filename = "test_wmsgetfeatureinfo_json_timeheightprofiles_as_dataset.json"
        wms_arg = "dataset=adaguc.tests.timeheightprofiles.xml&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=beta_raw&QUERY_LAYERS=beta_raw&BBOX=29109.947643979103,6500000,1190890.052356021,7200000&CRS=EPSG%3A3857&WIDTH=128&HEIGHT=512&I=707&J=557&INFO_FORMAT=application/json&STYLES=&time=2023-12-02T06:40:00Z"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            wms_arg,
            {"ADAGUC_CONFIG": config},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)

        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
            self.expectedoutputsspath + filename,
        )
