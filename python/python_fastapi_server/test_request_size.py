"""
Tests that uvicorn, with the settings of the gunicorn worker, accepts long query strings, like a STYLESHEET.
uvicorn's HTTP parser h11 refuses a request when more than 16 KB of it arrived without the end of the headers by default.
Over a network a long request arrives in several pieces, so this test sends the end of the headers separately.
"""

import socket
import threading
import time
import warnings
import pytest
import uvicorn

with warnings.catch_warnings():
    # uvicorn.workers warns that it moved to the uvicorn-worker package
    warnings.simplefilter("ignore", DeprecationWarning)
    from no_proxy_headers_uvicorn_worker import NoProxyHeadersUvicornWorker


async def app(scope, receive, send):  # pylint: disable=unused-argument
    """Answers with the length of the query string"""
    if scope["type"] != "http":
        return
    await send({"type": "http.response.start", "status": 200, "headers": []})
    await send({"type": "http.response.body", "body": str(len(scope["query_string"])).encode()})


def start_server(**config_kwargs):
    """Starts uvicorn in a thread on a free port"""
    sock = socket.socket()
    sock.bind(("127.0.0.1", 0))
    server = uvicorn.Server(uvicorn.Config(app, log_level="warning", **config_kwargs))
    thread = threading.Thread(target=server.run, kwargs={"sockets": [sock]}, daemon=True)
    thread.start()
    while not server.started:
        time.sleep(0.01)
    return server, thread, sock.getsockname()[1]


def get_in_two_pieces(port, query_string):
    """Sends the request without the end of its headers, waits until the server read it, then sends the end"""
    request = f"GET /wms?{query_string} HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n".encode()
    with socket.create_connection(("127.0.0.1", port)) as connection:
        connection.sendall(request)
        time.sleep(0.2)
        try:
            connection.sendall(b"\r\n")
        except (BrokenPipeError, ConnectionResetError):
            pass  # Refused before the whole request was sent, the status line is read below
        return connection.recv(4096).split(b"\r\n")[0].decode()


@pytest.mark.parametrize(
    "config_kwargs, expected",
    [
        (NoProxyHeadersUvicornWorker.CONFIG_KWARGS, "HTTP/1.1 200 OK"),
        ({}, "HTTP/1.1 400 Bad Request"),
    ],
    ids=["worker settings", "h11 default"],
)
def test_long_query_in_two_pieces(config_kwargs, expected):
    """A query string of 60 KB is accepted with the worker settings, h11 refuses it by default"""
    server, thread, port = start_server(**config_kwargs)
    try:
        assert get_in_two_pieces(port, "STYLESHEET=" + "a" * 60000) == expected
    finally:
        server.should_exit = True
        thread.join(timeout=5)
