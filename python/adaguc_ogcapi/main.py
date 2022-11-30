from fastapi import Depends, FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
import time


from .routers import maps

app = FastAPI()


@app.middleware("http")
async def add_process_time_header(request: Request, call_next):
    start_time = time.time()
    response = await call_next(request)
    process_time = time.time() - start_time
    response.headers["X-Process-Time"] = str(process_time)
    return response


app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

datasets = ["RADAR", "HARM_N25"]
for dataset in datasets:
    maps.add_router(dataset, app)
# app.include_router(maps.router)

print("OpenApi:", app.openapi_url)


@app.get("/")
async def root():
    return {"message": "Hello Bigger Applications!"}
