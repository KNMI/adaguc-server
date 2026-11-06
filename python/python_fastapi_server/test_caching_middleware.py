"""
Tests that the caching middleware only caches GET requests: the cache key is the url, a POST has parameters in the body.
"""

import os
from unittest.mock import patch
from fastapi import FastAPI, Request
from fastapi.responses import PlainTextResponse
from fastapi.testclient import TestClient
import pytest
from routers import caching_middleware


@pytest.fixture(name="cache")
def fixture_cache():
    """In memory replacement for redis, keyed like the real cache"""
    cache = {}

    async def get_cached_response(_redis_pool, request):
        entry = cache.get(caching_middleware.generate_key(request))
        return (0, dict(entry[0]), entry[1]) if entry else (None, None, None)

    async def response_to_cache(redis_pool, request, headers, data, ex):  # pylint: disable=unused-argument
        cache[caching_middleware.generate_key(request)] = (headers, data)

    with (
        patch.dict(os.environ, {"ADAGUC_REDIS": "redis://localhost:1"}),
        patch.object(caching_middleware, "ADAGUC_REDIS", "redis://localhost:1"),
        patch.object(caching_middleware, "get_cached_response", get_cached_response),
        patch.object(caching_middleware, "response_to_cache", response_to_cache),
    ):
        yield cache


@pytest.fixture(name="client")
def fixture_client(cache):  # pylint: disable=unused-argument
    """App that answers with the request method and body, behind the caching middleware"""
    app = FastAPI()
    app.add_middleware(caching_middleware.CachingMiddleware)

    @app.api_route("/wms", methods=["GET", "POST"])
    async def wms(req: Request):
        body = (await req.body()).decode()
        return PlainTextResponse(f"{req.method} {body}", headers={"cache-control": "max-age=60"})

    return TestClient(app)


def test_post_does_not_poison_get(client, cache):
    response = client.post("/wms?LAYERS=testdata", data={"STYLESHEET": "<Style/>"})
    assert response.text == "POST STYLESHEET=%3CStyle%2F%3E"
    assert "adaguc-cache" not in response.headers
    assert not cache

    response = client.get("/wms?LAYERS=testdata")
    assert response.text == "GET "
    assert response.headers["adaguc-cache"] == "miss"


def test_post_is_not_served_from_cache(client, cache):
    client.get("/wms?LAYERS=testdata")
    assert len(cache) == 1
    assert client.get("/wms?LAYERS=testdata").headers["adaguc-cache"] == "hit"

    response = client.post("/wms?LAYERS=testdata", data={"STYLESHEET": "<Style/>"})
    assert response.text == "POST STYLESHEET=%3CStyle%2F%3E"
    assert "adaguc-cache" not in response.headers
