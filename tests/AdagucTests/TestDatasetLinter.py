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

    def test_datasetlinter_invalid_style_values(self):
        """
        Test linter on a dataset with invalid values in its Style and Legend, checked with the STYLESHEET rules
        """
        adaguc_env = make_adaguc_env("adaguc.tests.testdatasetlinter-invalid-style.xml", self.testresultspath, self.expectedoutputsspath)
        args = ["--lint", "--config", adaguc_env.get("ADAGUC_CONFIG")]
        status, data, _ = AdagucTestTools().runADAGUCServer(args=args, env=adaguc_env, isCGI=False, showLog=True)
        assert status == 1
        output = data.decode()
        assert "Legend [testdatasetlinter_invalid_legend]: invalid value [300] for attribute [blue] in [palette]" in output
        assert "Style [testdata_style_invalid_values]: invalid value [0,5] for [Max]" in output
        assert "invalid value [#88000] for attribute [linecolor] in [ContourLine]" in output
        assert "unsupported format [%s] for attribute [textformatting] in [ContourLine], %g is used instead" in output
        assert "invalid value [0.10,a] for attribute [classes] in [ContourLine]" in output
        # -INF is a valid open interval in a configuration
        assert "FeatureInterval" not in output
        assert "Found (5) issues" in output
