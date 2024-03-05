from io import BytesIO
import os
from urllib.parse import urlsplit

from starlette.middleware.base import BaseHTTPMiddleware
from starlette.responses import Response
from fastapi import BackgroundTasks

import calendar
from datetime import datetime
from redis.asyncio import Redis  # This can also be used to connect to a Redis cluster
# from redis.asyncio.cluster import RedisCluster as Redis  # Cluster client, for testing

import json
import brotli

ADAGUC_REDIS=os.environ.get('ADAGUC_REDIS')

async def get_cached_response(redis_client, request):
    key = generate_key(request)
    cached = await redis_client.get(key)
    if not cached:
        # print("Cache miss")
        return None, None, None
    # print("Cache hit", len(cached))

    entrytime = int(cached[:10].decode('utf-8'))
    currenttime = calendar.timegm(datetime.utcnow().utctimetuple())
    age = currenttime-entrytime

    headers_len = int(cached[10:16].decode('utf-8'))
    headers = json.loads(cached[16:16+headers_len].decode('utf-8'))

    data = brotli.decompress(cached[16+headers_len:])
    return age, headers, data

skip_headers = ["x-process-time", "age"]

async def response_to_cache(redis_client, request, headers, data, ex: int):
    key=generate_key(request)

    fixed_headers={}
    for k in headers:
      if not k in skip_headers:
        fixed_headers[k] = headers[k]
    headers_json = json.dumps(fixed_headers, ensure_ascii=False).encode('utf-8')

    entrytime=f"{calendar.timegm(datetime.utcnow().utctimetuple()):10d}".encode('utf-8')
    await redis_client.set(key, entrytime+f"{len(headers_json):06d}".encode('utf-8')+headers_json+brotli.compress(data), ex=ex)

def generate_key(request):
    key = request.url.path.encode('utf-8')
    if len(request['query_string']) > 0:
        key += b"?" + request['query_string']
    return key

class CachingMiddleware(BaseHTTPMiddleware):
    shortcut = True
    def __init__(self, app):
        super().__init__(app)
        if "ADAGUC_REDIS" in os.environ:
            self.shortcut=False
            self.redis = Redis.from_url(ADAGUC_REDIS)

    async def dispatch(self, request, call_next):
        if self.shortcut:
            return await call_next(request)

        #Check if request is in cache, if so return that
        age, headers, data = await get_cached_response(self.redis, request)

        if data:
            #Fix Age header
            headers["Age"] = "%1d"%(age)
            headers["adaguc-cache"] = "hit"
            return Response(content=data, status_code=200, headers=headers, media_type=headers['content-type'])

        response: Response = await call_next(request)

        if response.status_code == 200:
            if "cache-control" in response.headers and response.headers['cache-control']!="no-store":
                cache_control_terms = response.headers['cache-control'].split(",")
                ttl = None
                for term in cache_control_terms:
                    age_terms = term.split("=")
                    if age_terms[0].lower()=="max-age":
                        ttl = int(age_terms[1])
                        break
                if ttl is None:
                    return response
                response_body_file = BytesIO()
                async for chunk in response.body_iterator:
                    response_body_file.write(chunk)
                tasks = BackgroundTasks()
                tasks.add_task(response_to_cache, redis_client=self.redis, request=request, headers=response.headers, data=response_body_file.getvalue(), ex=ttl)
                response.headers['age'] = "0"
                response.headers['adaguc-cache'] = "miss"
                return Response(content=response_body_file.getvalue(), status_code=200, headers=response.headers, media_type=response.media_type, background=tasks)

        response.headers["adaguc-cache"] = "err"
        return response
