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
import netCDF4

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestConvertUnits(unittest.TestCase):
    """
    TestConvertUnits class to the convert_units DataPostProcessor of adaguc-server.
    """

    testresultspath = "testresults/TestConvertUnits/"
    expectedoutputsspath = "expectedoutputs/TestConvertUnits/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_convert_units_air_temperature_celsius(self):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.tests.convert_units.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename = "test_convert_units_air_temperature_celsius.json"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.convert_units&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=air_temperature&query_layers=air_temperature&crs=EPSG%3A3857&bbox=-28610.793749589706%2C6128671.920262324%2C1284405.9693875897%2C7705268.256668678&width=1076&height=1292&i=512&j=651&info_format=application%2Fjson&dim_reference_time=2026-02-13T00:00:00Z&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_convert_units_air_temperature_fahrenheit(self):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.tests.convert_units.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename = "test_convert_units_air_temperature_fahrenheit.json"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.convert_units&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=air_temperature_fahrenheit&query_layers=air_temperature_fahrenheit&crs=EPSG%3A3857&bbox=-28610.793749589706%2C6128671.920262324%2C1284405.9693875897%2C7705268.256668678&width=1076&height=1292&i=512&j=651&info_format=application%2Fjson&dim_reference_time=2026-02-13T00:00:00Z&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_convert_units_wind_components_barbs(self):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.tests.convert_units.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename = "test_convert_units_wind_components_barbs.png"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.convert_units&service=WMS&request=GetMap&version=1.3.0&style=barb&layers=wind_components_barbs&crs=EPSG%3A3857&bbox=-28610.793749589706%2C6128671.920262324%2C1284405.9693875897%2C7705268.256668678&width=1076&height=1292&dim_reference_time=2026-02-13T00:00:00Z&time=2026-02-13T00:00:00Z&FORMAT=image/png",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )
        filename = "test_convert_units_wind_components_barbs.json"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.convert_units&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=wind_components_barbs&query_layers=wind_components_barbs&crs=EPSG%3A3857&bbox=-28610.793749589706%2C6128671.920262324%2C1284405.9693875897%2C7705268.256668678&width=1076&height=1292&i=512&j=651&info_format=application%2Fjson&dim_reference_time=2026-02-13T00:00:00Z&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )
