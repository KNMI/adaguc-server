"""
This class contains tests to test the adaguc-server binary executable file. This is similar to black box testing, it tests the behaviour of the server software. It configures the server and checks if the response is OK.
"""

import json
import os
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import (
    make_adaguc_env,
    run_adaguc_and_compare_json,
    update_db,
    reset_datasetsloaded,
)

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestMetadataRequest:
    """
    TestMetadataRequest class to thest Web Map Service behaviour of adaguc-server.
    """

    testresultspath = "testresults/TestMetadataRequest/"
    expectedoutputsspath = "expectedoutputs/TestMetadataRequest/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    reset_datasetsloaded()
    AdagucTestTools().mkdir_p(testresultspath)

    def test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl(self):
        """
        This checks standard metadata call for arcus uwcw
        """
        env = make_adaguc_env("adaguc.tests.arcus_uwcw", self.testresultspath, self.expectedoutputsspath)
        update_db(env, True)
        run_adaguc_and_compare_json(
            env,
            "test_GetMetadataRequest_arcus_uwcw.json",
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetMetadata&format=application/json",
        )

    def test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl_specific_query(self):
        """
        This checks the metadata call for a specific reference time. The time dimension will then advertise the matching dates for the given model run.
        Also note that the maxquerylimit is set to 2 on purpose, to demonstrate that this does have no influence on the metadata result.
        """
        env = make_adaguc_env("adaguc.tests.arcus_uwcw", self.testresultspath, self.expectedoutputsspath)
        update_db(env, True)
        run_adaguc_and_compare_json(
            env,
            "test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl_specific_query.json",
            "dataset=adaguc.tests.arcus_uwcw&&service=wms&version=1.3.0&request=getmetadata&format=application/json&dim_reference_time=2024-05-23T00:00:00Z&layer=air_temperature_hagl_with_limit",
        )

    def test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl_non_existing_referencetime(self):
        """
        This checks for correct behavior when passing a invalid reference time, a status OK should be returned with empty dims.
        """
        env = make_adaguc_env("adaguc.tests.arcus_uwcw", self.testresultspath, self.expectedoutputsspath)
        update_db(env, True)
        run_adaguc_and_compare_json(
            env,
            "test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl_non_existing_referencetime.json",
            "dataset=adaguc.tests.arcus_uwcw&&service=wms&version=1.3.0&request=getmetadata&format=application/json&dim_reference_time=5024-05-23T00:00:00Z&layer=air_temperature_hagl",
        )

    def test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl_non_datasets(self):
        """
        This checks for correct behavior when passing a non-existing dataset
        """
        env = make_adaguc_env("adaguc.tests.arcus_uwcw", self.testresultspath, self.expectedoutputsspath)

        env2 = make_adaguc_env("blabla", self.testresultspath, self.expectedoutputsspath)
        update_db(env, True)
        run_adaguc_and_compare_json(
            env2,
            "test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl_non_datasets.json",
            "dataset=blabla&&service=wms&version=1.3.0&request=getmetadata&format=application/json&dim_reference_time=5024-05-23T00:00:00Z&layer=air_temperature_hagl",
            404,
        )

    def test_metadata_call_adaguc_tests_arcus_uwcw_air_temperature_ml_for_soundings(self):
        """
        This checks the metadata call for a specific reference time. The time dimension will then advertise the matching dates for the given model run.
        """
        env = make_adaguc_env("adaguc.tests.arcus_uwcw_ml.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env, True)
        run_adaguc_and_compare_json(
            env,
            "test_metadata_call_adaguc_tests_arcus_uwcw_air_temperature_ml_for_soundings_all_model_runs.json",
            "dataset=adaguc.tests.arcus_uwcw_ml&service=WMS&request=Getmetadata&format=application/json&layer=air_temperature_ml&",
        )
        run_adaguc_and_compare_json(
            env,
            "test_metadata_call_adaguc_tests_arcus_uwcw_air_temperature_ml_for_soundings_all_modelruns_060.json",
            "dataset=adaguc.tests.arcus_uwcw_ml&service=WMS&request=Getmetadata&format=application/json&layer=air_temperature_ml&DIM_reference_time=2026-09-28T06:00:00Z",
        )
        run_adaguc_and_compare_json(
            env,
            "test_metadata_call_adaguc_tests_arcus_uwcw_air_temperature_ml_for_soundings_all_modelruns_070.json",
            "dataset=adaguc.tests.arcus_uwcw_ml&service=WMS&request=Getmetadata&format=application/json&layer=air_temperature_ml&DIM_reference_time=2026-09-28T07:00:00Z",
        )
        run_adaguc_and_compare_json(
            env,
            "test_metadata_call_adaguc_tests_arcus_uwcw_air_temperature_ml_for_soundings_all_modelruns_080.json",
            "dataset=adaguc.tests.arcus_uwcw_ml&service=WMS&request=Getmetadata&format=application/json&layer=air_temperature_ml&DIM_reference_time=2026-09-28T08:00:00Z",
        )

    def test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl_many_referencetimes(self):
        """
        This checks that many reference times are returned as an interval in the GetMetadata response.
        """
        env = make_adaguc_env("adaguc.tests.arcus_uwcw_manymodelruns", self.testresultspath, self.expectedoutputsspath)
        update_db(env, True)
        filename = "test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl_many_referencetimes.json"
        run_adaguc_and_compare_json(
            env,
            filename,
            "dataset=adaguc.tests.arcus_uwcw_manymodelruns&&service=WMS&request=GetMetadata&format=application/json",
        )
        with open(self.testresultspath + filename, encoding="utf-8") as f:
            metadata = json.load(f)
        reference_time = metadata["adaguc.tests.arcus_uwcw_manymodelruns"]["air_temperature"]["dims"]["reference_time"]
        assert (
            reference_time["values"]
            == "2026-09-30T12:00:00Z,2026-09-30T13:00:00Z,2026-09-30T14:00:00Z,2026-09-30T15:00:00Z,2026-09-30T16:00:00Z,2026-09-30T17:00:00Z,2026-09-30T18:00:00Z,2026-09-30T19:00:00Z,2026-09-30T20:00:00Z,2026-09-30T21:00:00Z,2026-09-30T22:00:00Z,2026-09-30T23:00:00Z,2026-10-01T00:00:00Z,2026-10-01T01:00:00Z,2026-10-01T02:00:00Z,2026-10-01T03:00:00Z,2026-10-01T04:00:00Z,2026-10-01T05:00:00Z,2026-10-01T06:00:00Z,2026-10-01T07:00:00Z,2026-10-01T08:00:00Z,2026-10-01T09:00:00Z"
        )
