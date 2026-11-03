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

    def test_CustomStyles_symbol_from_dataset_dir(self):
        """A SymbolInterval may use a .png from ADAGUC_DATASET_DIR, including its subdirectories"""
        symbol_style = """
          <Style name="symbols">
            <Legend>rainbow</Legend>
            <Min>0.0</Min>
            <Max>0.5</Max>
            <DataPostProc algorithm="pointsfromgrid" a="40" select="testdata"/>
            <SymbolInterval file="{ADAGUC_DATASET_DIR}/resources/symbols/okta/32px-Cloud_cover_3.svg.png"/>
            <Point pointstyle="symbol" discradius="0" textradius="0" dot="false" fontsize="8" textformat=" "/>
          </Style>
        """
        self.compare_image(self.post_getmap(symbol_style), "test_CustomStyles_symbol_from_dataset_dir.png")

    def test_CustomStyles_symbol_in_dataset_dir_root(self):
        """A .png directly in ADAGUC_DATASET_DIR is accepted as well"""
        response = self.post_getmap(
            '<Style name="s"><SymbolInterval file="{ADAGUC_DATASET_DIR}/csv_radiusandvalue_legend.png"/><Point pointstyle="symbol"/></Style>'
        )
        self.assertEqual(response.status_code, 200, response.text)
        self.assertEqual(response.headers["content-type"], "image/png")

    def test_CustomStyles_rejects_symbol_outside_symbol_dirs(self):
        """Symbol files outside the symbol directories, or not .png, are refused"""
        for file in [
            "/etc/passwd",
            "{ADAGUC_PATH}data/symbols/../../../../../etc/passwd",
            "{ADAGUC_PATH}data/resources/locations/global_edr_locations.geojson",
            "{ADAGUC_PATH}data/symbols/does_not_exist.png",
            "{ADAGUC_DATASET_DIR}/adaguc.tests.customstyles.xml",
            "{ADAGUC_DATASET_DIR}/../adaguc.tests.dataset.xml",
        ]:
            response = self.post_getmap(f'<Style name="s"><SymbolInterval file="{file}"/><Point pointstyle="symbol"/></Style>')
            self.assertEqual(response.status_code, 422, file)

    def test_CustomStyles_rejects_file_attribute_outside_symbolinterval(self):
        response = self.post_getmap('<Style name="s"><Point file="{ADAGUC_PATH}data/symbols/marker.png"/></Style>')
        self.assertEqual(response.status_code, 422)

    def test_CustomStyles_legendgraphic_from_symbol_dir(self):
        """A LegendGraphic may use a .png from the server's symbol directories"""
        response = self.post_getmap('<Style name="s"><LegendGraphic value="{ADAGUC_PATH}data/symbols/marker.png"/></Style>')
        self.assertEqual(response.status_code, 200, response.text)

    def test_CustomStyles_rejects_legendgraphic_outside_symbol_dirs(self):
        """A LegendGraphic is read as a local file, so the same rules as for symbols apply"""
        for value in ["/etc/passwd", "{ADAGUC_PATH}data/symbols/../../../../../etc/passwd", "/"]:
            response = self.post_getmap(f'<Style name="s"><LegendGraphic value="{value}"/></Style>')
            self.assertEqual(response.status_code, 422, value)

    def test_CustomStyles_large_stylesheet(self):
        """A stylesheet larger than the old 8192 byte fork server limit is accepted"""
        intervals = "".join(f'<ShadeInterval min="{i / 1000}" max="{(i + 1) / 1000}" fillcolor="#{i % 256:02x}0000"/>' for i in range(200))
        stylesheet = f'<Style name="large"><Legend>rainbow</Legend><Min>0</Min><Max>0.5</Max><RenderMethod>shaded</RenderMethod>{intervals}</Style>'
        self.assertGreater(len(stylesheet), 8192)
        response = self.post_getmap(stylesheet)
        self.assertEqual(response.status_code, 200, response.text)

    def contour_style(self, textformatting):
        return f'<Style name="s"><Legend>rainbow</Legend><Min>0</Min><Max>0.5</Max><ContourLine linecolor="#000000" textcolor="#000000" textformatting="{textformatting}" interval="0.05"/></Style>'

    def test_CustomStyles_rejects_unsafe_formats(self):
        """printf formats get a double: anything but one float conversion could crash or leak memory"""
        for fmt in ["%s", "%s%s%s%s", "%p %p", "%n", "%d", "%x", "%*f", "%1$f", "%Lf", "%f %f", "%123f", "%"]:
            for stylesheet in [
                self.contour_style(fmt),
                f'<Legend name="l" type="colorRange" textformatting="{fmt}"/>',
                f'<Style name="s"><Legend textformatting="{fmt}">rainbow</Legend></Style>',
                f'<Style name="s"><Point textformat="{fmt}"/></Style>',
                f'<Style name="s"><Vector textformat="{fmt}"/></Style>',
            ]:
                with self.subTest(stylesheet=stylesheet):
                    self.assertEqual(self.post_getmap(stylesheet).status_code, 422)

    def test_CustomStyles_accepts_safe_formats(self):
        for fmt in ["%2.2f", "%.1f degC", "%g", "%e", "%+08.3f", "100%%", " ", ""]:
            with self.subTest(fmt=fmt):
                response = self.post_getmap(self.contour_style(fmt))
                self.assertEqual(response.status_code, 200, response.text)

    def test_CustomStyles_rejects_disallowed_content(self):
        """Only allowlisted elements and attributes with valid values may be posted"""
        for stylesheet in [
            # File access and other layers
            '<Style name="s"><DataPostProc algorithm="addfeatures" a="/etc/passwd"/></Style>',
            '<Style name="s"><DataPostProc algorithm="include_layer" name="testdata"/></Style>',
            '<Style name="s"><DataPostProc algorithm="pointsfromgrid" name="x"/></Style>',
            '<Style name="s"><Point fontfile="/etc/passwd"/></Style>',
            '<Style name="s"><Point symbol="/etc/passwd"/></Style>',
            '<Style name="s"><FeatureInterval labelfontfile="/etc/passwd"/></Style>',
            '<Legend name="l" type="svg" file="/etc/passwd"/>',
            # Applied to strings and integers too, so no safe format exists
            '<Style name="s"><FeatureInterval labelpropertyformat="%s"/></Style>',
            # Unbounded recursion
            '<Style name="s"><IncludeStyle name="s"/></Style>',
            # Unknown elements, attributes and markup
            '<Style name="s"><Foo/></Style>',
            '<Style name="s"><ContourLine foo="1"/></Style>',
            '<Style name="s"><?php echo 1; ?></Style>',
            '<Configuration foo="1"><Style name="s"/></Configuration>',
            '<?xml version="1.0"?><!DOCTYPE Configuration [<!ENTITY x "y">]><Configuration><Style name="&x;"/></Configuration>',
            # Regex with back references
            '<Style name="s"><FeatureInterval match="\\(a*\\)*\\1"/></Style>',
            # Values that make loops endless or huge
            '<Style name="s"><Stippling distancex="-5"/></Style>',
            '<Style name="s"><SmoothingFilter>1000</SmoothingFilter></Style>',
            '<Style name="s"><RenderSettings striding="0"/></Style>',
            '<Style name="s"><ContourLine interval="0"/></Style>',
            '<Style name="s"><ContourLine interval="-1"/></Style>',
            '<Style name="s"><ContourLine width="1e9"/></Style>',
            '<Legend name="l" type="colorRange"><palette index="2000000000" red="0" green="0" blue="0"/></Legend>',
            '<Legend name="l" type="interval"><palette min="0" max="2000000000" red="0" green="0" blue="0"/></Legend>',
            # Malformed values
            '<Style name="s"><ContourLine linecolor="red"/></Style>',
            '<Style name="s"><Min>abc</Min></Style>',
            '<Style name="s"><Min>nan</Min></Style>',
            '<Style name="s"><Max>inf</Max></Style>',
            '<Style name="s"><ContourLine classes="0.1,abc"/></Style>',
            f'<Style name="{"x" * 2000}"/>',
        ]:
            with self.subTest(stylesheet=stylesheet[:120]):
                self.assertEqual(self.post_getmap(stylesheet).status_code, 422)

    def test_CustomStyles_rejects_too_large_body(self):
        """The POST body is limited, it is refused before it is read completely"""
        response = self.post_getmap("<!--" + "x" * 70000 + "-->")
        self.assertEqual(response.status_code, 413)
