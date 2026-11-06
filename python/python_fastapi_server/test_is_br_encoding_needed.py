import pytest
from routers.is_br_encoding_needed import requires_encoding


@pytest.mark.parametrize(
    "content_type",
    [
        "application/json",
        "application/json; charset=utf-8",
        "Application/JSON",
        "application/prs.coverage+json",
        "application/geo+json",
        "application/vnd.ogc.wms_xml+xml",
        "text/xml",
        "text/html; charset=utf-8",
        "text/plain",
        "application/javascript",
        "application/vnd.oai.openapi;version=3.0",
        "application/vnd.ogc.gml",
    ],
)
def test_requires_encoding(content_type):
    assert requires_encoding({"Content-Type": content_type})


@pytest.mark.parametrize(
    "content_type",
    [
        "image/png",
        "Image/PNG",
        "image/webp",
        "image/png32",
        "application/netcdf",
        "application/x-netcdf",
        "application/octet-stream",
        "application/zip",
        "",
    ],
)
def test_requires_no_encoding(content_type):
    assert not requires_encoding({"Content-Type": content_type})


def test_requires_no_encoding_without_content_type():
    assert not requires_encoding({"Content-Length": "100"})
