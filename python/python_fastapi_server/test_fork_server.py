import asyncio
import os
import re
import signal
import sys
from contextlib import asynccontextmanager
from pathlib import Path
from unittest.mock import AsyncMock, Mock

import psutil
import pytest
import pytest_asyncio

import adaguc.CGIRunner as cgi_runner_module
from adaguc.CGIRunner import CGIRunner
from fork_server_supervisor import ForkServerSupervisor

ADAGUC_PATH = Path(os.environ["ADAGUC_PATH"])
ADAGUC_BINARY = ADAGUC_PATH / "bin" / "adagucserver"
FORK_SOCKET = ADAGUC_PATH / "adaguc.socket"
EXPECTED_GET_MAP = ADAGUC_PATH / "tests" / "expectedoutputs" / "TestWMS" / "test_WMSGetMap_testdatanc.png"
GET_MAP_QUERY = "source=testdata.nc&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE"


@pytest.fixture
def fork_environment(monkeypatch):
    # Enable fork mode with the standard autoresource configuration and ensure no stale socket exists.
    assert not FORK_SOCKET.exists(), f"Refusing to replace an existing fork-server socket at {FORK_SOCKET}"
    monkeypatch.setenv("ADAGUC_FORK_ENABLE", "TRUE")
    monkeypatch.setenv("ADAGUC_CONFIG", str(ADAGUC_PATH / "data" / "config" / "adaguc.autoresource.xml"))


@pytest_asyncio.fixture
async def fork_server(fork_environment, monkeypatch, request):
    # Start a real supervised fork server with optional per-test settings and always stop it afterward.
    settings = getattr(request, "param", {})
    for key, value in settings.get("env", {}).items():
        monkeypatch.setenv(key, value)

    supervisor = ForkServerSupervisor(interval=settings.get("interval", 60), startup_timeout=10)
    try:
        await supervisor.start_monitoring()
        yield supervisor
    finally:
        await supervisor.stop_monitoring()

    assert not FORK_SOCKET.exists()


def get_child_pids(parent_pid) -> set:
    try:
        return {child.pid for child in psutil.Process(parent_pid).children()}
    except psutil.NoSuchProcess:
        return set()


async def wait_for_child_count(parent_pid, expected_count, timeout=2):
    loop = asyncio.get_running_loop()
    deadline = loop.time() + timeout
    while loop.time() < deadline:
        child_pids = get_child_pids(parent_pid)
        if len(child_pids) == expected_count:
            return child_pids
        await asyncio.sleep(0.02)

    child_pids = get_child_pids(parent_pid)
    pytest.fail(f"Expected {expected_count} fork-server children, found {child_pids}")


@pytest.fixture
def fork_connection():
    # Provide a raw fork-server connection context that always closes its client-side writer.
    @asynccontextmanager
    async def connect():
        reader, writer = await asyncio.open_unix_connection(FORK_SOCKET)
        try:
            yield reader, writer
        finally:
            writer.close()
            await writer.wait_closed()

    return connect


@pytest.mark.asyncio
async def test_fork_server_get_map_uses_distinct_children(fork_server, monkeypatch, tmp_path):
    # Verify real GetMap output, a stable mother, distinct request children, and a cwd-independent socket path.
    mother_pid = fork_server.process.pid
    child_pids = []

    assert FORK_SOCKET.is_socket()
    monkeypatch.chdir(tmp_path)
    assert not (tmp_path / "adaguc.socket").exists()

    for request_number in range(2):
        log_path = tmp_path / f"request-{request_number}.log"
        status, headers, process_error, body = await CGIRunner().run(
            [str(ADAGUC_BINARY)], GET_MAP_QUERY, env={"ADAGUC_LOGFILE": str(log_path)}
        )

        assert status == 0
        assert process_error == b""
        assert "Content-Type:image/png" in headers
        if request_number == 0:
            assert body == EXPECTED_GET_MAP.read_bytes()

        request_pids = set(re.findall(r"pid(\d+)", log_path.read_text(encoding="utf-8")))
        assert len(request_pids) == 1
        child_pids.append(request_pids.pop())

        assert fork_server.process.pid == mother_pid
        assert fork_server.process.returncode is None

    assert child_pids[0] != child_pids[1]


@pytest.mark.asyncio
async def test_command_invocation_bypasses_fork_server(fork_server):
    # Verify command-style invocations use a separate subprocess without disrupting the fork server.
    mother_pid = fork_server.process.pid

    status, *_ = await CGIRunner().run([str(ADAGUC_BINARY), "--test"], url=None, env=os.environ.copy(), isCGI=False)

    assert status == 0
    assert fork_server.process.pid == mother_pid
    assert fork_server.process.returncode is None
    assert await fork_server.health_check_mother()


@pytest.mark.asyncio
@pytest.mark.parametrize("fork_server", [{"env": {"ADAGUC_MAX_PROC_TIMEOUT": "1"}}], indirect=True)
async def test_fork_server_times_out_and_reaps_blocked_child(fork_server, fork_connection):
    # Leave a request blocked and verify the server kills and reaps its child while remaining healthy.
    async with fork_connection() as (reader, _):
        await wait_for_child_count(fork_server.process.pid, 1)
        response = await asyncio.wait_for(reader.read(), timeout=4)

    assert int.from_bytes(response, sys.byteorder) == signal.SIGKILL
    await wait_for_child_count(fork_server.process.pid, 0)
    assert await fork_server.health_check_mother()


@pytest.mark.asyncio
@pytest.mark.parametrize(
    "fork_server",
    [{"env": {"ADAGUC_NUMPARALLELPROCESSES": "2", "ADAGUC_MAX_PROC_TIMEOUT": "5"}}],
    indirect=True,
)
async def test_fork_server_keeps_health_check_headroom(fork_server, fork_connection):
    # Fill the configured request capacity and verify that the reserved extra child slot
    # still allows the supervisor's PING health check to run.
    async with fork_connection():
        await wait_for_child_count(fork_server.process.pid, 1)
        async with fork_connection():
            await wait_for_child_count(fork_server.process.pid, 2)
            assert await fork_server.health_check_mother()
            await wait_for_child_count(fork_server.process.pid, 2)

    await wait_for_child_count(fork_server.process.pid, 0)


@pytest.mark.asyncio
async def test_fork_server_child_does_not_keep_other_client_socket_open(fork_server, fork_connection):
    # A newly forked child inherits older client descriptors; it must close them so
    # an older client can receive EOF while the newer child remains active.
    async with fork_connection() as (reader_a, writer_a):
        await wait_for_child_count(fork_server.process.pid, 1)
        async with fork_connection() as (reader_b, _):
            await wait_for_child_count(fork_server.process.pid, 2)

            writer_a.write(b"PING\n")
            await writer_a.drain()
            response = await asyncio.wait_for(reader_a.read(), timeout=1)

            assert response == b"PONG\n" + (0).to_bytes(4, sys.byteorder)
            assert not reader_b.at_eof()

    await wait_for_child_count(fork_server.process.pid, 0)


@pytest.mark.asyncio
async def test_fork_server_clean_shutdown(fork_server, fork_connection):
    # Stop the supervisor with an active request and verify all processes, sockets, and connections close.
    process = fork_server.process
    async with fork_connection() as (reader, _):
        child_pid = (await wait_for_child_count(process.pid, 1)).pop()

        await fork_server.stop_monitoring()

        assert process.returncode is not None
        assert not FORK_SOCKET.exists()
        assert await asyncio.wait_for(reader.read(), timeout=1) == b""
        with pytest.raises(ProcessLookupError):
            os.kill(child_pid, 0)


@pytest.mark.asyncio
@pytest.mark.parametrize("fork_server", [{"interval": 0.05}], indirect=True)
async def test_fork_server_restarts_after_mother_is_killed(fork_server):
    # Kill the real mother process and verify the supervisor replaces it with a healthy process.
    old_process = fork_server.process
    old_pid = old_process.pid
    old_process.kill()
    await old_process.wait()

    loop = asyncio.get_running_loop()
    deadline = loop.time() + 3
    while loop.time() < deadline:
        new_process = fork_server.process
        if new_process is not None and new_process.pid != old_pid and new_process.returncode is None:
            if await fork_server.health_check_mother():
                break
        await asyncio.sleep(0.02)
    else:
        pytest.fail("Fork-server supervisor did not restart the killed mother")

    assert fork_server.process.pid != old_pid


@pytest.mark.asyncio
async def test_supervisor_retries_failed_restart_and_can_still_stop(monkeypatch):
    # Verify a temporary restart error is retried without preventing later supervisor cancellation.
    supervisor = ForkServerSupervisor(interval=0.01)
    restarted = asyncio.Event()
    restart_attempts = 0

    async def restart_process():
        nonlocal restart_attempts
        restart_attempts += 1
        if restart_attempts == 1:
            raise OSError("temporary process-start failure")
        restarted.set()

    monkeypatch.setattr(supervisor, "start_process", AsyncMock())
    monkeypatch.setattr(supervisor, "wait_until_ready", AsyncMock(return_value=True))
    monkeypatch.setattr(supervisor, "health_check_mother", AsyncMock(return_value=False))
    monkeypatch.setattr(supervisor, "restart_process", restart_process)

    await supervisor.start_monitoring()
    task = supervisor._task
    try:
        await asyncio.wait_for(restarted.wait(), timeout=1)
    finally:
        await supervisor.stop_monitoring()

    assert restart_attempts >= 2
    assert task.done()


@pytest.mark.asyncio
@pytest.mark.parametrize(
    ("query", "env"),
    [
        ("x" * cgi_runner_module.MAX_FORK_REQUEST_BYTES, {}),
        ("SERVICE=WMS", {"ADAGUC_LOGFILE": "invalid\nvalue"}),
    ],
)
async def test_fork_client_rejects_invalid_request_before_connecting(query, env):
    # Reject oversized messages and newline-containing values before attempting a socket connection.
    assert not FORK_SOCKET.exists()

    response = await cgi_runner_module.socket_communicate(query, env)

    assert response.status_code == cgi_runner_module.HTTP_STATUSCODE_400_BAD_REQUEST
    assert response.process_output is None


@pytest.mark.asyncio
async def test_fork_client_rejects_response_without_exit_status(monkeypatch):
    # Reject a truncated server response that does not contain the required four-byte exit status.
    reader = asyncio.StreamReader()
    reader.feed_data(b"bad")
    reader.feed_eof()
    writer = Mock(write=Mock(), drain=AsyncMock(), close=Mock(), wait_closed=AsyncMock())
    monkeypatch.setattr(cgi_runner_module.asyncio, "open_unix_connection", AsyncMock(return_value=(reader, writer)))

    with pytest.raises(RuntimeError, match="missing exit status"):
        await cgi_runner_module.socket_communicate("SERVICE=WMS", {})

    writer.close.assert_called_once()
    writer.wait_closed.assert_awaited_once()
