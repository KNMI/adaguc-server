"""
Tests custom styles posted with the STYLESHEET parameter (form url encoded POST through the FastAPI server).
The posted Style is used for the requested layer (only one allowed), regardless of its name and of STYLES, for that request only.
A posted Legend replaces the configured one with the same name.
"""

import os
import sys
from importlib import reload
from urllib.parse import parse_qsl
import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from fastapi.testclient import TestClient
from conftest import make_adaguc_env, update_db

ADAGUC_PATH = os.environ["ADAGUC_PATH"]
sys.path.insert(0, os.path.join(ADAGUC_PATH, "python", "python_fastapi_server"))
import main  # pylint: disable=wrong-import-position

DATASET = "adaguc.tests.customstyles"
GET_MAP_QUERY = "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata_style_manycontours&FORMAT=image/png&TRANSPARENT=FALSE"
GET_LEGEND_GRAPHIC_QUERY = "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetLegendGraphic&LAYER=testdata&FORMAT=image/png"

TESTRESULTSPATH = "testresults/TestCustomStyles/"
EXPECTEDOUTPUTSPATH = "expectedoutputs/TestCustomStyles/"
AdagucTestTools().mkdir_p(TESTRESULTSPATH)

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


@pytest.fixture(name="client")
def fixture_client(monkeypatch):
    """FastAPI test client for the customstyles dataset, with STYLESHEET enabled"""
    env = make_adaguc_env(DATASET, TESTRESULTSPATH, EXPECTEDOUTPUTSPATH)
    update_db(env)
    monkeypatch.setenv("ADAGUC_CONFIG", env["ADAGUC_CONFIG"])
    monkeypatch.setenv("ADAGUC_STYLESHEET_ENABLE", "TRUE")
    reload(main)
    return TestClient(main.app)


def post_getmap(client, stylesheet=None, styles=None, layers=None):
    params = dict(parse_qsl(GET_MAP_QUERY))
    if styles is not None:
        params["STYLES"] = styles
    if layers is not None:
        params["LAYERS"] = layers
    if stylesheet is not None:
        params["STYLESHEET"] = stylesheet
    return client.post("/wms", data=params)


def post_getlegendgraphic(client, stylesheet):
    params = dict(parse_qsl(GET_LEGEND_GRAPHIC_QUERY))
    params["STYLESHEET"] = stylesheet
    return client.post("/wms", data=params)


def assert_image(response, filename):
    assert response.status_code == 200, response.text
    assert response.headers["content-type"] == "image/png"
    AdagucTestTools().writetofile(TESTRESULTSPATH + filename, response.content)
    assert AdagucTestTools().compareImage(EXPECTEDOUTPUTSPATH + filename, TESTRESULTSPATH + filename)


def contour_style(textformatting):
    return f'<Style name="s"><Legend>rainbow</Legend><Min>0</Min><Max>0.5</Max><ContourLine linecolor="#000000" textcolor="#000000" textformatting="{textformatting}" interval="0.05"/></Style>'


def short_id(value):
    return str(value)[:80]


def test_CustomStyles_dataset_style(client):
    """Reference: the style as configured in the dataset, without STYLESHEET"""
    assert_image(post_getmap(client), "test_CustomStyles_dataset_style.png")


def test_CustomStyles_post_same_style(client):
    """Posting the configured style again gives the same image as the dataset style"""
    assert_image(post_getmap(client, SAME_STYLE), "test_CustomStyles_dataset_style.png")


def test_CustomStyles_post_variant_style(client):
    """Posting a style with other colours and contour lines overrides the configured one"""
    response = post_getmap(client, VARIANT_STYLE)
    assert_image(response, "test_CustomStyles_post_variant_style.png")
    assert response.content != post_getmap(client).content


@pytest.mark.parametrize("styles", ["", "default", "testdata_style_manycontours/nearest", "does_not_exist"])
def test_CustomStyles_post_variant_style_ignores_styles(client, styles):
    """The posted style is used whatever STYLES is set to"""
    assert_image(post_getmap(client, VARIANT_STYLE, styles=styles), "test_CustomStyles_post_variant_style.png")


@pytest.mark.parametrize("value", [None, "FALSE", "true"])
def test_CustomStyles_disabled_by_default(client, monkeypatch, value):
    """STYLESHEET is refused unless ADAGUC_STYLESHEET_ENABLE is TRUE"""
    monkeypatch.delenv("ADAGUC_STYLESHEET_ENABLE", raising=False)
    if value is not None:
        monkeypatch.setenv("ADAGUC_STYLESHEET_ENABLE", value)
    response = post_getmap(client, VARIANT_STYLE)
    assert response.status_code == 422
    assert "STYLESHEET is not enabled" in response.text


def test_CustomStyles_variant_is_per_request(client):
    """A posted style only applies to its own request"""
    post_getmap(client, VARIANT_STYLE)
    assert_image(post_getmap(client), "test_CustomStyles_dataset_style.png")


def test_CustomStyles_rejects_layer(client):
    """Only Style and Legend elements may be posted"""
    response = post_getmap(client, '<Layer type="database"><FilePath>/etc</FilePath><Variable>x</Variable></Layer>')
    assert response.status_code == 422


def test_CustomStyles_rejects_file_attribute(client):
    """Attributes that make the server read a local file are refused"""
    response = post_getmap(client, '<Legend name="no2" file="/etc/passwd"/>')
    assert response.status_code == 422


def test_CustomStyles_rejects_multiple_layers(client):
    """A STYLESHEET applies to the one requested layer, more layers are refused"""
    response = post_getmap(client, VARIANT_STYLE, layers="testdata,testdata")
    assert response.status_code == 422


def test_CustomStyles_rejects_multiple_styles(client):
    """Only one Style can be posted, it is used for every layer"""
    response = post_getmap(client, SAME_STYLE + SAME_STYLE)
    assert response.status_code == 422


def test_CustomStyles_rejects_invalid_xml(client):
    response = post_getmap(client, "<Style name=")
    assert response.status_code == 422


def test_CustomStyles_symbol_from_symbol_dir(client):
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
    assert_image(post_getmap(client, symbol_style), "test_CustomStyles_symbol_from_symbol_dir.png")


def test_CustomStyles_symbol_from_dataset_dir(client):
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
    assert_image(post_getmap(client, symbol_style), "test_CustomStyles_symbol_from_dataset_dir.png")


def test_CustomStyles_symbol_in_dataset_dir_root(client):
    """A .png directly in ADAGUC_DATASET_DIR is accepted as well"""
    response = post_getmap(client, '<Style name="s"><SymbolInterval file="{ADAGUC_DATASET_DIR}/csv_radiusandvalue_legend.png"/><Point pointstyle="symbol"/></Style>')
    assert response.status_code == 200, response.text
    assert response.headers["content-type"] == "image/png"


@pytest.mark.parametrize(
    "file",
    [
        "/etc/passwd",
        "{ADAGUC_PATH}data/symbols/../../../../../etc/passwd",
        "{ADAGUC_PATH}data/resources/locations/global_edr_locations.geojson",
        "{ADAGUC_PATH}data/symbols/does_not_exist.png",
        "{ADAGUC_DATASET_DIR}/adaguc.tests.customstyles.xml",
        "{ADAGUC_DATASET_DIR}/../adaguc.tests.dataset.xml",
    ],
)
def test_CustomStyles_rejects_symbol_outside_symbol_dirs(client, file):
    """Symbol files outside the symbol directories, or not .png, are refused"""
    response = post_getmap(client, f'<Style name="s"><SymbolInterval file="{file}"/><Point pointstyle="symbol"/></Style>')
    assert response.status_code == 422, file


def test_CustomStyles_rejects_file_attribute_outside_symbolinterval(client):
    response = post_getmap(client, '<Style name="s"><Point file="{ADAGUC_PATH}data/symbols/marker.png"/></Style>')
    assert response.status_code == 422


def test_CustomStyles_legendgraphic_from_symbol_dir(client):
    """A LegendGraphic may use a .png from the server's symbol directories"""
    response = post_getmap(client, '<Style name="s"><LegendGraphic value="{ADAGUC_PATH}data/symbols/marker.png"/></Style>')
    assert response.status_code == 200, response.text


@pytest.mark.parametrize("value", ["/etc/passwd", "{ADAGUC_PATH}data/symbols/../../../../../etc/passwd", "/"])
def test_CustomStyles_rejects_legendgraphic_outside_symbol_dirs(client, value):
    """A LegendGraphic is read as a local file, so the same rules as for symbols apply"""
    response = post_getmap(client, f'<Style name="s"><LegendGraphic value="{value}"/></Style>')
    assert response.status_code == 422, value


def test_CustomStyles_large_stylesheet(client):
    """A stylesheet larger than the old 8192 byte fork server limit is accepted"""
    intervals = "".join(f'<ShadeInterval min="{i / 1000}" max="{(i + 1) / 1000}" fillcolor="#{i % 256:02x}0000"/>' for i in range(200))
    stylesheet = f'<Style name="large"><Legend>rainbow</Legend><Min>0</Min><Max>0.5</Max><RenderMethod>shaded</RenderMethod>{intervals}</Style>'
    assert len(stylesheet) > 8192
    response = post_getmap(client, stylesheet)
    assert response.status_code == 200, response.text


@pytest.mark.parametrize("fmt", ["%s", "%s%s%s%s", "%p %p", "%n", "%d", "%x", "%*f", "%1$f", "%Lf", "%f %f", "%123f", "%"])
@pytest.mark.parametrize(
    "template",
    [
        "contour",
        '<Legend name="l" type="colorRange" textformatting="{fmt}"/>',
        '<Style name="s"><Legend textformatting="{fmt}">rainbow</Legend></Style>',
        '<Style name="s"><Point textformat="{fmt}"/></Style>',
        '<Style name="s"><Vector textformat="{fmt}"/></Style>',
    ],
)
def test_CustomStyles_accepts_unsafe_formats(client, fmt, template):
    """printf formats get a double: CT::formatFloat prints anything but one float conversion with %g instead"""
    stylesheet = contour_style(fmt) if template == "contour" else template.format(fmt=fmt)
    response = post_getmap(client, stylesheet)
    assert response.status_code == 200, response.text


def test_CustomStyles_rejects_long_formats(client):
    assert post_getmap(client, contour_style("%f" * 33)).status_code == 422


@pytest.mark.parametrize("fmt", ["%2.2f", "%.1f degC", "%g", "%e", "%+08.3f", "100%%", " ", ""])
def test_CustomStyles_accepts_safe_formats(client, fmt):
    response = post_getmap(client, contour_style(fmt))
    assert response.status_code == 200, response.text


@pytest.mark.parametrize(
    "stylesheet",
    [
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
        # IncludeStyle of a style that is not configured, like the posted style itself
        '<Style name="s"><IncludeStyle name="s"/></Style>',
        '<Style name="s"><IncludeStyle name="does_not_exist"/></Style>',
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
        # Values read with atoi must be whole numbers: 20000e-3 is 20 as a number, but atoi reads 20000
        '<Style name="s"><SmoothingFilter>20000e-3</SmoothingFilter></Style>',
        '<Style name="s"><Stippling distancex="100000e-3"/></Style>',
        '<Style name="s"><RenderSettings striding="0.5"/></Style>',
        '<Legend name="l" type="colorRange"><palette index="100000e-3" red="0" green="0" blue="0"/></Legend>',
        # Malformed values
        '<Style name="s"><ContourLine linecolor="red"/></Style>',
        '<Style name="s"><Min>abc</Min></Style>',
        '<Style name="s"><Min>nan</Min></Style>',
        '<Style name="s"><Max>inf</Max></Style>',
        '<Style name="s"><ContourLine classes="0.1,abc"/></Style>',
        f'<Style name="{"x" * 2000}"/>',
        '<Style name="s"><ShadeInterval>inf</ShadeInterval></Style>',
        '<Style name="s"><ShadeInterval>-1</ShadeInterval></Style>',
        '<Style name="s"><Log>-10</Log></Style>',
        # drawContour has one bit per ContourLine
        '<Style name="s">' + '<ContourLine interval="0.1"/>' * 33 + "</Style>",
        # A legend that is neither configured nor posted
        '<Style name="s"><Legend>does_not_exist</Legend></Style>',
        # Only the text before a comment is read as value, so it must be the text that is checked
        '<Style name="s"><SmoothingFilter>100000<!---->e-4</SmoothingFilter></Style>',
        '<Style name="s"><ContourIntervalL>-1<!---->e-999</ContourIntervalL></Style>',
        # Every IncludeStyle adds the included elements again
        '<Style name="s">' + '<IncludeStyle name="testdata_style_manycontours"/>' * 5 + "</Style>",
    ],
    ids=short_id,
)
def test_CustomStyles_rejects_disallowed_content(client, stylesheet):
    """Only allowlisted elements and attributes with valid values may be posted"""
    assert post_getmap(client, stylesheet).status_code == 422


@pytest.mark.parametrize(
    "stylesheet",
    [
        '<Style name="s"><SmoothingFilter> 20 </SmoothingFilter></Style>',
        '<Style name="s"><Stippling distancex="+10" distancey="10" discradius="2"/></Style>',
        '<Legend name="l" type="colorRange"><palette index="0" red="0" green="0" blue="0" alpha="-1"/></Legend>',
    ],
    ids=short_id,
)
def test_CustomStyles_accepts_whole_numbers(client, stylesheet):
    """Values read with atoi accept whole numbers with an optional sign and surrounding whitespace"""
    response = post_getmap(client, stylesheet)
    assert response.status_code == 200, response.text


def test_CustomStyles_rejects_too_large_body(client):
    """The POST body is limited, it is refused before it is read completely"""
    response = post_getmap(client, "<!--" + "x" * 70000 + "-->")
    assert response.status_code == 413


def test_CustomStyles_rejects_too_large_query_string(client):
    """The forwarded query string is encoded again: a space in the body becomes three bytes"""
    response = post_getmap(client, "<!--" + " " * 30000 + "-->")
    assert response.status_code == 413


@pytest.mark.parametrize(
    "point",
    [
        '<Point textradius="1000" maxpointspercell="1" maxpointcellsize="1000"/>',
        '<Point maxpointcellsize="0" maxpointspercell="1"/>',
    ],
)
def test_CustomStyles_point_cells_stay_in_bounds(client, point):
    """The point cell matrix is indexed at the text position, which can be far outside the image"""
    response = post_getmap(client, f'<Style name="s"><DataPostProc algorithm="pointsfromgrid" a="40" select="testdata"/>{point}</Style>')
    assert response.status_code == 200, response.text
    assert response.headers["content-type"] == "image/png"


def test_CustomStyles_accepts_32_contourlines(client):
    response = post_getmap(client, '<Style name="s"><Legend>rainbow</Legend>' + '<ContourLine interval="0.1"/>' * 32 + "</Style>")
    assert response.status_code == 200, response.text


def test_CustomStyles_layer_settings_do_not_override_posted_style(client):
    """Style settings in the layer (RenderMethod, Legend, Min, Max, ContourLine) apply to configured styles only"""
    assert post_getmap(client, layers="testdata_layer_overrides").content != post_getmap(client).content
    assert_image(post_getmap(client, VARIANT_STYLE, layers="testdata_layer_overrides"), "test_CustomStyles_post_variant_style.png")


def test_CustomStyles_legend_with_many_classes(client):
    """A discrete legend with thousands of classes draws the first ones, like for a configured style"""
    response = post_getlegendgraphic(
        client,
        '<Style name="s"><Legend fixedclasses="true">rainbow</Legend><RenderMethod>shaded</RenderMethod><Min>0</Min><Max>0.5</Max><ShadeInterval>0.0001</ShadeInterval></Style>',
    )
    assert response.status_code == 200, response.text
    assert response.headers["content-type"] == "image/png"


@pytest.mark.parametrize(
    "stylesheet",
    [
        # 1e9f + 1 == 1e9f, the class loop would not advance
        '<Style name="s"><Legend fixedclasses="true">rainbow</Legend><RenderMethod>shaded</RenderMethod><Min>1e9</Min><Max>1000000240</Max><ContourIntervalL>1</ContourIntervalL></Style>',
        # The smallest value of the log legend underflows to 0
        '<Style name="s"><Legend>rainbow</Legend><Log>10</Log><Scale>1</Scale><Offset>1000</Offset></Style>',
    ],
    ids=short_id,
)
def test_CustomStyles_legend_loops_end(client, stylesheet):
    """Values that used to make the legend loops endless give an error instead of a timeout"""
    response = post_getlegendgraphic(client, stylesheet)
    assert response.status_code == 200, response.text
    assert "GetLegendGraphic Request failed" in response.text


@pytest.mark.parametrize("name", ["included", "testdata_style_manycontours"])
def test_CustomStyles_include_configured_style(client, name):
    """A posted style can include a configured style, also when it has the same name"""
    response = post_getmap(client, f'<Style name="{name}"><IncludeStyle name="testdata_style_manycontours"/></Style>')
    assert_image(response, "test_CustomStyles_dataset_style.png")
