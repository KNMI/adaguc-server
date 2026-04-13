# pylint: disable=missing-function-docstring
"""
This class contains tests to test the adaguc-server binary executable file. This is similar to black box testing, it tests the behaviour of the server software. It configures the server and checks if the response is OK.
"""

import os
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import (
    make_adaguc_env,
    run_adaguc_and_compare_file,
    run_adaguc_and_compare_getcapabilities,
    run_adaguc_and_compare_image,
    run_adaguc_and_compare_json,
    update_db,
)

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestMetadataRequest:
    """
    TestMetadataRequest class to thest Web Map Service behaviour of adaguc-server.
    """

    testresultspath = "testresults/TestMetadataRequest/"
    expectedoutputsspath = "expectedoutputs/TestMetadataRequest/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl(self):
        env = make_adaguc_env("adaguc.tests.arcus_uwcw.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_json(
            env,
            "test_GetMetadataRequest_arcus_uwcw.json",
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetMetadata&format=application/json",
        )

    def test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl_specific_query(self):
        env = make_adaguc_env("adaguc.tests.arcus_uwcw.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_json(
            env,
            "test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl_specific_query.json",
            "dataset=adaguc.tests.arcus_uwcw&&service=wms&version=1.3.0&request=getmetadata&format=application/json&dim_reference_time=2024-05-23T00:00:00Z&layer=air_temperature_hagl",
        )

    def test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl_non_existing_referencetime(self):
        env = make_adaguc_env("adaguc.tests.arcus_uwcw.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_json(
            env,
            "test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl_non_existing_referencetime.json",
            "dataset=adaguc.tests.arcus_uwcw&&service=wms&version=1.3.0&request=getmetadata&format=application/json&dim_reference_time=5024-05-23T00:00:00Z&layer=air_temperature_hagl",
        )
