"""NoProxyHeadersUvicornWorker

This class extends the UvicornWorker class, setting a few properties
to non-default values to work in combination with the ForwardedHostAndPrefixMiddleware class:
It sets proxy_headers=False: ForwardedHostAndPrefixMiddleware needs ProxyHeadersMiddleware to be
      disabled in uvicorn, which is done by setting `proxy_headers` to false.

It also sets h11_max_incomplete_event_size, so long query strings are accepted, see MAX_REQUEST_HEAD_BYTES.
"""

from uvicorn.workers import UvicornWorker
from routers.wmswcs import MAX_REQUEST_HEAD_BYTES


class NoProxyHeadersUvicornWorker(UvicornWorker):
    """UvicornWorker without proxy headers and with room for long query strings"""

    CONFIG_KWARGS = {"proxy_headers": False, "h11_max_incomplete_event_size": MAX_REQUEST_HEAD_BYTES}
