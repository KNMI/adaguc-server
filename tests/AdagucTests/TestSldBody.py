"""
Tests the SLD_BODY parameter of the SLD profile of WMS (OGC 05-078r4). The style of a UserStyle is ADAGUC style XML
in an AdagucSymbolizer, so each SLD is compared with the STYLESHEET that has the same XML.
"""

import base64
import os
import sys
import zlib
from importlib import reload
from urllib.parse import parse_qsl
import pytest
from fastapi.testclient import TestClient
from conftest import make_adaguc_env, update_db

sys.path.insert(0, os.path.join(os.environ["ADAGUC_PATH"], "python", "python_fastapi_server"))
import main  # pylint: disable=wrong-import-position

# The same dataset as TestCustomStyles
DATASET = "adaguc.tests.customstyles"
GET_MAP_QUERY = (
    "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&FORMAT=image/png&TRANSPARENT=FALSE"
)
GET_LEGEND_GRAPHIC_QUERY = "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetLegendGraphic&FORMAT=image/png"
TESTRESULTSPATH = "testresults/TestSldBody/"

# A style that differs from the configured one, as in TestCustomStyles
ADAGUC_STYLE = """
  <Legend name="sld_bluegreen" type="colorRange">
    <palette index="0" red="0" green="0" blue="128"/>
    <palette index="120" red="0" green="160" blue="255"/>
    <palette index="240" red="220" green="255" blue="200"/>
  </Legend>
  <Style name="ignored">
    <Legend fixedclasses="true" tickinterval="0.1" tickround=".01">sld_bluegreen</Legend>
    <Min>0.0</Min>
    <Max>0.5</Max>
    <ContourLine width="2" linecolor="#FFFF00" textcolor="#FFFFFF" textformatting="%2.2f" classes="0.05"/>
  </Style>
"""
OTHER_ADAGUC_STYLE = '<Style name="other"><Legend>rainbow</Legend><Min>0</Min><Max>0.3</Max></Style>'


def user_style(adaguc_style=ADAGUC_STYLE, name="my_style", is_default=True):
    return f"""<UserStyle>
      <se:Name>{name}</se:Name>
      {"<IsDefault>1</IsDefault>" if is_default else ""}
      <se:CoverageStyle>
        <se:Rule>
          <AdagucSymbolizer xmlns="https://github.com/KNMI/adaguc-server">{adaguc_style}</AdagucSymbolizer>
        </se:Rule>
      </se:CoverageStyle>
    </UserStyle>"""


def sld(*styles, layer="testdata"):
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<StyledLayerDescriptor version="1.1.0" xmlns="http://www.opengis.net/sld" xmlns:se="http://www.opengis.net/se">
  <NamedLayer>
    <se:Name>{layer}</se:Name>
    {"".join(styles)}
  </NamedLayer>
</StyledLayerDescriptor>"""


@pytest.fixture(name="client")
def fixture_client(monkeypatch):
    """FastAPI test client for the customstyles dataset, with STYLESHEET and SLD_BODY enabled"""
    env = make_adaguc_env(DATASET, TESTRESULTSPATH, TESTRESULTSPATH)
    update_db(env)
    monkeypatch.setenv("ADAGUC_CONFIG", env["ADAGUC_CONFIG"])
    monkeypatch.setenv("ADAGUC_STYLESHEET_ENABLE", "TRUE")
    reload(main)
    return TestClient(main.app)


def compress(xml):
    """Raw deflate compressed and base64url encoded, as CompressionStream("deflate-raw") in a browser"""
    compressor = zlib.compressobj(9, zlib.DEFLATED, -15)
    return base64.urlsafe_b64encode(compressor.compress(xml.encode()) + compressor.flush()).decode().rstrip("=")


def get(client, query=GET_MAP_QUERY, **params):
    """A request with the given parameters, None leaves a parameter out. SLD_VERSION is added with SLD_BODY."""
    if "SLD_BODY" in params:
        params.setdefault("SLD_VERSION", "1.1.0")
    return client.get("/wms", params={**dict(parse_qsl(query)), **{k: v for k, v in params.items() if v is not None}})


def getmap(client, **params):
    return get(client, **{"LAYERS": "testdata", "STYLES": "", **params})


def assert_same_image(response, expected):
    assert response.status_code == 200, response.text
    assert expected.status_code == 200, expected.text
    assert response.headers["content-type"] == "image/png"
    assert response.content == expected.content


def test_SldBody_gives_the_same_image_as_stylesheet(client):
    expected = getmap(client, STYLESHEET=ADAGUC_STYLE)
    assert expected.content != getmap(client).content
    assert_same_image(getmap(client, SLD_BODY=sld(user_style())), expected)


def test_SldBody_getlegendgraphic(client):
    expected = get(client, GET_LEGEND_GRAPHIC_QUERY, LAYER="testdata", STYLESHEET=ADAGUC_STYLE)
    assert_same_image(get(client, GET_LEGEND_GRAPHIC_QUERY, LAYER="testdata", SLD_BODY=sld(user_style())), expected)


def test_SldBody_compressed(client):
    """SLD_BODY is encoded like STYLESHEET: XML, or base64 encoded XML, optionally compressed"""
    assert_same_image(getmap(client, SLD_BODY=compress(sld(user_style()))), getmap(client, STYLESHEET=ADAGUC_STYLE))


def test_SldBody_literal_mode_without_layers(client):
    """Without LAYERS, the layer and its style come from the SLD (05-078r4 9.2.2), also without IsDefault"""
    expected = getmap(client, STYLESHEET=ADAGUC_STYLE)
    assert_same_image(getmap(client, LAYERS=None, STYLES=None, SLD_BODY=sld(user_style(is_default=False))), expected)
    assert_same_image(
        get(client, GET_LEGEND_GRAPHIC_QUERY, SLD_BODY=sld(user_style())),
        get(client, GET_LEGEND_GRAPHIC_QUERY, LAYER="testdata", STYLESHEET=ADAGUC_STYLE),
    )


def test_SldBody_literal_mode_without_style_uses_the_default_style(client):
    assert_same_image(getmap(client, LAYERS=None, STYLES=None, SLD_BODY=sld()), getmap(client))


def test_SldBody_library_mode_selects_the_style(client):
    """With LAYERS, STYLES names a UserStyle, and without STYLES the one with IsDefault is used (05-078r4 9.2.2)"""
    two_styles = sld(user_style(OTHER_ADAGUC_STYLE, "other", is_default=False), user_style())
    assert_same_image(getmap(client, SLD_BODY=two_styles), getmap(client, STYLESHEET=ADAGUC_STYLE))
    assert_same_image(getmap(client, SLD_BODY=two_styles, STYLES="other"), getmap(client, STYLESHEET=OTHER_ADAGUC_STYLE))


@pytest.mark.parametrize(
    "sld_body, styles",
    [
        (sld(user_style(), layer="other_layer"), ""),
        (sld(user_style()), "testdata_style_manycontours"),
        (sld(user_style(is_default=False)), ""),
    ],
    ids=["other layer", "other style", "no default"],
)
def test_SldBody_library_mode_without_style_uses_the_server_style(client, sld_body, styles):
    """Without a NamedLayer for the layer, or a UserStyle for STYLES, the server's style is used"""
    assert_same_image(getmap(client, SLD_BODY=sld_body, STYLES=styles), getmap(client, STYLES=styles))


def test_SldBody_named_style_is_the_configured_style(client):
    named_style = sld("<NamedStyle><se:Name>testdata_style_manycontours</se:Name></NamedStyle>")
    assert_same_image(getmap(client, LAYERS=None, STYLES=None, SLD_BODY=named_style), getmap(client, STYLES="testdata_style_manycontours"))


def test_SldBody_capabilities_advertise_user_styles(client, monkeypatch):
    capabilities = get(client, "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities")
    assert (
        '<sld:UserDefinedSymbolization xmlns:sld="http://www.opengis.net/sld" SupportSLD="1" UserLayer="0" UserStyle="1"'
        in capabilities.text
    )
    monkeypatch.delenv("ADAGUC_STYLESHEET_ENABLE")
    assert "UserDefinedSymbolization" not in get(client, "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities").text


@pytest.mark.parametrize(
    "params, message",
    [
        ({"SLD_VERSION": None}, "SLD_BODY needs SLD_VERSION=1.1.0"),
        ({"SLD_VERSION": "1.0.0"}, "SLD_BODY needs SLD_VERSION=1.1.0"),
        ({"SLD_BODY": sld(user_style()).replace('version="1.1.0"', 'version="1.0.0"')}, "StyledLayerDescriptor version must be 1.1.0"),
        ({"SLD_BODY": sld(user_style()).replace('xmlns="http://www.opengis.net/sld" ', "")}, "root element must be StyledLayerDescriptor"),
        ({"SLD_BODY": sld(user_style()).replace("NamedLayer", "UserLayer")}, "[UserLayer] is not supported"),
        ({"SLD_BODY": sld(user_style()).replace("AdagucSymbolizer", "RasterSymbolizer")}, "[RasterSymbolizer] in Rule is not supported"),
        (
            {"SLD_BODY": sld(user_style()).replace(' xmlns="https://github.com/KNMI/adaguc-server"', "")},
            "[AdagucSymbolizer] in Rule is not supported",
        ),
        (
            {"SLD_BODY": sld(user_style()).replace("<se:Rule>", "<se:Rule><se:MaxScaleDenominator>1000</se:MaxScaleDenominator>")},
            "[MaxScaleDenominator] in Rule is not supported",
        ),
        ({"SLD_BODY": sld(user_style(ADAGUC_STYLE + ADAGUC_STYLE))}, "Invalid SLD_BODY"),
        ({"SLD_BODY": sld(user_style('<Layer type="database"><FilePath>/etc</FilePath></Layer>'))}, "Invalid SLD_BODY"),
        ({"SLD_BODY": '<!DOCTYPE x [<!ENTITY e "x">]><StyledLayerDescriptor/>'}, "DOCTYPE is not allowed"),
        ({"SLD": "http://example.com/style.sld"}, "SLD with a URL is not supported"),
        ({"STYLESHEET": ADAGUC_STYLE}, "STYLESHEET and SLD_BODY can not be used together"),
        ({"LAYERS": "testdata,testdata"}, "SLD_BODY can only be used with one layer"),
        (
            {
                "LAYERS": None,
                "SLD_BODY": sld(user_style()).replace(
                    "</StyledLayerDescriptor>", "<NamedLayer><se:Name>x</se:Name></NamedLayer></StyledLayerDescriptor>"
                ),
            },
            "needs exactly one NamedLayer",
        ),
    ],
    ids=[
        "no version",
        "version 1.0.0",
        "sld 1.0.0",
        "no namespace",
        "user layer",
        "raster symbolizer",
        "symbolizer namespace",
        "scale",
        "two styles",
        "layer",
        "doctype",
        "sld url",
        "both",
        "two layers",
        "literal two named layers",
    ],
)
def test_SldBody_refused(client, params, message):
    response = getmap(client, **{"SLD_BODY": sld(user_style()), **params})
    assert response.status_code == 422
    assert message in response.text


def test_SldBody_disabled(client, monkeypatch):
    monkeypatch.delenv("ADAGUC_STYLESHEET_ENABLE")
    response = getmap(client, SLD_BODY=sld(user_style()))
    assert response.status_code == 422
    assert "SLD_BODY is not enabled" in response.text
