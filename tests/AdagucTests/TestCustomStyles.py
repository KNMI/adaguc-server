"""
Tests custom styles posted as JSON with the STYLESHEET parameter (form url encoded POST through the FastAPI server).
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
GET_FEATURE_INFO_QUERY = "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetFeatureInfo&LAYERS=testdata&QUERY_LAYERS=testdata&CRS=EPSG%3A4326&BBOX=30,-30,75,30&WIDTH=256&HEIGHT=256&I=128&J=128&INFO_FORMAT=text/plain"

TESTRESULTSPATH = "testresults/TestCustomStyles/"
EXPECTEDOUTPUTSPATH = "expectedoutputs/TestCustomStyles/"
AdagucTestTools().mkdir_p(TESTRESULTSPATH)

# Same as the dataset's style, without StandardNames
SAME_STYLE = """
  {"Style": {"@name": "testdata_style_manycontours", "@title": "Rainbow colors", "@abstract": "Drawing with rainbow colors",
    "Legend": {"@fixedclasses": "true", "@tickinterval": "0.1", "@tickround": ".01", "#text": "no2"},
    "Min": "0.0",
    "Max": "0.5",
    "ContourLine": [{"@width": "0.8", "@linecolor": "#880000", "@textcolor": "#000000", "@textformatting": "%2.2f", "@classes": "0.10"},
                    {"@width": "0.8", "@linecolor": "#000000", "@textcolor": "#000000", "@textformatting": "%2.2f", "@classes": "0.01"}]
  }}
"""

# Other colours and contour intervals, with its own Legend. The name differs from the dataset's style on purpose.
VARIANT_STYLE = """
  {"Legend": {"@name": "customstyles_bluegreen", "@type": "colorRange",
    "palette": [{"@index": 0, "@red": 0, "@green": 0, "@blue": 128},
                {"@index": 120, "@red": 0, "@green": 160, "@blue": 255},
                {"@index": 240, "@red": 220, "@green": 255, "@blue": 200}]},
  "Style": {"@name": "my_custom_style", "@title": "Blue green", "@abstract": "Drawing with blue and green colors",
    "Legend": {"@fixedclasses": "true", "@tickinterval": "0.1", "@tickround": ".01", "#text": "customstyles_bluegreen"},
    "Min": "0.0",
    "Max": "0.5",
    "ContourLine": [{"@width": "2", "@linecolor": "#FFFF00", "@textcolor": "#FFFFFF", "@textformatting": "%2.2f", "@classes": "0.05"},
                    {"@width": "0.5", "@linecolor": "#00FF00", "@textcolor": "#FFFFFF", "@textformatting": "%2.3f", "@classes": "0.025"}]
  }}
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


def assert_image(response, filename, expected=None):
    assert response.status_code == 200, response.text
    assert response.headers["content-type"] == "image/png"
    AdagucTestTools().writetofile(TESTRESULTSPATH + filename, response.content)
    assert AdagucTestTools().compareImage(expected or EXPECTEDOUTPUTSPATH + filename, TESTRESULTSPATH + filename)


def contour_style(textformatting):
    return f'{{"Style": {{"@name": "s", "Legend": "rainbow", "Min": 0, "Max": 0.5, "ContourLine": {{"@linecolor": "#000000", "@textcolor": "#000000", "@textformatting": "{textformatting}", "@interval": 0.05}}}}}}'


def short_id(value):
    return str(value)[:80]


def test_CustomStyles_post_variant_style(client):
    """A posted style overrides the configured one for its own request only: the requests before and after it use the dataset style"""
    assert_image(post_getmap(client), "test_CustomStyles_dataset_style.png")
    assert_image(post_getmap(client, VARIANT_STYLE), "test_CustomStyles_post_variant_style.png")
    assert_image(post_getmap(client), "test_CustomStyles_dataset_style.png")


def test_CustomStyles_post_same_style(client):
    """Posting the configured style again gives the same image as the dataset style"""
    assert_image(post_getmap(client, SAME_STYLE), "test_CustomStyles_dataset_style.png")


@pytest.mark.parametrize("styles", [None, "", "default", "testdata_style_manycontours/nearest", "does_not_exist"])
def test_CustomStyles_post_variant_style_ignores_styles(client, styles):
    """The posted style is used whatever STYLES is set to, None keeps the configured style testdata_style_manycontours"""
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


def test_CustomStyles_rejects_layer(client):
    """A refused STYLESHEET gives 422. What is refused is tested in C++, see the CServerParams applyStyleSheet tests in testadagucserver.cpp"""
    response = post_getmap(client, '{"Layer": {"@type": "database", "FilePath": "/etc", "Variable": "x"}}')
    assert response.status_code == 422


def test_CustomStyles_rejects_xml(client):
    """A STYLESHEET is JSON only"""
    assert post_getmap(client, '<Style name="s"><Legend>rainbow</Legend></Style>').status_code == 422


def test_CustomStyles_rejects_multiple_layers(client):
    """A STYLESHEET applies to the one requested layer, more layers are refused"""
    response = post_getmap(client, VARIANT_STYLE, layers="testdata,testdata")
    assert response.status_code == 422


def test_CustomStyles_symbol_from_symbol_dir(client):
    """A SymbolInterval may use a .png from the server's symbol directories"""
    symbol_style = """
      {"Style": {"@name": "symbols",
        "Legend": "rainbow",
        "Min": "0.0",
        "Max": "0.5",
        "DataPostProc": {"@algorithm": "pointsfromgrid", "@a": "40", "@select": "testdata"},
        "SymbolInterval": {"@file": "{ADAGUC_PATH}data/resources/icons/helicopter.png"},
        "Point": {"@pointstyle": "symbol", "@discradius": "0", "@textradius": "0", "@dot": "false", "@fontsize": "8", "@textformat": " "}
      }}
    """
    assert_image(post_getmap(client, symbol_style), "test_CustomStyles_symbol_from_symbol_dir.png")


def test_CustomStyles_symbol_from_dataset_dir(client):
    """A SymbolInterval may use a .png from ADAGUC_DATASET_DIR, including its subdirectories"""
    symbol_style = """
      {"Style": {"@name": "symbols",
        "Legend": "rainbow",
        "Min": "0.0",
        "Max": "0.5",
        "DataPostProc": {"@algorithm": "pointsfromgrid", "@a": "40", "@select": "testdata"},
        "SymbolInterval": {"@file": "{ADAGUC_DATASET_DIR}/resources/symbols/okta/32px-Cloud_cover_3.svg.png"},
        "Point": {"@pointstyle": "symbol", "@discradius": "0", "@textradius": "0", "@dot": "false", "@fontsize": "8", "@textformat": " "}
      }}
    """
    assert_image(post_getmap(client, symbol_style), "test_CustomStyles_symbol_from_dataset_dir.png")


def test_CustomStyles_large_stylesheet(client):
    """A stylesheet larger than the old 8192 byte fork server limit is accepted"""
    intervals = ", ".join(f'{{"@min": "{i / 1000}", "@max": "{(i + 1) / 1000}", "@fillcolor": "#{i % 256:02x}0000"}}' for i in range(200))
    stylesheet = f'{{"Style": {{"@name": "large", "Legend": "rainbow", "Min": 0, "Max": 0.5, "RenderMethod": "shaded", "ShadeInterval": [{intervals}]}}}}'
    assert len(stylesheet) > 8192
    response = post_getmap(client, stylesheet)
    assert response.status_code == 200, response.text


def post_vector_getmap(client, monkeypatch, vector):
    """GetMap of wind_wave_direction, drawn with the posted Vector from directions and a constant magnitude of 1"""
    env = make_adaguc_env("adaguc.tests.vectorrendering")
    update_db(env)
    monkeypatch.setenv("ADAGUC_CONFIG", env["ADAGUC_CONFIG"])
    params = {
        "SERVICE": "WMS",
        "VERSION": "1.3.0",
        "REQUEST": "GetMap",
        "LAYERS": "wind_wave_direction",
        "WIDTH": "1151",
        "HEIGHT": "1135",
        "CRS": "EPSG:3857",
        "BBOX": "-61120.32818755589,6253361.75474204,1285634.726913556,7581395.62749596",
        "STYLES": "",
        "FORMAT": "image/png",
        "TRANSPARENT": "TRUE",
        "time": "2026-01-27T03:00:00Z",
        "DIM_reference_time": "2026-01-26T06:00:00Z",
        "STYLESHEET": f"""
          {{"Style": {{"@name": "s",
            "DataPostProc": [{{"@algorithm": "add_dataobject", "@name": "dummy-magnitude", "@a": "1"}},
                             {{"@algorithm": "pointsfromgrid", "@select": "dummy-magnitude,wind-wave-from-direction-mean-msl", "@a": "50"}}],
            "RenderSettings": {{"@drawgrid": "false"}},
            "Vector": {vector}
          }}}}
        """,
    }
    return client.post("/wms", data=params)


# One case per kind of text that is printed with a posted format
FORMAT_CASES = {
    "contour": lambda client, monkeypatch, fmt: post_getmap(client, contour_style(fmt)),
    "continuous_legend": lambda client, monkeypatch, fmt: post_getlegendgraphic(
        client, f'{{"Style": {{"@name": "s", "Legend": {{"@textformatting": "{fmt}", "#text": "rainbow"}}, "Min": 0, "Max": 0.5}}}}'
    ),
    "discrete_legend": lambda client, monkeypatch, fmt: post_getlegendgraphic(
        client,
        f'{{"Style": {{"@name": "s", "Legend": {{"@fixedclasses": "true", "@textformatting": "{fmt}", "#text": "rainbow"}}, "RenderMethod": "shaded", "Min": 0, "Max": 0.5, "ShadeInterval": 0.1}}}}',
    ),
    "point": lambda client, monkeypatch, fmt: post_getmap(
        client, f'{{"Style": {{"@name": "s", "Legend": "rainbow", "DataPostProc": {{"@algorithm": "pointsfromgrid", "@a": "40", "@select": "testdata"}}, "Point": {{"@textformat": "{fmt}"}}}}}}'
    ),
    "vector": lambda client, monkeypatch, fmt: post_vector_getmap(client, monkeypatch, f'{{"@vectorstyle": "disc", "@textformat": "{fmt}"}}'),
}


@pytest.mark.parametrize("case", FORMAT_CASES)
def test_CustomStyles_unsafe_format_falls_back_to_g(client, monkeypatch, case):
    """A posted format is accepted, but CT::formatFloat prints anything but one float conversion with %g instead.
    Which formats are safe is tested in C++ (CT isSafeFloatFormat), this checks that the text of every renderer goes through it."""
    render = lambda fmt: FORMAT_CASES[case](client, monkeypatch, fmt)
    unsafe, fallback, other = render("%s%s%s%s"), render("%g"), render("%.4f")
    for response in (unsafe, fallback, other):
        assert response.status_code == 200, response.text
        assert response.headers["content-type"] == "image/png", response.text
    assert unsafe.content == fallback.content
    # The format must show up in the image, otherwise the comparison above proves nothing
    assert fallback.content != other.content


@pytest.mark.parametrize(
    ("dataPostProc", "expected"),
    [
        ('{"@algorithm": "ax+b", "@a": "2", "@b": "1", "@units": "x"}', "Testdata 1.395386 x"),
        ('{"@algorithm": "convert_units", "@a": "2", "@from_units": "1", "@units": "x2"}', "Testdata 0.395386 x2"),
        # Only data in one of the from_units is converted
        ('{"@algorithm": "convert_units", "@a": "2", "@from_units": "kts", "@units": "x2"}', "Testdata 0.197693 1"),
    ],
)
def test_CustomStyles_convert_units(client, dataPostProc, expected):
    """A posted style can convert units, the value at this location is 0.197693 with units 1"""
    params = dict(parse_qsl(GET_FEATURE_INFO_QUERY))
    params["STYLESHEET"] = f'{{"Style": {{"@name": "s", "DataPostProc": {dataPostProc}}}}}'
    response = client.post("/wms", data=params)
    assert response.status_code == 200, response.text
    assert expected in response.text


@pytest.mark.parametrize(
    ("dataPostProcs", "expected"),
    [
        # pow(pow(10, 0.197693 / 10) / 200, 0.625)
        ('{"@algorithm": "dbztorr"}', ["Testdata 0.037516 mm/hr"]),
        # Only the selected data objects are in the output
        ('[{"@algorithm": "add_dataobject", "@name": "m1", "@a": "1"}, {"@algorithm": "filter_dataobjects", "@select": "testdata"}]', ["Testdata 0.197693 1"]),
        ('[{"@algorithm": "add_dataobject", "@name": "m1", "@a": "1"}, {"@algorithm": "filter_dataobjects", "@select": "m1"}]', ["m1 1.000000 1"]),
    ],
)
def test_CustomStyles_datapostproc_getfeatureinfo(client, dataPostProcs, expected):
    """Other DataPostProc algorithms without file or layer access"""
    params = dict(parse_qsl(GET_FEATURE_INFO_QUERY))
    params["STYLESHEET"] = f'{{"Style": {{"@name": "s", "DataPostProc": {dataPostProcs}}}}}'
    response = client.post("/wms", data=params)
    assert response.status_code == 200, response.text
    assert [line.strip(" -") for line in response.text.splitlines() if line.startswith("  ")] == expected


def test_CustomStyles_add_dataobject_getfeatureinfo(client):
    """Every add_dataobject adds one data object with its own value. GetFeatureInfo reads the data twice, which used to add
    a second object that was never filled: its value was uninitialized memory."""
    params = dict(parse_qsl(GET_FEATURE_INFO_QUERY))
    params["STYLESHEET"] = '{"Style": {"@name": "s", "DataPostProc": [{"@algorithm": "add_dataobject", "@name": "m1", "@a": "1"}, {"@algorithm": "add_dataobject", "@name": "m2", "@a": "2"}]}}'
    response = client.post("/wms", data=params)
    assert response.status_code == 200, response.text
    values = [line.strip(" -") for line in response.text.splitlines() if line.startswith("  ")]
    assert values == ["m2 2.000000 1", "m1 1.000000 1", "Testdata 0.197693 1"]


def test_CustomStyles_add_dataobject(client, monkeypatch):
    """add_dataobject adds a constant magnitude to draw vectors from directions only, as the configured style wind_wave_direction does"""
    filename = "test_WMSGetMap_wave_direction_vector_with_add_dataobject.png"
    response = post_vector_getmap(client, monkeypatch, '{"@vectorstyle": "vector", "@linecolor": "#000000", "@linewidth": "2.0", "@scale": "40.0"}')
    assert_image(response, filename, expected=f"expectedoutputs/TestPointsAndVectors/{filename}")


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
        '{"@textradius": "1000", "@maxpointspercell": "1", "@maxpointcellsize": "1000"}',
        '{"@maxpointcellsize": "0", "@maxpointspercell": "1"}',
    ],
)
def test_CustomStyles_point_cells_stay_in_bounds(client, point):
    """The point cell matrix is indexed at the text position, which can be far outside the image"""
    response = post_getmap(client, f'{{"Style": {{"@name": "s", "DataPostProc": {{"@algorithm": "pointsfromgrid", "@a": "40", "@select": "testdata"}}, "Point": {point}}}}}')
    assert response.status_code == 200, response.text
    assert response.headers["content-type"] == "image/png"


def test_CustomStyles_accepts_32_contourlines(client):
    response = post_getmap(client, '{"Style": {"@name": "s", "Legend": "rainbow", "ContourLine": [' + ", ".join(['{"@interval": 0.1}'] * 32) + "]}}")
    assert response.status_code == 200, response.text


def test_CustomStyles_layer_settings_do_not_override_posted_style(client):
    """Style settings in the layer (RenderMethod, Legend, Min, Max, ContourLine) apply to configured styles only"""
    assert post_getmap(client, layers="testdata_layer_overrides").content != post_getmap(client).content
    assert_image(post_getmap(client, VARIANT_STYLE, layers="testdata_layer_overrides"), "test_CustomStyles_post_variant_style.png")


def test_CustomStyles_legend_with_many_classes(client):
    """A discrete legend with thousands of classes draws the first ones, like for a configured style"""
    response = post_getlegendgraphic(
        client,
        '{"Style": {"@name": "s", "Legend": {"@fixedclasses": "true", "#text": "rainbow"}, "RenderMethod": "shaded", "Min": 0, "Max": 0.5, "ShadeInterval": "0.0001"}}',
    )
    assert response.status_code == 200, response.text
    assert response.headers["content-type"] == "image/png"


@pytest.mark.parametrize(
    "stylesheet",
    [
        # 1e9f + 1 == 1e9f, the class loop would not advance
        '{"Style": {"@name": "s", "Legend": {"@fixedclasses": "true", "#text": "rainbow"}, "RenderMethod": "shaded", "Min": "1e9", "Max": "1000000240", "ContourIntervalL": "1"}}',
        # The smallest value of the log legend underflows to 0
        '{"Style": {"@name": "s", "Legend": "rainbow", "Log": "10", "Scale": "1", "Offset": "1000"}}',
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
    response = post_getmap(client, f'{{"Style": {{"@name": "{name}", "IncludeStyle": {{"@name": "testdata_style_manycontours"}}}}}}')
    assert_image(response, "test_CustomStyles_dataset_style.png")
