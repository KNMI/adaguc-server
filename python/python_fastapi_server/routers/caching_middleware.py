from io import BytesIO
import os

from starlette.middleware.base import BaseHTTPMiddleware
from starlette.responses import Response
from fastapi import BackgroundTasks

import calendar
from datetime import datetime
import redis.asyncio as redis  # This can also be used to connect to a Redis cluster

import json
import brotli

ADAGUC_REDIS = os.environ.get("ADAGUC_REDIS")

MAX_SIZE_FOR_CACHING = 10_000_000


async def get_cached_response(redis_pool, request):
    key = generate_key(request)
    redis_client = redis.Redis(connection_pool=redis_pool)
    cached = await redis_client.get(key)
    await redis_client.aclose()
    if not cached:
        return None, None, None

    entrytime = int(cached[:10].decode("utf-8"))
    currenttime = calendar.timegm(datetime.utcnow().utctimetuple())
    age = currenttime - entrytime

    headers_len = int(cached[10:16].decode("utf-8"))
    headers = json.loads(cached[16 : 16 + headers_len].decode("utf-8"))

    data = brotli.decompress(cached[16 + headers_len :])
    return age, headers, data


headers_to_skip = ["x-process-time", "age"]


async def response_to_cache(redis_pool, request, headers, data, ex: int):
    key = generate_key(request)

    fixed_headers = {}
    for k in headers:
        if not k in headers_to_skip:
            fixed_headers[k] = headers[k]
    headers_json = json.dumps(fixed_headers, ensure_ascii=False).encode("utf-8")

    entrytime = f"{calendar.timegm(datetime.utcnow().utctimetuple()):10d}".encode(
        "utf-8"
    )
    compressed_data = brotli.compress(data)
    if len(compressed_data) < MAX_SIZE_FOR_CACHING:
        redis_client = redis.Redis(connection_pool=redis_pool)
        await redis_client.set(
            key,
            entrytime
            + f"{len(headers_json):06d}".encode("utf-8")
            + headers_json
            + brotli.compress(data),
            ex=ex,
        )
        await redis_client.aclose()


def generate_key(request):
    key = request.url.path.encode("utf-8")
    if len(request["query_string"]) > 0:
        key += b"?" + request["query_string"]
    return key


class CachingMiddleware(BaseHTTPMiddleware):
    shortcut = True

    def __init__(self, app):
        super().__init__(app)
        if "ADAGUC_REDIS" in os.environ:
            self.shortcut = False
            self.redis_pool = redis.ConnectionPool.from_url(ADAGUC_REDIS)

    async def dispatch(self, request, call_next):
        if self.shortcut:
            return await call_next(request)

        # Check if request is in cache, if so return that
        age, headers, data = await get_cached_response(self.redis_pool, request)

        if data:
            # Fix Age header
            headers["Age"] = f"{age:1d}"
            headers["adaguc-cache"] = "hit"
            return Response(
                content=data,
                status_code=200,
                headers=headers,
                media_type=headers["content-type"],
            )

        response: Response = await call_next(request)

        if response.status_code == 200:
            if (
                "cache-control" in response.headers
                and response.headers["cache-control"] != "no-store"
            ):
                cache_control_terms = response.headers["cache-control"].split(",")
                ttl = None
                for term in cache_control_terms:
                    age_terms = term.split("=")
                    if age_terms[0].lower() == "max-age":
                        ttl = int(age_terms[1])
                        break
                if ttl is None:
                    return response

                with BytesIO() as response_body_file:
                    async for chunk in response.body_iterator:
                        response_body_file.write(chunk)
                    response_body = response_body_file.getvalue()

                tasks = BackgroundTasks()
                tasks.add_task(
                    response_to_cache,
                    redis_pool=self.redis_pool,
                    request=request,
                    headers=response.headers,
                    data=response_body,
                    ex=ttl,
                )
                response.headers["age"] = "0"
                response.headers["adaguc-cache"] = "miss"
                return Response(
                    content=response_body,
                    status_code=200,
                    headers=response.headers,
                    media_type=response.media_type,
                    background=tasks,
                )

        response.headers["adaguc-cache"] = "err"
        return response
