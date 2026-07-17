"""
Tests uvicorn with various settings for environment and headers. Used for new functionality to run behind a proxy.
"""

import os
import re
from importlib import reload
from xml.etree import ElementTree
from fastapi.testclient import TestClient
from fastapi import Response
import main


def get_testclient(environment=None):
    """Return a fresh testclient with specific environment settings"""

    # Default environment
    os.environ["EXTERNALADDRESS"] = ""
    os.environ["ADAGUC_TRUSTED_HOSTS"] = ""
    os.environ["ADAGUC_TRUSTED_PROXIES"] = ""
    os.environ["ADAGUC_CONFIG"] = os.path.join(os.environ["ADAGUC_PATH"], "data", "config", "adaguc.dataset.xml")

    # Configured environment
    if environment is not None:
        for item, value in environment.items():
            os.environ[item] = value

    # Force reload of the app, so environment variables are propagated to the middlewares
    reload(main)
    client = TestClient(
        main.app,
        client=("adaguc-testclient", 50000),
    )
    client.base_url = "http://default-test-server"
    client.client = {}
    client.headers = {}
    return client


def check_healthcheck(client, headers={}):
    response = client.get("/healthcheck", headers=headers)
    return response.status_code == 200 and response.text == "OK"


def get_url_from_capabilities(response: Response):
    """
    Parses the GetCapabilities and returns the online resource url
    """
    capabilities = ElementTree.fromstring(re.sub(b' xmlns="[^"]+"', b"", response.content, count=1))
    return capabilities.findall("Service")[0].findall("OnlineResource")[0].get("{http://www.w3.org/1999/xlink}href")


def test_getcapabilities_default():
    """
    Check default behavior
    """
    client = get_testclient()
    response = client.get("/adaguc-server?service=WMS&request=getCapabilities")
    assert get_url_from_capabilities(response) == "http://default-test-server/adaguc-server?SERVICE=WMS&"
    assert check_healthcheck(client)


def test_externaladdress_http():
    """
    Check behavior when EXTERNALADDRESS is set.
    """
    client = get_testclient(environment={"EXTERNALADDRESS": "http://my-awesome-server/with-a-prefix/"})
    response = client.get("/adaguc-server?service=WMS&request=getCapabilities")
    assert get_url_from_capabilities(response) == "http://my-awesome-server/with-a-prefix/adaguc-server?SERVICE=WMS&"
    assert check_healthcheck(client)


def test_externaladdress_http_port():
    """
    Check behavior when EXTERNALADDRESS is set with a prefix and a port
    """
    client = get_testclient(environment={"EXTERNALADDRESS": "http://my-awesome-server:1234/with-a-prefix/"})
    response = client.get("/adaguc-server?service=WMS&request=getCapabilities")
    assert get_url_from_capabilities(response) == "http://my-awesome-server:1234/with-a-prefix/adaguc-server?SERVICE=WMS&"
    assert check_healthcheck(client)


def test_externaladdress_https():
    """
    Check behavior when EXTERNALADDRESS is set with a prefix and as https
    """
    client = get_testclient(environment={"EXTERNALADDRESS": "https://my-awesome-server/with-a-prefix/"})
    response = get_testclient(environment={"EXTERNALADDRESS": "https://my-awesome-server/with-a-prefix/"}).get(
        "/adaguc-server?service=WMS&request=getCapabilities"
    )
    assert get_url_from_capabilities(response) == "https://my-awesome-server/with-a-prefix/adaguc-server?SERVICE=WMS&"
    assert check_healthcheck(client)


def test_externaladdress_with_host():
    """
    it should not take host header but use externaladdress
    """
    client = get_testclient(environment={"EXTERNALADDRESS": "http://externaladdress"})
    response = client.get("/adaguc-server?service=WMS&request=getCapabilities", headers={"Host": "my-host"})
    assert get_url_from_capabilities(response) == "http://externaladdress/adaguc-server?SERVICE=WMS&"
    assert check_healthcheck(client, headers={"Host": "my-host"})


def test_host_header_not_trusted():
    """
    TODO: Is it expected that by default the my-host is returned in the online resource?
    """
    client = get_testclient()
    response = client.get("/adaguc-server?service=WMS&request=getCapabilities", headers={"Host": "my-host"})
    assert get_url_from_capabilities(response) == "http://my-host/adaguc-server?SERVICE=WMS&"
    assert check_healthcheck(client, headers={"Host": "my-host"})


def test_x_forwarded_for_headers():
    """
    Test x-forward- headers
    """
    client = get_testclient(
        environment={
            "ADAGUC_TRUSTED_HOSTS": "supertestadagucserver",
            "ADAGUC_TRUSTED_PROXIES": "adaguc-testclient",  # default
        }
    )
    headers = {
        "Host": "my-host-to-be-ignored",
        "x-forwarded-host": "supertestadagucserver",
        "x-forwarded-prefix": "/thepathafterhost",
        "x-forwarded-port": "1234",
        "x-forwarded-proto": "https",
    }
    response = client.get(
        "/adaguc-server?service=WMS&request=getCapabilities",
        headers=headers,
    )

    assert get_url_from_capabilities(response) == "https://supertestadagucserver:1234/thepathafterhost/adaguc-server?SERVICE=WMS&"
    assert check_healthcheck(client, headers=headers)


def test_x_forwarded_for_headers_https_port():
    """
    Test x-forward- headers
    """
    client = get_testclient(
        environment={
            "EXTERNALADDRESS": "",
            "ADAGUC_TRUSTED_HOSTS": "supertestadagucserver",
            "ADAGUC_TRUSTED_PROXIES": "adaguc-testclient",  # default
        }
    )
    headers = {
        "Host": "my-host-to-be-ignored",
        "x-forwarded-host": "supertestadagucserver",
        "x-forwarded-prefix": "/thepathafterhost",
        "x-forwarded-port": "443",
        "x-forwarded-proto": "https",
    }
    response = client.get("/adaguc-server?service=WMS&request=getCapabilities", headers=headers)

    assert get_url_from_capabilities(response) == "https://supertestadagucserver/thepathafterhost/adaguc-server?SERVICE=WMS&"
    assert check_healthcheck(client, headers=headers)


def test_x_forwarded_for_headers_wildcard():
    """
    Test x-forward- headers
    """
    client = get_testclient(
        environment={
            "ADAGUC_TRUSTED_HOSTS": "*",
            "ADAGUC_TRUSTED_PROXIES": "*",  # default
        }
    )
    headers = {
        "Host": "my-host-to-be-ignored",
        "x-forwarded-host": "supertestadagucserver",
        "x-forwarded-prefix": "/thepathafterhost",
        "x-forwarded-port": "1234",
        "x-forwarded-proto": "https",
    }

    response = client.get(
        "/adaguc-server?service=WMS&request=getCapabilities",
        headers=headers,
    )

    assert get_url_from_capabilities(response) == "https://supertestadagucserver:1234/thepathafterhost/adaguc-server?SERVICE=WMS&"
    assert check_healthcheck(client, headers=headers)


def test_x_forwarded_for_empty_port():
    """
    Test x-forward- headers
    """
    client = get_testclient(
        environment={
            "ADAGUC_TRUSTED_HOSTS": "*",
            "ADAGUC_TRUSTED_PROXIES": "*",  # default
        }
    )
    headers = {
        "Host": "my-host-to-be-ignored",
        "x-forwarded-host": "supertestadagucserver",
        "x-forwarded-prefix": "/thepathafterhost",
        "x-forwarded-port": "",
        "x-forwarded-proto": "https",
    }
    response = client.get(
        "/adaguc-server?service=WMS&request=getCapabilities",
        headers=headers,
    )

    assert get_url_from_capabilities(response) == "https://supertestadagucserver/thepathafterhost/adaguc-server?SERVICE=WMS&"
    assert check_healthcheck(client, headers=headers)


def test_externaddress_not_trusted():
    """
    Test trusted hosts not trusted
    """
    client = get_testclient(
        environment={
            "EXTERNALADDRESS": "http://untrustedhost:1234/thepathafterhost/",
            "ADAGUC_TRUSTED_HOSTS": "supertestadagucserver",
        }
    )
    response = client.get(
        "/adaguc-server?service=WMS&request=getCapabilities",
        headers={},
    )

    assert response.status_code == 200
    assert get_url_from_capabilities(response) == "http://untrustedhost:1234/thepathafterhost/adaguc-server?SERVICE=WMS&"
    assert check_healthcheck(client)


def test_host_not_trusted():
    """
    Test trusted hosts not trusted
    """
    client = get_testclient(
        environment={
            "ADAGUC_TRUSTED_HOSTS": "supertestadagucserver",
        }
    )
    response = client.get(
        "/adaguc-server?service=WMS&request=getCapabilities",
        headers={"Host": "untrustedhost"},
    )

    assert response.status_code == 400


def test_externaddress_not_trusted_healthcheck():
    """
    Test trusted hosts not trusted
    """
    client = get_testclient(
        environment={
            "EXTERNALADDRESS": "http://untrustedhost:1234/thepathafterhost/",
            "ADAGUC_TRUSTED_HOSTS": "supertestadagucserver",
        }
    )
    response = client.get(
        "/healthcheck",
        headers={},
    )
    assert response.status_code == 200
    assert response.text == "OK"
    assert check_healthcheck(client)


def test_externaddress_trusted():
    """
    Test trusted hosts but is trusted
    """
    client = get_testclient(
        environment={
            "EXTERNALADDRESS": "http://untrustedhost:1234/thepathafterhost/",
            "ADAGUC_TRUSTED_PROXIES": "adaguc-testclient",
            "ADAGUC_TRUSTED_HOSTS": "untrustedhost",
        }
    )
    response = client.get(
        "/adaguc-server?service=WMS&request=getCapabilities",
        headers={},
    )

    assert response.status_code == 200
    assert get_url_from_capabilities(response) == "http://untrustedhost:1234/thepathafterhost/adaguc-server?SERVICE=WMS&"

    assert check_healthcheck(client, headers={})


def test_externaddress_wildcard():
    """
    Test trusted hosts wildcard
    """
    client = get_testclient(
        environment={
            "EXTERNALADDRESS": "http://untrustedhost:1234/thepathafterhost/",
            "ADAGUC_TRUSTED_HOSTS": "*",
        }
    )
    response = client.get(
        "/adaguc-server?service=WMS&request=getCapabilities",
        headers={},
    )

    assert response.status_code == 200
    assert get_url_from_capabilities(response) == "http://untrustedhost:1234/thepathafterhost/adaguc-server?SERVICE=WMS&"
    assert check_healthcheck(client)
