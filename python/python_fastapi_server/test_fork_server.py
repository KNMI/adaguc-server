import asyncio
import os
import re
import signal
import sys
from pathlib import Path
from unittest.mock import AsyncMock, Mock

import psutil
import pytest
import pytest_asyncio
from fastapi.testclient import TestClient

import adaguc.CGIRunner as cgi_runner_module
from adaguc.CGIRunner import CGIRunner, HTTP_STATUSCODE_404_NOT_FOUND
from fork_server_supervisor import ForkServerSupervisor
from main import app

ADAGUC_PATH = Path(os.environ["ADAGUC_PATH"])
ADAGUC_BINARY = ADAGUC_PATH / "bin" / "adagucserver"
FORK_SOCKET = ADAGUC_PATH / "adaguc.socket"
EXPECTED_GET_MAP = ADAGUC_PATH / "tests" / "expectedoutputs" / "TestWMS" / "test_WMSGetMap_testdatanc.png"
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
GET_MAP_QUERY = "source=testdata.nc&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE"
MISSING_COVERAGE_QUERY = (
    "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=nonexisting&CRS=EPSG%3A4326&FORMAT=NetCDF4&BBOX=-10,40,20,60"
)


@pytest.fixture
def fork_environment(monkeypatch):
    assert not FORK_SOCKET.exists(), f"Refusing to replace an existing fork-server socket at {FORK_SOCKET}"
    monkeypatch.setenv("ADAGUC_FORK_ENABLE", "TRUE")
    monkeypatch.setenv("ADAGUC_CONFIG", str(ADAGUC_PATH / "data" / "config" / "adaguc.autoresource.xml"))


@pytest_asyncio.fixture
async def fork_server(fork_environment, monkeypatch, request):
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


@pytest.mark.asyncio
async def test_fork_server_get_map_uses_distinct_children(fork_server, tmp_path):
    mother_pid = fork_server.process.pid
    child_pids = []

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


def test_fastapi_lifespan_routes_wms_through_fork_server(fork_environment, monkeypatch, tmp_path):
    monkeypatch.chdir(tmp_path)

    with TestClient(app) as client:
        assert FORK_SOCKET.is_socket()
        assert not (tmp_path / "adaguc.socket").exists()
        response = client.get(f"/wms?{GET_MAP_QUERY}")

        assert response.status_code == 200
        assert response.headers["content-type"] == "image/png"
        assert response.content.startswith(PNG_SIGNATURE)

    assert not FORK_SOCKET.exists()


@pytest.mark.asyncio
async def test_fork_server_propagates_child_exit_status(fork_server):
    status, *_ = await CGIRunner().run([str(ADAGUC_BINARY)], MISSING_COVERAGE_QUERY)

    assert status == HTTP_STATUSCODE_404_NOT_FOUND


@pytest.mark.asyncio
async def test_command_invocation_bypasses_fork_server(fork_server):
    mother_pid = fork_server.process.pid

    status, *_ = await CGIRunner().run([str(ADAGUC_BINARY), "--test"], url=None, env=os.environ.copy(), isCGI=False)

    assert status == 0
    assert fork_server.process.pid == mother_pid
    assert fork_server.process.returncode is None
    assert await fork_server.health_check_mother()


@pytest.mark.asyncio
@pytest.mark.parametrize("fork_server", [{"env": {"ADAGUC_MAX_PROC_TIMEOUT": "1"}}], indirect=True)
async def test_fork_server_times_out_and_reaps_blocked_child(fork_server):
    reader, writer = await asyncio.open_unix_connection(FORK_SOCKET)
    try:
        await wait_for_child_count(fork_server.process.pid, 1)
        response = await asyncio.wait_for(reader.read(), timeout=4)
    finally:
        writer.close()
        await writer.wait_closed()

    assert int.from_bytes(response, sys.byteorder) == signal.SIGKILL
    await wait_for_child_count(fork_server.process.pid, 0)
    assert await fork_server.health_check_mother()


@pytest.mark.asyncio
@pytest.mark.parametrize(
    "fork_server",
    [{"env": {"ADAGUC_NUMPARALLELPROCESSES": "2", "ADAGUC_MAX_PROC_TIMEOUT": "5"}}],
    indirect=True,
)
async def test_fork_server_keeps_health_check_headroom(fork_server):
    connections = []
    try:
        for expected_children in (1, 2):
            connections.append(await asyncio.open_unix_connection(FORK_SOCKET))
            await wait_for_child_count(fork_server.process.pid, expected_children)

        assert await fork_server.health_check_mother()
        await wait_for_child_count(fork_server.process.pid, 2)
    finally:
        for _, writer in connections:
            writer.close()
            await writer.wait_closed()

    await wait_for_child_count(fork_server.process.pid, 0)


@pytest.mark.asyncio
async def test_fork_server_child_does_not_keep_other_client_socket_open(fork_server):
    reader_a, writer_a = await asyncio.open_unix_connection(FORK_SOCKET)
    reader_b = writer_b = None
    try:
        await wait_for_child_count(fork_server.process.pid, 1)
        reader_b, writer_b = await asyncio.open_unix_connection(FORK_SOCKET)
        await wait_for_child_count(fork_server.process.pid, 2)

        writer_a.write(b"PING\n")
        await writer_a.drain()
        response = await asyncio.wait_for(reader_a.read(), timeout=1)

        assert response == b"PONG\n" + (0).to_bytes(4, sys.byteorder)
        assert not reader_b.at_eof()
    finally:
        writer_a.close()
        await writer_a.wait_closed()
        if writer_b is not None:
            writer_b.close()
            await writer_b.wait_closed()

    await wait_for_child_count(fork_server.process.pid, 0)


@pytest.mark.asyncio
async def test_fork_server_clean_shutdown(fork_server):
    process = fork_server.process
    reader, writer = await asyncio.open_unix_connection(FORK_SOCKET)
    child_pid = (await wait_for_child_count(process.pid, 1)).pop()

    await fork_server.stop_monitoring()

    assert process.returncode is not None
    assert not FORK_SOCKET.exists()
    assert await asyncio.wait_for(reader.read(), timeout=1) == b""
    with pytest.raises(ProcessLookupError):
        os.kill(child_pid, 0)

    writer.close()
    await writer.wait_closed()


@pytest.mark.asyncio
@pytest.mark.parametrize("fork_server", [{"interval": 0.05}], indirect=True)
async def test_fork_server_restarts_after_mother_is_killed(fork_server):
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
async def test_cgi_runner_selects_request_and_command_timeouts(monkeypatch):
    calls = []

    async def fake_socket_request(url, env, timeout):
        calls.append(("request", timeout))
        return cgi_runner_module.AdagucResponse(0, b"", b"")

    async def fake_command(cmds, env, timeout):
        calls.append(("command", timeout))
        return cgi_runner_module.AdagucResponse(0, b"", b"")

    monkeypatch.setenv("ADAGUC_FORK_ENABLE", "TRUE")
    monkeypatch.setattr(cgi_runner_module, "wait_socket_communicate", fake_socket_request)
    monkeypatch.setattr(cgi_runner_module, "wait_process_communicate", fake_command)

    runner = CGIRunner()
    await runner.run([str(ADAGUC_BINARY)], GET_MAP_QUERY, isCGI=False)
    await runner.run([str(ADAGUC_BINARY), "--test"], None, isCGI=False)

    assert calls == [
        ("request", cgi_runner_module.MAX_PROC_TIMEOUT),
        ("command", cgi_runner_module.MAX_COMMAND_TIMEOUT),
    ]


@pytest.mark.asyncio
@pytest.mark.parametrize(
    ("query", "env"),
    [
        ("x" * cgi_runner_module.MAX_FORK_REQUEST_BYTES, {}),
        ("SERVICE=WMS", {"ADAGUC_LOGFILE": "invalid\nvalue"}),
    ],
)
async def test_fork_client_rejects_invalid_request_before_connecting(query, env):
    assert not FORK_SOCKET.exists()

    response = await cgi_runner_module.socket_communicate(query, env)

    assert response.status_code == cgi_runner_module.HTTP_STATUSCODE_400_BAD_REQUEST
    assert response.process_output is None


@pytest.mark.asyncio
async def test_fork_client_rejects_response_without_exit_status(monkeypatch):
    reader = asyncio.StreamReader()
    reader.feed_data(b"bad")
    reader.feed_eof()
    writer = Mock(write=Mock(), drain=AsyncMock(), close=Mock(), wait_closed=AsyncMock())
    monkeypatch.setattr(cgi_runner_module.asyncio, "open_unix_connection", AsyncMock(return_value=(reader, writer)))

    with pytest.raises(RuntimeError, match="missing exit status"):
        await cgi_runner_module.socket_communicate("SERVICE=WMS", {})

    writer.close.assert_called_once()
    writer.wait_closed.assert_awaited_once()
