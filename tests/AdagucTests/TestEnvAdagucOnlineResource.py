"""
Tests adaguc's ADAGUC_ONLINERESOURCE environment, this should be reflected in the GetCapabilities document
"""

import os
import unittest
import re
from lxml import objectify
from adaguc.AdagucTestTools import AdagucTestTools


from conftest import (
    make_adaguc_env,
    update_db,
)

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestEnvAdagucOnlineResource(unittest.TestCase):
    """
    class TestEnvAdagucOnlineResource(unittest.TestCase):

    """

    testresultspath = "testresults/TestEnvAdagucOnlineResource/"
    expectedoutputsspath = "expectedoutputs/TestEnvAdagucOnlineResource/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_env_adaguc_onlineresource(self):
        """
        Tests if ADAGUC_ONLINERESOURCE is correctly picked up in the GetCapabilities document
        """
        adaguc_env = make_adaguc_env("adaguc.test.members.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(adaguc_env)
        adaguc_env["ADAGUC_ONLINERESOURCE"] = "https://testhost/bla/wms?"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&request=getcapabilities", env=adaguc_env, showLog=False, showLogOnError=False
        )
        self.assertEqual(status, 0)
        capabilities = objectify.fromstring(re.sub(b' xmlns="[^"]+"', b"", data, count=1))
        assert (
            capabilities.Service.OnlineResource[0].attrib["{http://www.w3.org/1999/xlink}href"]
            == "https://testhost/bla/wms?DATASET=adaguc.test.members&SERVICE=WMS&"
        )
