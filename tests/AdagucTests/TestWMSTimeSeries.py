# pylint: disable=invalid-name,missing-function-docstring
"""
This class contains tests to test the adaguc-server binary executable file. This is similar to black box testing, it tests the behaviour of the server software. It configures the server and checks if the response is OK.
"""
import json
import os
import os.path
import unittest
import sys
from adaguc.AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestWMSTimeSeries(unittest.TestCase):
    """
    TestWMSTimeSeries class to thest Web Map Service behaviour of adaguc-server.
    """

    testresultspath = "testresults/TestWMSTimeSeries/"
    expectedoutputsspath = "expectedoutputs/TestWMSTimeSeries/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl(self):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.arcus_uwcw.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_air_temperature_hagl.json"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=air_temperature_hagl&query_layers=air_temperature_hagl&crs=EPSG%3A3857&bbox=-28610.793749589706%2C6128671.920262324%2C1284405.9693875897%2C7705268.256668678&width=1076&height=1292&i=512&j=651&format=image%2Fgif&info_format=application%2Fjson&dim_reference_time=2024-05-23T00%3A00%3A00Z&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_member=*",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )



    def test_WMSGetFeatureInfo_timeseries_5dims_json(self):
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
            showLog=False,
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetFeatureInfo_timeseries_5dims_json.json"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "service=WMS&request=GetFeatureInfo&version=1.3.0&layers=data&query_layers=data&crs=EPSG%3A4326&bbox=-403.75436389819754%2C-192.99495925556732%2C220.28509739554607%2C253.15304074443293&width=943&height=1319&i=783&j=292&format=image%2Fgif&info_format=application%2Fjson&dim_member=*&elevation=*&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.timeseries.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )


    def test_WMSGetFeatureInfo_timeseries_forecastreferencetime_json(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetFeatureInfo_timeseries_forecastreferencetime.json"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=forecast_reference_time%2FHARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=air_temperature__at_2m&query_layers=air_temperature__at_2m&crs=EPSG%3A4326&bbox=47.80599631376197%2C1.4162628389784275%2C56.548995855839685%2C9.526486675156528&width=910&height=981&i=502&j=481&format=image%2Fgif&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_reference_time=2017-12-15T09%3A00%3A00Z",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )
        filename = "test_WMSGetCapabilities_testdatanc.xml"
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


    def test_WMSGetFeatureInfo_KNMIHDF5_echotops_RAD_NL25_ETH_NA_TOPS(self):
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

        filename = "test_WMSGetFeatureInfo_KNMIHDF5_echotops_RAD_NL25_ETH_NA_TOPS.json"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.KNMIHDF5.test&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetPointValue&LAYERS=RAD_NL25_ETH_NA_TOPS&CRS=EPSG%3A4326&INFO_FORMAT=application/json&time=2020-04-30T13%3A15%3A00Z&X=5.68&Y=50.89",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )


    def test_WMSGetFeatureInfo_KNMIHDF5_echotops_RAD_NL25_ETH_NA_GRID(self):
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

        filename = "test_WMSGetFeatureInfo_KNMIHDF5_echotops_RAD_NL25_ETH_NA_GRID.json"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.KNMIHDF5.test&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetPointValue&LAYERS=RAD_NL25_ETH_NA_GRID&CRS=EPSG%3A4326&INFO_FORMAT=application/json&time=2020-04-30T13%3A15%3A00Z&X=5.68&Y=50.89",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )


    def test_timeseries_adaguc_tests_arcus_uwcw_wind_kts(self):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.arcus_uwcw.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_wind_speed_hagl_convertedtokts.json"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=wind_speed_hagl_kts&query_layers=wind_speed_hagl_kts&crs=EPSG%3A3857&bbox=-20378.42428384231%2C5273127.343490437%2C1263825.4854055976%2C8560812.833440565&width=416&height=1065&i=172.99996948242188&j=561&format=image%2Fgif&info_format=application%2Fjson&dim_reference_time=2024-06-05T03%3A00%3A00Z&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_member=*",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_timeseries_adaguc_tests_arcus_uwcw_wind_kts_to_ms(self):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.arcus_uwcw.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_wind_speed_hagl_convertedtoms.json"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=wind_speed_hagl_ms&query_layers=wind_speed_hagl_ms&crs=EPSG%3A3857&bbox=-20378.42428384231%2C5273127.343490437%2C1263825.4854055976%2C8560812.833440565&width=416&height=1065&i=172.99996948242188&j=561&format=image%2Fgif&info_format=application%2Fjson&dim_reference_time=2024-06-05T03%3A00%3A00Z&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_member=*",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )


    def test_timeseries_adaguc_tests_arcus_uwcw_wind_kts_to_ms_wind_speed_hagl_ms_wrong_dim_order(self):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.arcus_uwcw.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_wind_speed_hagl_convertedtoms_wrong_order.json"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=wind_speed_hagl_ms_wrong_dim_order&query_layers=wind_speed_hagl_ms_wrong_dim_order&crs=EPSG%3A3857&bbox=-20378.42428384231%2C5273127.343490437%2C1263825.4854055976%2C8560812.833440565&width=416&height=1065&i=172.99996948242188&j=561&format=image%2Fgif&info_format=application%2Fjson&dim_reference_time=2024-06-05T03%3A00%3A00Z&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_member=*",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )


    def test_timeseries_adaguc_tests_arcus_uwcw_wind_kts_to_ms_member_3(self):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.arcus_uwcw.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_wind_speed_hagl_convertedtoms_member_3.json"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=wind_speed_hagl_ms_member_3&query_layers=wind_speed_hagl_ms_member_3&crs=EPSG%3A3857&bbox=-20378.42428384231%2C5273127.343490437%2C1263825.4854055976%2C8560812.833440565&width=416&height=1065&i=172.99996948242188&j=561&format=image%2Fgif&info_format=application%2Fjson&dim_reference_time=2024-06-05T03%3A00%3A00Z&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

        # Check if member_3 is the same as in the multidim file from previous test:
        data_file_member_3 = json.loads(data.getvalue())[0]["data"]["2024-06-05T03:00:00Z"]
        filename_many_members = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_wind_speed_hagl_convertedtoms.json"
        with open(self.expectedoutputsspath+filename_many_members, encoding = 'utf-8') as fp:
            filename_many_members_data = json.load(fp)
        data_file_all_members = filename_many_members_data[0]["data"]["2024-06-05T03:00:00Z"]
        assert data_file_member_3 == data_file_all_members["3"]


    def test_timeseries_adaguc_tests_arcus_uwcw_ha43_dini_5p5km_10x8_air_temperature_pl(self):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/test.uwcw_ha43_dini_5p5km_10x8.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_uwcw_ha43_dini_5p5km_10x8_air_temperature_pl.json"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetFeatureInfo&CRS=EPSG:4326&DATASET=test.uwcw_ha43_dini_5p5km_10x8&QUERY_LAYERS=air_temperature_pl&LAYERS=air_temperature_pl&crs=EPSG%3A3857&bbox=-20378.42428384231%2C5273127.343490437%2C1263825.4854055976%2C8560812.833440565&width=416&height=1065&i=172.99996948242188&j=561&INFO_FORMAT=application/json&TIME=2024-07-11T06:00:00Z/2024-07-11T07:00:00Z:00Z&DIM_reference_time=2024-07-11T05:00:00Z&DIM_pressure_level_in_hpa=*",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        # TODO Check why pressure levels sometimes have suffix .0 on different environments
        server_response = data.getvalue().decode("utf-8")
        server_response = server_response.replace("500.0", "500").replace("700.0", "700").replace("850.0", "850").replace("925.0", "925").encode("utf-8")

        AdagucTestTools().writetofile(self.testresultspath + filename, server_response)
        self.assertEqual(status, 0)
        # print("---------------- 1 -----------------", file=sys.stderr)
        # print(json.loads(server_response), file=sys.stderr)
        # print("---------------- 2 -----------------", file=sys.stderr)
        # print(json.loads(AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)), file=sys.stderr)
        # print("---------------- 3 -----------------", file=sys.stderr)
        assert  json.loads(server_response) == json.loads(AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
        



    def test_wmsgetfeatureinfo_json_timeheight_profile_for_sorted_model_levels(self):
      
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.arcus_harmonie_sorted_model_levels.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_WMSGetCapabilities_adaguc_arcus_harmonie_sorted_model_levels.xml"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.arcus_harmonie_sorted_model_levels&service=WMS&request=GetCapabilities",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )
        
        filename = "test_WMSGetFeatureInfoTimeSeries_arcus_harmonie_sorted_model_levels.json"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.arcus_harmonie_sorted_model_levels&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=air-temperature-ml&query_layers=air-temperature-ml&crs=EPSG%3A3857&bbox=-14204.36702572%2C1107764.666804308%2C1269999.54266372%2C12726175.510126693&width=106&height=959&i=54&j=478&format=image%2Fgif&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_reference_time=2024-12-22T06%3A00%3A00Z&dim_model_level_number=*&",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetFeatureInfo_exceed_maxquerylimit(self):
        AdagucTestTools().cleanTempDir()
        if os.getenv("ADAGUC_DB", "").endswith(".db"):
            print("SKIP: Only PSQL")
            return

        status, *_ = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH
                + "/data/config/adaguc.tests.dataset.xml,"
                + ADAGUC_PATH
                + "/data/config/datasets/adaguc.test.members.xml",
            ],
            isCGI=False,
        )
        self.assertEqual(status, 0)

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
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 422)

        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )
