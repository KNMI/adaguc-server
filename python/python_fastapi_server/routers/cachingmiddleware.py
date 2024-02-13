import os
from urllib.parse import urlsplit

# from starlette.middleware.base import BaseHTTPMiddleware
from starlette.middleware.base import BaseHTTPMiddleware
from starlette.responses import Response
from fastapi import BackgroundTasks

import calendar
from datetime import datetime, timedelta
import time
import redis.asyncio as redis
import json

ADAGUC_REDIS=os.environ.get('ADAGUC_REDIS')

async def get_cached_response(redis_client, request):
    key = generate_key(request)
    cached = await redis_client.get(key)
    await redis_client.aclose()
    if not cached:
        # print("Cache miss")
        return None, None, None
    # print("Cache hit", len(cached))

    entrytime=int(cached[:10])
    currenttime=calendar.timegm(datetime.utcnow().utctimetuple())
    age=currenttime-entrytime

    headers_len=int(cached[10:16])
    headers=json.loads(cached[16:16+headers_len])

    data = cached[16+headers_len:]
    return age, headers, data

skip_headers=["x-process-time", "age"]

async def cache_response(redis_client, request, headers, data, ex: int=60):
    key=generate_key(request)

    allheaders={}
    for k in headers.keys():
        if k not in skip_headers:
            allheaders[k]=headers[k]
    allheaders_json=json.dumps(allheaders)

    entrytime="%10d"%calendar.timegm(datetime.utcnow().utctimetuple())
    await redis_client.set(key, bytes(entrytime, 'utf-8')+bytes("%06d"%len(allheaders_json), 'utf-8')+bytes(allheaders_json, 'utf-8')+data, ex=ex)
    await redis_client.aclose()

def generate_key(request):
    key = f"{request.url.path}?{request['query_string']}"
    # print(f"generate_key({key})")
    return key

class CachingMiddleware(BaseHTTPMiddleware):
    shortcut = True
    def __init__(self, app):
        super().__init__(app)
        if "ADAGUC_REDIS" in os.environ:
            self.shortcut=False
            self.redis = None

    async def dispatch(self, request, call_next):
        if self.redis is None:
            self.redis = redis.from_url(ADAGUC_REDIS)
        if self.shortcut:
            return await call_next(request)

        #Check if request is in cache, if so return that
        expire, headers, data = await get_cached_response(self.redis, request)

        if data:
            #Fix Age header
            headers["Age"]="%1d"%(expire)
            return Response(content=data, status_code=200, headers=headers, media_type=headers['content-type'])

        response: Response = await call_next(request)

        if response.status_code == 200:
            if "cache-control" in response.headers and response.headers['cache-control']!="no-store":
                cache_control_terms = response.headers['cache-control'].split(",")
                ttl = None
                for term in cache_control_terms:
                    age_terms = term.split("=")
                    if age_terms[0].lower()=="max-age":
                        ttl=int(age_terms[1])
                        break
                if ttl is None:
                    return
                response_body = b""
                async for chunk in response.body_iterator:
                    response_body+=chunk
                tasks = BackgroundTasks()
                tasks.add_task(cache_response, redis_client=self.redis, request=request, headers=response.headers, data=response_body, ex=ttl)
                response.headers['age']="0"
                return Response(content=response_body, status_code=200, headers=response.headers, media_type=response.media_type, background=tasks)
        return response
