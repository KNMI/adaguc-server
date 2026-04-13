from uvicorn.workers import UvicornWorker


class NoProxyHeadersUvicornWorker(UvicornWorker):
    CONFIG_KWARGS = {"loop": "asyncio", "http": "h11", "proxy_headers": False}
