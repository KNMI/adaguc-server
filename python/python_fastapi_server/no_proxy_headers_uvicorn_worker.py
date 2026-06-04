from uvicorn.workers import UvicornWorker

"""NoProxyHeadersUvicornWorker

This class extends the UvicornWorker class, setting a few properties
to non-default values to work in combination with the ForwardedHostAndPrefixMiddleware class:
It sets proxy_headers=False: ForwardedHostAndPrefixMiddleware needs ProxyHeadersMiddleware to be 
      disabled in uvicorn, which is done by setting `proxy_headers` to false. 

"""


class NoProxyHeadersUvicornWorker(UvicornWorker):
    CONFIG_KWARGS = {"proxy_headers": False}
