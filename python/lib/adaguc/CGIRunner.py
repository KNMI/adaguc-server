"""
This Python script runs CGI scripts without an webserver.
Created by Maarten Plieger, KNMI -  2021-09-01
"""

import asyncio
import os
import re
import sys
from asyncio.subprocess import Process
from io import BytesIO
from subprocess import PIPE
from typing import NamedTuple

from adaguc.fork_settings import ADAGUC_FORK_SOCKET_PATH, is_fork_enabled

HTTP_STATUSCODE_404_NOT_FOUND = 32  # Must be the same as in Definitions.h
HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY = 33  # Must be the same as in Definitions.h
HTTP_STATUSCODE_500_TIMEOUT = 34  # Not defined in C++, is generated from this file
SCAN_EXITCODE_FILENOMATCH = 64  #  File is available but does not match any of the available datasets
SCAN_EXITCODE_DATASETNOEXIST = 65  # The reason for this status code is that the dataset configuration file does not exist.
SCAN_EXITCODE_SCANERROR = 66  #  An error occured during scanning
SCAN_EXITCODE_FILENOEXIST = 67  # The file does not exist on the file system
SCAN_EXITCODE_FILENOMATCH_ISDELETED = 68  #  File does not match any of the available datasets and is deleted
SCAN_EXITCODE_TIMEOUT = 124  # The process timed out


ADAGUC_NUMPARALLELPROCESSES = int(os.getenv("ADAGUC_NUMPARALLELPROCESSES", "4"))
sem = asyncio.Semaphore(max(ADAGUC_NUMPARALLELPROCESSES, 2))  # At least two, to allow for me layer metadata update

ON_POSIX = "posix" in sys.builtin_module_names

MAX_PROC_TIMEOUT = int(os.getenv("ADAGUC_MAX_PROC_TIMEOUT", "180"))
print("Max procs: ", max(ADAGUC_NUMPARALLELPROCESSES, 2))


class AdagucResponse(NamedTuple):
    status_code: int
    process_output: bytes | None


async def wait_socket_communicate(url: str, env: dict, timeout) -> AdagucResponse:
    """
    Call `socket_communicate` with a timeout.

    If communication with the fork server exceeds `timeout`, a timeout response is returned to the client.
    Any running child process will be cleaned up by the fork server itself.
    """

    try:
        resp = await asyncio.wait_for(socket_communicate(url, env), timeout=timeout)
    except asyncio.exceptions.TimeoutError:
        return AdagucResponse(status_code=HTTP_STATUSCODE_500_TIMEOUT, process_output=None)
    except ConnectionRefusedError:
        raise Exception("Failed to communicate over unix socket!")
    return resp


async def socket_communicate(url: str, env: dict) -> AdagucResponse:
    """
    Send a request to the ADAGUC fork server through a Unix socket.

    Environment variables required for request handling are serialized and written to the socket.
    The response contains the normal ADAGUC output, followed by a 4-byte exit status appended by the fork server.
    """

    process_output = bytearray()
    reader, writer = await asyncio.open_unix_connection(ADAGUC_FORK_SOCKET_PATH)

    data = [
        f"ADAGUC_LOGFILE={env['ADAGUC_LOGFILE']}",
        f"SCRIPT_NAME={env.get('SCRIPT_NAME', '')}",
        f"REQUEST_URI={env.get('REQUEST_URI', '')}",
        f"QUERY_STRING={url}",
    ]
    message = "\n".join(data)

    writer.write(message.encode())
    await writer.drain()

    process_output = await reader.read()

    writer.close()
    await writer.wait_closed()

    # Status code is stored in the last 4 bytes from the received data
    status_code = int.from_bytes(process_output[-4:], sys.byteorder)
    process_output = process_output[:-4]
    return AdagucResponse(status_code=status_code, process_output=process_output)


async def wait_process_communicate(cmds: list[str], localenv: dict, timeout) -> AdagucResponse:
    """
    Spawn an adagucserver process and wait for completion with a timeout.

    If the process does not finish within `timeout`, it is terminated and a timeout response is returned.
    """

    # process: Process | None = None

    try:
        process = None
        resp = await asyncio.wait_for(process_communicate(process, cmds, localenv), timeout=timeout)
    except asyncio.exceptions.TimeoutError:
        if process:
            process.kill()
            await process.communicate()

        return AdagucResponse(status_code=HTTP_STATUSCODE_500_TIMEOUT, process_output=None)
    return resp


async def process_communicate(process: Process, cmds, localenv) -> AdagucResponse:
    """
    Wait for a subprocess to finish and collect its output.

    The process stdout is returned as the response body, and the subprocess exit code is returned as the status code.
    """

    process = await asyncio.create_subprocess_exec(
        *cmds,
        stdout=PIPE,
        stderr=PIPE,
        env=localenv,
        close_fds=ON_POSIX,
    )

    process_output, _ = await process.communicate()
    status = await process.wait()

    return AdagucResponse(status_code=status, process_output=process_output)


class CGIRunner:
    """
    Run the CGI script with specified URL and environment. Stdout is captured and put in a BytesIO object provided in output
    """

    async def run(
        self,
        cmds: list[str],
        url: str,
        output: BytesIO,
        env: dict = {},
        path: str | None = None,
        isCGI: bool = True,
        timeout: int = MAX_PROC_TIMEOUT,
    ) -> tuple[int, list[str], bytes | None]:
        localenv = {}
        if url != None:
            localenv["QUERY_STRING"] = url
        else:
            localenv["QUERY_STRING"] = ""
        if path != None:
            localenv["SCRIPT_NAME"] = "/myscriptname"
            # SCRIPT_NAME [/cgi-bin/autoresource.cgi], REQUEST_URI [/cgi-bin/autoresource.cgi/opendap/clipc/combinetest/wcs_nc2.nc.das]
            localenv["REQUEST_URI"] = "/myscriptname/" + path
        localenv.update(env)

        # Only use fork server if ADAGUC_FORK_ENABLE=TRUE and adaguc is not executed with extra arguments e.g. `--updatelayermetadata`
        use_fork = is_fork_enabled() and len(cmds) == 1

        async with sem:
            if use_fork:
                response = await wait_socket_communicate(url, localenv, timeout=timeout)
            else:
                response = await wait_process_communicate(cmds, localenv, timeout=timeout)

            if response.status_code == HTTP_STATUSCODE_500_TIMEOUT:
                output.write(b"Adaguc server processs timed out")
                return HTTP_STATUSCODE_500_TIMEOUT, [], None

        process_error = "".encode()
        process_output = response.process_output
        status = response.status_code

        # Split headers from body using a regex
        headersEndAt = -2
        headers = ""
        if isCGI == True:
            pattern = re.compile(b"\x0a\x0a")
            search = pattern.search(process_output)
            if search:
                headersEndAt = search.start()
                headers = (process_output[0 : headersEndAt - 1]).decode()
            else:
                output.write(b"Error: No headers found in response from adaguc-server application, status was %d" % status)
                return 1, [], None

        body = process_output[headersEndAt + 2 :]

        output.write(body)
        headersList = headers.split("\r\n")
        return status, [s for s in headersList if s != "\n" and ":" in s], process_error
