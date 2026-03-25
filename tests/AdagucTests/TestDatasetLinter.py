"""
Tests adaguc dataset linter capability
"""

import os
import unittest
from adaguc.AdagucTestTools import AdagucTestTools


from conftest import (
    make_adaguc_env,
)

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestDatasetLinter(unittest.TestCase):
    """
    class TestDatasetLinter(unittest.TestCase):

    """

    testresultspath = "testresults/TestDatasetLinter/"
    expectedoutputsspath = "expectedoutputs/TestDatasetLinter/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_datasetlintercorrectlyconfigureddataset(self):
        """
        Test linter on a correctly configured dataset
        """
        adaguc_env = make_adaguc_env("adaguc.tests.testdatasetlinter-noissues.xml", self.testresultspath, self.expectedoutputsspath)
        args = ["--lint", "--config", adaguc_env.get("ADAGUC_CONFIG")]
        status, *_ = AdagucTestTools().runADAGUCServer(
            args=args,
            env=adaguc_env,
            isCGI=False,
            showLog=True,
        )
        assert status == 0

    def test_datasetlinter_unused_xmlattr(self):
        """
        Test linter on a misconfigured datasets
        """
        adaguc_env = make_adaguc_env("adaguc.tests.testdatasetlinter-unused-xmlattr.xml", self.testresultspath, self.expectedoutputsspath)
        args = ["--lint", "--config", adaguc_env.get("ADAGUC_CONFIG")]
        status, *_ = AdagucTestTools().runADAGUCServer(args=args, env=adaguc_env, isCGI=False, showLog=True)
        assert status == 1
