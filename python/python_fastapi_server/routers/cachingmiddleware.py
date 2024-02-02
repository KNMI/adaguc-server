import os
from urllib.parse import urlsplit

from starlette.concurrency import iterate_in_threadpool
from starlette.middleware.base import BaseHTTPMiddleware
from starlette.responses import StreamingResponse, Response
from starlette.background import BackgroundTask

import calendar
from datetime import datetime, timedelta
import time
from redis import asyncio
import json

ADAGUC_REDIS=os.environ.get('ADAGUC_REDIS', "redis://localhost:6379")
redis = asyncio.from_url(ADAGUC_REDIS)

async def get_cached_response(request):
    key = generate_key(request)
    cached = await redis.get(key)
    if not cached:
        print("Cache miss")
        return None, None, None
    print("Cache hit")

    entrytime=int(cached[:10])
    currenttime=calendar.timegm(datetime.utcnow().utctimetuple())
    age=currenttime-entrytime

    headers_len=int(cached[10:16])
    headers=json.loads(cached[16:16+headers_len])

    data = cached[16+headers_len:]
    return age, headers, data

skip_headers=["x-process-time"]

async def cache_response(request, headers, data, ex: int=60):
    key=generate_key(request)

    allheaders={}
    for k in headers.keys():
        if k not in skip_headers:
            allheaders[k]=headers[k]
        else:
            print("skipping header", k)
    allheaders_json=json.dumps(allheaders)

    entrytime="%10d"%calendar.timegm(datetime.utcnow().utctimetuple())
    print("ENTRY:", entrytime)

    print("Caching ", key, allheaders_json, "<><><>", type(data), data[:80])
    await redis.set(key, bytes(entrytime, 'utf-8')+bytes("%06d"%len(allheaders_json), 'utf-8')+bytes(allheaders_json, 'utf-8')+data, ex=ex)

def generate_key(request):
    key = request['query_string']
    return key

class CachingMiddleware(BaseHTTPMiddleware):
    def __init__(self, app):
        super().__init__(app)

    async def dispatch(self, request, call_next):
        #Check if request is in cache, if so return that
        expire, headers, data = await get_cached_response(request)
        print("AGE:", expire, data[:20] if data else None)

        if data:
            #Fix Age header
            headers["Age"]="%1d"%(expire)
            return Response(content=data, status_code=200, headers=headers, media_type=headers['content-type'])

        response: Response = await call_next(request)

        if response.status_code == 200:
            if "cache-control" in response.headers and response.headers['cache-control']!="no-store":
                print("CachingMiddleware:", response.headers['cache-control'])
                age_terms = response.headers['cache-control'].split("=")
                if age_terms[0].lower()!="max-age":
                    return
                age=int(age_terms[1])
                response_body = b""
                async for chunk in response.body_iterator:
                    response_body+=chunk
                task = BackgroundTask(cache_response, request=request, headers=response.headers, data=response_body, ex=age)
    #            await cache_response(request, response.headers, response_body, age)
                response.headers['age']="0"
    #            print("HDRS:",response.headers)
                return Response(content=response_body, status_code=200, headers=response.headers, media_type=response.media_type, background=task)
            print("NOT CACHING ", generate_key(request))
        return response
