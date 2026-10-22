# pylint: disable=invalid-name,missing-function-docstring
"""
This class contains tests to test the adaguc-server binary executable file. This is similar to black box testing, it tests the behaviour of the server software. It configures the server and checks if the response is OK.
"""

import json
import os
import unittest
from adaguc.AdagucTestTools import AdagucTestTools
import netCDF4
from conftest import (
    make_adaguc_autoresource_env,
    make_adaguc_env,
    run_adaguc_and_compare_file,
    run_adaguc_and_compare_getcapabilities,
    run_adaguc_and_compare_json,
    update_db,
)

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestWMSTimeSeries(unittest.TestCase):
    """
    TestWMSTimeSeries class to thest Web Map Service behaviour of adaguc-server.
    """

    testresultspath = "testresults/TestWMSTimeSeries/"
    expectedoutputsspath = "expectedoutputs/TestWMSTimeSeries/"

    AdagucTestTools().mkdir_p(testresultspath)

    def test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl(self):
        env = make_adaguc_env("adaguc.tests.arcus_uwcw", self.testresultspath, self.expectedoutputsspath)
        update_db(env)

        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_air_temperature_hagl.json"

        run_adaguc_and_compare_json(
            env,
            filename,
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=air_temperature_hagl&query_layers=air_temperature_hagl&crs=EPSG%3A3857&bbox=-28610.793749589706%2C6128671.920262324%2C1284405.9693875897%2C7705268.256668678&width=1076&height=1292&i=512&j=651&info_format=application%2Fjson&dim_reference_time=2024-05-23T00%3A00%3A00Z&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_member=*",
        )

    def test_WMSGetFeatureInfo_timeseries_5dims_json(self):
        env = make_adaguc_env("", self.testresultspath, self.expectedoutputsspath)
        env["ADAGUC_CONFIG"] = ADAGUC_PATH + "/data/config/adaguc.timeseries.xml"
        update_db(env)

        filename = "test_WMSGetFeatureInfo_timeseries_5dims_json.json"
        run_adaguc_and_compare_json(
            env,
            filename,
            "service=WMS&request=GetFeatureInfo&version=1.3.0&layers=data&query_layers=data&crs=EPSG%3A4326&bbox=-403.75436389819754%2C-192.99495925556732%2C220.28509739554607%2C253.15304074443293&width=943&height=1319&i=783&j=292&info_format=application%2Fjson&dim_member=*&elevation=*&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&",
        )

    def test_WMSGetFeatureInfo_timeseries_forecastreferencetime_json(self):
        env = make_adaguc_autoresource_env(self.testresultspath, self.expectedoutputsspath)
        filename = "test_WMSGetFeatureInfo_timeseries_forecastreferencetime.json"
        run_adaguc_and_compare_json(
            env,
            filename,
            "source=forecast_reference_time%2FHARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=air_temperature__at_2m&query_layers=air_temperature__at_2m&crs=EPSG%3A4326&bbox=47.80599631376197%2C1.4162628389784275%2C56.548995855839685%2C9.526486675156528&width=910&height=981&i=502&j=481&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_reference_time=2017-12-15T09%3A00%3A00Z",
        )

        filename = "test_WMSGetFeatureInfo_timeseries_forecastreferencetime_json_WMSGetCapabilities_testdatanc.xml"
        run_adaguc_and_compare_getcapabilities(
            env,
            filename,
            "source=testdata.nc&SERVICE=WMS&request=getcapabilities",
        )

    def test_WMSGetFeatureInfo_KNMIHDF5_echotops_RAD_NL25_ETH_NA_TOPS(self):
        env = make_adaguc_env("adaguc.KNMIHDF5.test", self.testresultspath, self.expectedoutputsspath)
        update_db(env)

        filename = "test_WMSGetFeatureInfo_KNMIHDF5_echotops_RAD_NL25_ETH_NA_TOPS.json"
        run_adaguc_and_compare_json(
            env,
            filename,
            "dataset=adaguc.KNMIHDF5.test&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetPointValue&LAYERS=RAD_NL25_ETH_NA_TOPS&CRS=EPSG%3A4326&INFO_FORMAT=application/json&time=2020-04-30T13%3A15%3A00Z&X=5.68&Y=50.89",
        )

    def test_WMSGetFeatureInfo_KNMIHDF5_echotops_RAD_NL25_ETH_NA_GRID(self):
        env = make_adaguc_env("adaguc.KNMIHDF5.test", self.testresultspath, self.expectedoutputsspath)
        update_db(env)

        filename = "test_WMSGetFeatureInfo_KNMIHDF5_echotops_RAD_NL25_ETH_NA_GRID.json"
        run_adaguc_and_compare_json(
            env,
            filename,
            "dataset=adaguc.KNMIHDF5.test&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetPointValue&LAYERS=RAD_NL25_ETH_NA_GRID&CRS=EPSG%3A4326&INFO_FORMAT=application/json&time=2020-04-30T13%3A15%3A00Z&X=5.68&Y=50.89",
        )

    def test_timeseries_adaguc_tests_arcus_uwcw_wind_kts(self):
        env = make_adaguc_env("adaguc.tests.arcus_uwcw", self.testresultspath, self.expectedoutputsspath)
        update_db(env)

        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_wind_speed_hagl_convertedtokts.json"

        run_adaguc_and_compare_json(
            env,
            filename,
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=wind_speed_hagl_kts&query_layers=wind_speed_hagl_kts&crs=EPSG%3A3857&bbox=-20378.42428384231%2C5273127.343490437%2C1263825.4854055976%2C8560812.833440565&width=416&height=1065&i=172.99996948242188&j=561&info_format=application%2Fjson&dim_reference_time=2024-06-05T03%3A00%3A00Z&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_member=*",
        )

    def test_timeseries_adaguc_tests_arcus_uwcw_wind_kts_to_ms(self):
        env = make_adaguc_env("adaguc.tests.arcus_uwcw", self.testresultspath, self.expectedoutputsspath)
        update_db(env)

        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_wind_speed_hagl_convertedtoms.json"

        run_adaguc_and_compare_json(
            env,
            filename,
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=wind_speed_hagl_ms&query_layers=wind_speed_hagl_ms&crs=EPSG%3A3857&bbox=-20378.42428384231%2C5273127.343490437%2C1263825.4854055976%2C8560812.833440565&width=416&height=1065&i=172.99996948242188&j=561&info_format=application%2Fjson&dim_reference_time=2024-06-05T03%3A00%3A00Z&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_member=*",
        )

    def test_timeseries_adaguc_tests_arcus_uwcw_wind_kts_to_ms_wind_speed_hagl_ms_wrong_dim_order(self):
        env = make_adaguc_env("adaguc.tests.arcus_uwcw", self.testresultspath, self.expectedoutputsspath)
        update_db(env)

        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_wind_speed_hagl_convertedtoms_wrong_order.json"

        run_adaguc_and_compare_json(
            env,
            filename,
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=wind_speed_hagl_ms_wrong_dim_order&query_layers=wind_speed_hagl_ms_wrong_dim_order&crs=EPSG%3A3857&bbox=-20378.42428384231%2C5273127.343490437%2C1263825.4854055976%2C8560812.833440565&width=416&height=1065&i=172.99996948242188&j=561&info_format=application%2Fjson&dim_reference_time=2024-06-05T03%3A00%3A00Z&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_member=*",
        )

    def test_timeseries_adaguc_tests_arcus_uwcw_wind_kts_to_ms_member_3(self):
        env = make_adaguc_env("adaguc.tests.arcus_uwcw")
        update_db(env)

        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_wind_speed_hagl_convertedtoms_member_3.json"

        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=wind_speed_hagl_ms_member_3&query_layers=wind_speed_hagl_ms_member_3&crs=EPSG%3A3857&bbox=-20378.42428384231%2C5273127.343490437%2C1263825.4854055976%2C8560812.833440565&width=416&height=1065&i=172.99996948242188&j=561&info_format=application%2Fjson&dim_reference_time=2024-06-05T03%3A00%3A00Z&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        self.assertEqual(status, 0)
        self.assertEqual(
            data,
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

        # Check if member_3 is the same as in the multidim file from previous test:
        data_file_member_3 = json.loads(data)[0]["data"]["2024-06-05T03:00:00Z"]
        filename_many_members = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_wind_speed_hagl_convertedtoms.json"
        with open(self.expectedoutputsspath + filename_many_members, encoding="utf-8") as fp:
            filename_many_members_data = json.load(fp)
        data_file_all_members = filename_many_members_data[0]["data"]["2024-06-05T03:00:00Z"]
        assert data_file_member_3 == data_file_all_members["3"]

    def test_timeseries_adaguc_tests_arcus_uwcw_ha43_dini_5p5km_10x8_air_temperature_pl(self):
        env = make_adaguc_env("test.uwcw_ha43_dini_5p5km_10x8")
        update_db(env)

        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_ha43_dini_5p5km_10x8_air_temperature_pl.json"

        status, data, _ = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetFeatureInfo&CRS=EPSG:4326&DATASET=test.uwcw_ha43_dini_5p5km_10x8&QUERY_LAYERS=air_temperature_pl&LAYERS=air_temperature_pl&crs=EPSG%3A3857&bbox=-20378.42428384231%2C5273127.343490437%2C1263825.4854055976%2C8560812.833440565&width=416&height=1065&i=172.99996948242188&j=561&INFO_FORMAT=application/json&TIME=2024-07-11T06:00:00Z/2024-07-11T07:00:00Z:00Z&DIM_reference_time=2024-07-11T05:00:00Z&DIM_pressure_level_in_hpa=*",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        # TODO Check why pressure levels sometimes have suffix .0 on different environments
        server_response = data.decode("utf-8")
        server_response = (
            server_response.replace("500.0", "500").replace("700.0", "700").replace("850.0", "850").replace("925.0", "925").encode("utf-8")
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, server_response)
        self.assertEqual(status, 0)
        assert json.loads(server_response) == json.loads(AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_wmsgetfeatureinfo_json_timeheight_profile_for_sorted_model_levels(self):
        env = make_adaguc_env("adaguc.tests.arcus_harmonie_sorted_model_levels", self.testresultspath, self.expectedoutputsspath)
        update_db(env)

        filename = "test_WMSGetCapabilities_adaguc_arcus_harmonie_sorted_model_levels.xml"
        run_adaguc_and_compare_getcapabilities(
            env,
            filename,
            "dataset=adaguc.tests.arcus_harmonie_sorted_model_levels&service=WMS&request=GetCapabilities",
        )

        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_harmonie_sorted_model_levels.json"

        run_adaguc_and_compare_json(
            env,
            filename,
            "dataset=adaguc.tests.arcus_harmonie_sorted_model_levels&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=air-temperature-ml&query_layers=air-temperature-ml&crs=EPSG%3A3857&bbox=-14204.36702572%2C1107764.666804308%2C1269999.54266372%2C12726175.510126693&width=106&height=959&i=54&j=478&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_reference_time=2024-12-22T06%3A00%3A00Z&dim_model_level_number=*&",
        )

    def test_WMSGetFeatureInfo_exceed_maxquerylimit(self):
        if os.getenv("ADAGUC_DB", "").endswith(".db"):
            print("SKIP: Only PSQL")
            return

        env = make_adaguc_env("adaguc.test.members")
        update_db(env)

        # Querying a NetCDF file with 60 timesteps and a single member should succeed
        status, *_ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.test.members&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=mymemberdata&query_layers=mymemberdata&crs=EPSG%3A3857&bbox=497598.1238462%2C6418354.600684818%2C838235.7656737999%2C7557289.301081181&width=294&height=983&i=134&j=352&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_reference_time=2025-03-01T00%3A00%3A00Z&dim_member=1",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        self.assertEqual(status, 0)

        # Querying all 50 members (members=*) will exceed the default `maxquerylimit=512` and should return a HTTP 422
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.test.members&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=mymemberdata&query_layers=mymemberdata&crs=EPSG%3A3857&bbox=497598.1238462%2C6418354.600684818%2C838235.7656737999%2C7557289.301081181&width=294&height=983&i=134&j=352&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_reference_time=2025-03-01T00%3A00%3A00Z&dim_member=*",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        filename = "test_WMSGetFeatureInfo_exceed_maxquerylimit"
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        self.assertEqual(status, 422)

        self.assertEqual(
            data,
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_UWCW_DINI_windcomponents_GetMetadata(self):
        filename = "test_UWCW_DINI_windcomponents_GetMetadata.json"
        env = make_adaguc_env("adaguc_tests_uwcwdini_windcomponents")
        update_db(env)

        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc_tests_uwcwdini_windcomponents&service=wms&request=getmetadata&format=application/json",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        self.assertEqual(status, 0)

        metadata_json = json.loads(data)

        self.assertEqual(len(metadata_json["adaguc_tests_uwcwdini_windcomponents"]["wind-hagl"]["layer"]["variables"]), 6)

        self.assertEqual(metadata_json["adaguc_tests_uwcwdini_windcomponents"]["wind-hagl"]["layer"]["layername"], "wind-hagl")

    def test_UWCW_DINI_windcomponents_xwind_ywind_WMSGetMapBarbs(self):
        filename = "test_UWCW_DINI_windcomponents_xwind_ywind_WMSGetMapBarbs.png"
        env = make_adaguc_env("adaguc_tests_uwcwdini_windcomponents")
        update_db(env)

        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc_tests_uwcwdini_windcomponents&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind-hagl&WIDTH=512&HEIGHT=512&CRS=EPSG%3A3857&BBOX=4509.516234000213,5087774.625591068,1352501.5362287262,6506770.215673305&STYLES=windbarbs_kts%2Fbarbshadedcontour&FORMAT=image/png&TRANSPARENT=TRUE&&DIM_wind_at_10m=10&time=2024-09-07T12%3A00%3A00Z&DIM_reference_time=2024-09-05T00%3A00%3A00Z&showlegend=true",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                37,
                0.1,
            )
        )

    def test_UWCW_DINI_windcomponents_xwind_ywind_WMSGetFeatureInfoTimeSeries(self):
        filename = "test_UWCW_DINI_windcomponents_xwind_ywind_WMSGetFeatureInfoTimeSeries.json"
        env = make_adaguc_env("adaguc_tests_uwcwdini_windcomponents", self.testresultspath, self.expectedoutputsspath)
        update_db(env)

        run_adaguc_and_compare_json(
            env,
            filename,
            "dataset=adaguc_tests_uwcwdini_windcomponents&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=wind-hagl&query_layers=wind-hagl&crs=EPSG%3A3857&bbox=4509.516234000446%2C4728761.919206079%2C1352501.536228726%2C6865782.922058294&width=832&height=1319&i=423&j=647&info_format=application%2Fjson&dim_wind_at_10m=10&time=2024-09-05T20%3A00%3A00Z%2F2024-09-05T20%3A00%3A00Z&dim_reference_time=2024-09-05T00%3A00%3A00Z&",
        )

    def test_UWCW_DINI_windcomponents_xwind_ywind_WMSGetFeatureInfoHtml(self):
        filename = "test_UWCW_DINI_windcomponents_xwind_ywind_WMSGetFeatureInfoHtml.html"
        env = make_adaguc_env("adaguc_tests_uwcwdini_windcomponents", self.testresultspath, self.expectedoutputsspath)
        update_db(env)

        run_adaguc_and_compare_file(
            env,
            filename,
            "dataset=adaguc_tests_uwcwdini_windcomponents&&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=wind-hagl&QUERY_LAYERS=wind-hagl&CRS=EPSG%3A3857&BBOX=-2696318.373760471,3217800.239685233,2821293.326680254,11965071.673436817&WIDTH=832&HEIGHT=1319&I=461&J=696&INFO_FORMAT=text/html&STYLES=&&DIM_wind_at_10m=10&time=2024-09-07T12%3A00%3A00Z&DIM_reference_time=2024-09-05T00%3A00%3A00Z",
        )

    def test_UWCW_DINI_windcomponents_xwind_ywind_WCSGetCoverage(self):
        filename = "test_UWCW_DINI_windcomponents_xwind_ywind_WCSGetCoverage.nc"
        env = make_adaguc_env("adaguc_tests_uwcwdini_windcomponents")
        update_db(env)

        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc_tests_uwcwdini_windcomponents&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=wind-hagl&CRS=EPSG%3A4326&FORMAT=NetCDF3&BBOX=-42.15749,37.709509,38.831969,69.575&RESX=10.123682375000001&RESY=4.552213000000001&DIM_WIND_AT_10M=10&TIME=2024-09-07T12:00:00Z&DIM_REFERENCE_TIME=2024-09-05T00:00:00Z",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)

        self.assertEqual(status, 0)

        # Check NetCDF file: Number of variables and global attribute
        ds = netCDF4.Dataset("filename.nc", memory=data)
        self.assertEqual(ds.getncattr("convert_uv_components"), "metadata")
        self.assertEqual(
            list(ds.variables),
            [
                "x",
                "y",
                "time",
                "wind_at_10m",
                "forecast_reference_time",
                "crs",
                "speed_component",
                "direction_component",
                "eastward_component",
                "northward_component",
                "x-wind-hagl",
                "y-wind-hagl",
            ],
        )

        # Do getmap request on the NetCDF file we just obtained via WCS, query the direction_component
        filename = "test_UWCW_DINI_windcomponents_xwind_ywind_GetMapOnWCSCoverage_direction_component.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=TestWMSTimeSeries%2Ftest_UWCW_DINI_windcomponents_xwind_ywind_WCSGetCoverage%2Enc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=direction_component&WIDTH=512HEIGHT=512&CRS=EPSG%3A3857&BBOX=-4787988.272387099,4425096.702571644,4348576.567740106,10946270.416926505&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&DIM_wind_at_10m=10&time=2024-09-07T12%3A00%3A00Z&DIM_reference_time=2024-09-05T00%3A00%3A00Z&0.783935864573529",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "data/config/adaguc.autoresource.xml", "ADAGUC_AUTOWMS_DIR": self.testresultspath},
            showLog=False,
        )
        self.assertEqual(status, 0)
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                37,
                0.1,
            )
        )

        # Do getmap request on the NetCDF file we just obtained via WCS, query the speed_component
        filename = "test_UWCW_DINI_windcomponents_xwind_ywind_GetMapOnWCSCoverage_speed_component.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=TestWMSTimeSeries%2Ftest_UWCW_DINI_windcomponents_xwind_ywind_WCSGetCoverage%2Enc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=speed_component&WIDTH=512HEIGHT=512&CRS=EPSG%3A3857&BBOX=-4787988.272387099,4425096.702571644,4348576.567740106,10946270.416926505&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&DIM_wind_at_10m=10&time=2024-09-07T12%3A00%3A00Z&DIM_reference_time=2024-09-05T00%3A00%3A00Z&0.783935864573529",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "data/config/adaguc.autoresource.xml", "ADAGUC_AUTOWMS_DIR": self.testresultspath},
            showLog=False,
        )
        self.assertEqual(status, 0)
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                37,
                0.1,
            )
        )

    def test_timeseries_adaguc_tests_adaguc_tests_multi_reftime_temporal_extent_asterisk(self):
        """
        This checks for correct behavior when passing a non-existing dataset
        """
        env = make_adaguc_env("adaguc.tests.multi_reftime_temporal_extent", self.testresultspath, self.expectedoutputsspath)
        update_db(env, force_update=True)
        run_adaguc_and_compare_json(
            env,
            "test_timeseries_adaguc_tests_adaguc_tests_multi_reftime_temporal_extent_asterisk.json",
            "time=*&dataset=adaguc.tests.multi_reftime_temporal_extent&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=weather_code_gl&query_layers=weather_code_gl&crs=EPSG%3A3857&bbox=-14204.36702572%2C3630706.712827636%2C1269999.54266372%2C10203233.464103365&width=178&height=911&i=107&j=458&format=image%2Fgif&info_format=application%2Fjson&dim_reference_time=2026-04-07T00%3A00%3A00Z&",
        )

    def test_changing_height_levels(self):
        """Defaults must select available heights as forecast runs lose levels."""

        env = make_adaguc_env("adaguc.tests.changing_dimension_levels")
        fixtures = ADAGUC_PATH + "/data/datasets/windspeed_changing_levels/"

        # Ingest runs in order with different heights: (10, 20, 50) in one file, only 10 in file, only 20 in file, then only 50 in file.
        for run in ["20260902T12", "20260908T12", "20260909T12", "20260910T12"]:
            update_db(env, filepath=fixtures + f"uwcw-ha-det-nl-2km_{run}_wind-speed-hagl_small.nc", force_update=True)

        # Check the selected height for both defaults, including the original run.
        for reference_date, min_height, max_height in [
            ("2026-09-02", 10, 50),
            ("2026-09-08", 10, 10),
            ("2026-09-09", 20, 20),
            ("2026-09-10", 50, 50),
        ]:
            for layer, height in [("wind_speed_min", min_height), ("wind_speed_max", max_height)]:
                reference_time = f"{reference_date}T12:00:00Z"
                time = f"{reference_date}T13:00:00Z"
                query = (
                    f"service=WMS&version=1.3.0&request=GetPointValue&query_layers={layer}&crs=EPSG:4326"
                    f"&X=5.626&Y=52.51&info_format=application/json&dim_reference_time={reference_time}&time={time}"
                )
                status, data, _ = AdagucTestTools().runADAGUCServer(url=query, env=env)
                assert status == 0
                heights = json.loads(data)[0]["data"][reference_time]
                assert list(heights) == [str(height)]

        # The configured fixed reference time must select the run with height 20,
        # overriding a conflicting request value or supplying the omitted value.
        for reference_time in ["&dim_reference_time=2026-09-02T12:00:00Z", ""]:
            query = (
                "service=WMS&version=1.3.0&request=GetPointValue&query_layers=wind_speed_fixed_run&crs=EPSG:4326"
                "&time=2026-09-09T13:00:00Z&X=5.626&Y=52.51&info_format=application/json" + reference_time
            )
            status, data, _ = AdagucTestTools().runADAGUCServer(url=query, env=env)
            assert status == 0
            heights = json.loads(data)[0]["data"]["2026-09-09T12:00:00Z"]
            assert list(heights) == ["20"]
