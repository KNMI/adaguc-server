"""
Tests custom styles posted with the STYLESHEET parameter (form url encoded POST through the FastAPI server).
The posted Style is used for the requested layer (only one allowed), regardless of its name and of STYLES, for that request only.
A posted Legend replaces the configured one with the same name.
"""

import os
import sys
import unittest
from importlib import reload
from unittest.mock import patch
from urllib.parse import parse_qsl
from adaguc.AdagucTestTools import AdagucTestTools
from fastapi.testclient import TestClient
from conftest import make_adaguc_env, update_db

ADAGUC_PATH = os.environ["ADAGUC_PATH"]
sys.path.insert(0, os.path.join(ADAGUC_PATH, "python", "python_fastapi_server"))
import main  # pylint: disable=wrong-import-position

DATASET = "adaguc.tests.customstyles"
GET_MAP_QUERY = "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata_style_manycontours&FORMAT=image/png&TRANSPARENT=FALSE"

# Same as the dataset's style, without StandardNames
SAME_STYLE = """
  <Style name="testdata_style_manycontours" title="Rainbow colors" abstract="Drawing with rainbow colors">
    <Legend fixedclasses="true" tickinterval="0.1" tickround=".01">no2</Legend>
    <Min>0.0</Min>
    <Max>0.5</Max>
    <ContourLine width="0.8" linecolor="#880000" textcolor="#000000" textformatting="%2.2f" classes="0.10"/>
    <ContourLine width="0.8" linecolor="#000000" textcolor="#000000" textformatting="%2.2f" classes="0.01"/>
  </Style>
"""

# Other colours and contour intervals, with its own Legend. The name differs from the dataset's style on purpose.
VARIANT_STYLE = """
  <Legend name="customstyles_bluegreen" type="colorRange">
    <palette index="0" red="0" green="0" blue="128"/>
    <palette index="120" red="0" green="160" blue="255"/>
    <palette index="240" red="220" green="255" blue="200"/>
  </Legend>
  <Style name="my_custom_style" title="Blue green" abstract="Drawing with blue and green colors">
    <Legend fixedclasses="true" tickinterval="0.1" tickround=".01">customstyles_bluegreen</Legend>
    <Min>0.0</Min>
    <Max>0.5</Max>
    <ContourLine width="2" linecolor="#FFFF00" textcolor="#FFFFFF" textformatting="%2.2f" classes="0.05"/>
    <ContourLine width="0.5" linecolor="#00FF00" textcolor="#FFFFFF" textformatting="%2.3f" classes="0.025"/>
  </Style>
"""


class TestCustomStyles(unittest.TestCase):
    """
    Tests styles posted in the STYLESHEET parameter
    """

    testresultspath = "testresults/TestCustomStyles/"
    expectedoutputsspath = "expectedoutputs/TestCustomStyles/"

    AdagucTestTools().mkdir_p(testresultspath)

    def setUp(self):
        self.env = make_adaguc_env(DATASET, self.testresultspath, self.expectedoutputsspath)
        update_db(self.env)
        patcher = patch.dict(os.environ, {"ADAGUC_CONFIG": self.env["ADAGUC_CONFIG"]})
        patcher.start()
        self.addCleanup(patcher.stop)
        reload(main)
        self.client = TestClient(main.app)

    def post_getmap(self, stylesheet=None, styles=None):
        params = dict(parse_qsl(GET_MAP_QUERY))
        if styles is not None:
            params["STYLES"] = styles
        if stylesheet is not None:
            params["STYLESHEET"] = stylesheet
        return self.client.post("/wms", data=params)

    def compare_image(self, response, filename):
        self.assertEqual(response.status_code, 200, response.text)
        self.assertEqual(response.headers["content-type"], "image/png")
        AdagucTestTools().writetofile(self.testresultspath + filename, response.content)
        self.assertTrue(AdagucTestTools().compareImage(self.expectedoutputsspath + filename, self.testresultspath + filename))

    def test_CustomStyles_dataset_style(self):
        """Reference: the style as configured in the dataset, without STYLESHEET"""
        self.compare_image(self.post_getmap(), "test_CustomStyles_dataset_style.png")

    def test_CustomStyles_post_same_style(self):
        """Posting the configured style again gives the same image as the dataset style"""
        self.compare_image(self.post_getmap(SAME_STYLE), "test_CustomStyles_dataset_style.png")

    def test_CustomStyles_post_variant_style(self):
        """Posting a style with other colours and contour lines overrides the configured one"""
        response = self.post_getmap(VARIANT_STYLE)
        self.compare_image(response, "test_CustomStyles_post_variant_style.png")
        self.assertNotEqual(response.content, self.post_getmap().content)

    def test_CustomStyles_post_variant_style_ignores_styles(self):
        """The posted style is used whatever STYLES is set to"""
        for styles in ["", "default", "testdata_style_manycontours/nearest", "does_not_exist"]:
            with self.subTest(styles=styles):
                self.compare_image(self.post_getmap(VARIANT_STYLE, styles=styles), "test_CustomStyles_post_variant_style.png")

    def test_CustomStyles_variant_is_per_request(self):
        """A posted style only applies to its own request"""
        self.post_getmap(VARIANT_STYLE)
        self.compare_image(self.post_getmap(), "test_CustomStyles_dataset_style.png")

    def test_CustomStyles_rejects_layer(self):
        """Only Style and Legend elements may be posted"""
        response = self.post_getmap('<Layer type="database"><FilePath>/etc</FilePath><Variable>x</Variable></Layer>')
        self.assertEqual(response.status_code, 422)

    def test_CustomStyles_rejects_file_attribute(self):
        """Attributes that make the server read a local file are refused"""
        response = self.post_getmap('<Legend name="no2" file="/etc/passwd"/>')
        self.assertEqual(response.status_code, 422)

    def test_CustomStyles_rejects_multiple_layers(self):
        """A STYLESHEET applies to the one requested layer, more layers are refused"""
        params = dict(parse_qsl(GET_MAP_QUERY))
        params["LAYERS"] = "testdata,testdata"
        params["STYLESHEET"] = VARIANT_STYLE
        response = self.client.post("/wms", data=params)
        self.assertEqual(response.status_code, 422)

    def test_CustomStyles_rejects_multiple_styles(self):
        """Only one Style can be posted, it is used for every layer"""
        response = self.post_getmap(SAME_STYLE + SAME_STYLE)
        self.assertEqual(response.status_code, 422)

    def test_CustomStyles_rejects_invalid_xml(self):
        response = self.post_getmap("<Style name=")
        self.assertEqual(response.status_code, 422)

    def test_CustomStyles_symbol_from_symbol_dir(self):
        """A SymbolInterval may use a .png from the server's symbol directories"""
        symbol_style = """
          <Style name="symbols">
            <Legend>rainbow</Legend>
            <Min>0.0</Min>
            <Max>0.5</Max>
            <DataPostProc algorithm="pointsfromgrid" a="40" select="testdata"/>
            <SymbolInterval file="{ADAGUC_PATH}data/resources/icons/helicopter.png"/>
            <Point pointstyle="symbol" discradius="0" textradius="0" dot="false" fontsize="8" textformat=" "/>
          </Style>
        """
        self.compare_image(self.post_getmap(symbol_style), "test_CustomStyles_symbol_from_symbol_dir.png")

    def test_CustomStyles_rejects_symbol_outside_symbol_dirs(self):
        """Symbol files outside the symbol directories, or not .png, are refused"""
        for file in [
            "/etc/passwd",
            "{ADAGUC_PATH}data/symbols/../../../../../etc/passwd",
            "{ADAGUC_PATH}data/resources/locations/global_edr_locations.geojson",
            "{ADAGUC_PATH}data/symbols/does_not_exist.png",
        ]:
            response = self.post_getmap(f'<Style name="s"><SymbolInterval file="{file}"/><Point pointstyle="symbol"/></Style>')
            self.assertEqual(response.status_code, 422, file)

    def test_CustomStyles_rejects_file_attribute_outside_symbolinterval(self):
        response = self.post_getmap('<Style name="s"><Point file="{ADAGUC_PATH}data/symbols/marker.png"/></Style>')
        self.assertEqual(response.status_code, 422)
