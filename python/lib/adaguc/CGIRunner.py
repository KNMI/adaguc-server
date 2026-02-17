"""
This Python script runs CGI scripts without an webserver.
Created by Maarten Plieger, KNMI -  2021-09-01
"""

import asyncio
import sys
from subprocess import PIPE
import os
import re
from typing import NamedTuple

HTTP_STATUSCODE_404_NOT_FOUND = 32  # Must be the same as in Definitions.h
HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY = 33  # Must be the same as in Definitions.h
HTTP_STATUSCODE_500_TIMEOUT = 34  # Not defined in C++, is generated from this file
SCAN_EXITCODE_FILENOMATCH = 64  #  File is available but does not match any of the available datasets
SCAN_EXITCODE_DATASETNOEXIST = 65  # The reason for this status code is that the dataset configuration file does not exist.
SCAN_EXITCODE_SCANERROR = 66  #  An error occured during scanning
SCAN_EXITCODE_FILENOEXIST = 67  # The file does not exist on the file system
SCAN_EXITCODE_TIMEOUT = 124  # The process timed out


ADAGUC_NUMPARALLELPROCESSES = int(os.getenv("ADAGUC_NUMPARALLELPROCESSES", "4"))
sem = asyncio.Semaphore(max(ADAGUC_NUMPARALLELPROCESSES, 2))  # At least two, to allow for me layer metadata update

ADAGUC_FORK_SOCKET_PATH = f"{os.getenv('ADAGUC_PATH')}/adaguc.socket"
ON_POSIX = "posix" in sys.builtin_module_names

MAX_PROC_TIMEOUT = int(os.getenv("ADAGUC_MAX_PROC_TIMEOUT", "300"))
print("Max procs: ", max(ADAGUC_NUMPARALLELPROCESSES, 2))


class AdagucResponse(NamedTuple):
    status_code: int
    process_output: bytearray


async def wait_socket_communicate(url, timeout) -> AdagucResponse:
    """
    If `socket_communicate` takes longer than `timeout`, we send a 500 timeout to the client.
    The adagucserver process will get cleaned up by the adagucserver parent process.
    """

    try:
        resp = await asyncio.wait_for(socket_communicate(url), timeout=timeout)
    except asyncio.exceptions.TimeoutError:
        return AdagucResponse(status_code=HTTP_STATUSCODE_500_TIMEOUT, process_output=None)
    except ConnectionRefusedError:
        # TODO: If for whatever reason socket communication to the fork server fails, should we fall back to regular process spawning?
        raise

    return resp


async def socket_communicate(url: str) -> AdagucResponse:
    """
    Connect to unix socket, send query string over socket, receive bytes from adagucserver.

    Last 4 bytes are status code.
    """

    process_output = bytearray()
    reader, writer = await asyncio.open_unix_connection(ADAGUC_FORK_SOCKET_PATH)
    writer.write(url.encode())
    await writer.drain()

    process_output = await reader.read()

    writer.close()
    await writer.wait_closed()

    # Status code is stored in the last 4 bytes from the received data
    status_code = int.from_bytes(process_output[-4:], sys.byteorder)
    process_output = process_output[:-4]
    return AdagucResponse(status_code=status_code, process_output=process_output)


async def wait_process_communicate(cmds, localenv, timeout) -> AdagucResponse:
    """
    If `process_communicate` takes longer than `timeout`, we send a 500 timeout to the client.
    We also have to manually kill the adagucserver process that we spawned before.
    """

    try:
        process = None
        resp = await asyncio.wait_for(process_communicate(process, cmds, localenv), timeout=timeout)
    except asyncio.exceptions.TimeoutError:
        if process:
            process.kill()
            await process.communicate()

        return AdagucResponse(status_code=HTTP_STATUSCODE_500_TIMEOUT, process_output=None)
    return resp


async def process_communicate(process, cmds, localenv) -> AdagucResponse:
    """
    Spawn a new adagucserver process, wait for output.
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

    async def run(self, cmds, url, output, env=[], path=None, isCGI=True, timeout=MAX_PROC_TIMEOUT):
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

        print("@@@@", url)

        # Only use fork server if ADAGUC_FORK_SOCKET_PATH is set and adaguc is not executed with extra arguments e.g. `--updatelayermetadata`
        use_fork = os.getenv("ADAGUC_FORK_SOCKET_PATH", None) and len(cmds) == 1

        async with sem:
            if use_fork:
                response = await wait_socket_communicate(url, timeout=timeout)
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
