"""
Tests that the caching middleware only caches GET requests: the cache key is the url, a POST has parameters in the body.
Requests with a STYLESHEET are not cached.
Also tests that cached responses are stored brotli compressed for content types which need compression.
"""

import asyncio
import json
import os
from unittest.mock import patch
from fastapi import FastAPI, Request
from fastapi.responses import PlainTextResponse
from fastapi.testclient import TestClient
from starlette.requests import Request as StarletteRequest
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


@pytest.mark.parametrize("key", ["STYLESHEET", "stylesheet"])
def test_stylesheet_is_not_cached(client, cache, key):
    """A STYLESHEET is a style for one request, its responses are neither stored nor served from the cache"""
    client.get("/wms?LAYERS=testdata")
    assert len(cache) == 1

    response = client.get(f"/wms?LAYERS=testdata&{key}=%3CStyle%2F%3E")
    assert response.text == "GET "
    assert "adaguc-cache" not in response.headers
    assert len(cache) == 1


class FakeRedis:
    """Replaces redis.Redis, stores the values in a dict"""

    store = {}

    def __init__(self, connection_pool):  # pylint: disable=unused-argument
        pass

    async def get(self, key):
        return self.store.get(key)

    async def set(self, key, value, ex):  # pylint: disable=unused-argument
        self.store[key] = value

    async def aclose(self):
        pass


def make_request(path):
    return StarletteRequest({"type": "http", "method": "GET", "path": path, "query_string": b"", "headers": []})


@pytest.fixture(name="fake_redis")
def fixture_fake_redis():
    FakeRedis.store = {}
    with patch.object(caching_middleware.redis, "Redis", FakeRedis):
        yield FakeRedis.store


COVJSON = json.dumps({"type": "Coverage", "ranges": {"ta": {"values": [12.5] * 100}}}).encode("utf-8")
COVJSON_HEADERS = {"content-type": "application/prs.coverage+json"}


def test_covjson_is_cached_compressed(fake_redis):
    request = make_request("/edr/collections/test/position")
    asyncio.run(caching_middleware.response_to_cache(None, request, COVJSON_HEADERS, COVJSON, ex=60))
    (cached,) = fake_redis.values()
    assert len(cached) < len(COVJSON)

    _, headers, data = asyncio.run(caching_middleware.get_cached_response(None, request))
    assert headers == COVJSON_HEADERS
    assert data == COVJSON


def test_uncompressed_covjson_in_cache_is_a_miss(fake_redis):
    """CoverageJSON cached by a version which did not compress it"""
    request = make_request("/edr/collections/test/position")
    headers_json = json.dumps(COVJSON_HEADERS).encode("utf-8")
    fake_redis[caching_middleware.generate_key(request)] = f"{0:10d}{len(headers_json):06d}".encode("utf-8") + headers_json + COVJSON

    assert asyncio.run(caching_middleware.get_cached_response(None, request)) == (None, None, None)
