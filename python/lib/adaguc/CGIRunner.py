"""
 This Python script runs CGI scripts without an webserver.
 Created by Maarten Plieger, KNMI -  2021-09-01
"""

import asyncio
import sys
from subprocess import PIPE, Popen, STDOUT, TimeoutExpired
from threading import Thread
import os
import io
import errno
import time
import chardet
from queue import Queue, Empty  # python 3.x
import re

HTTP_STATUSCODE_404_NOT_FOUND = 32  # Must be the same as in Definitions.h
HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY = 33  # Must be the same as in Definitions.h
HTTP_STATUSCODE_500_TIMEOUT = 34  # Not defined in C++, is generated from this file

ADAGUC_NUMPARALLELPROCESSES = int(os.getenv("ADAGUC_NUMPARALLELPROCESSES", "4"))
sem = asyncio.Semaphore(max(ADAGUC_NUMPARALLELPROCESSES, 2))  # At least two, to allow for me layer metadata update


import socket


class CGIRunner:
    """
    Run the CGI script with specified URL and environment. Stdout is captured and put in a BytesIO object provided in output
    """

    async def run(self, cmds, url, output, env=[], path=None, isCGI=True, timeout=300):
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

        # print("# QUERY STRING:", url)
        # print("# LOCAL ENV:", localenv)

        # Execute adaguc-server binary
        ON_POSIX = "posix" in sys.builtin_module_names
        async with sem:
            # process_output = ""

            # client = socket.socket(socket.AF_UNIX)
            # client.connect("/tmp/adaguc.socket")
            # client.send(url.encode())

            process_output = bytearray()
            # while data := client.recv(4096):
            #     # print(data)
            #     process_output.extend(data)

            # process_error = ""
            # status = 0

            # process_error = process_error.encode()

            reader, writer = await asyncio.open_unix_connection(
                f"{os.getenv('ADAGUC_PATH')}/adaguc.socke"
            )
            writer.write(url.encode())
            await writer.drain()

            process_output = await reader.read()

            print("Close the connection")
            writer.close()
            await writer.wait_closed()

            process_error = "".encode()

            # process = await asyncio.create_subprocess_exec(
            #     *cmds,
            #     stdout=PIPE,
            #     stderr=PIPE,
            #     env=localenv,
            #     close_fds=ON_POSIX,
            # )
            # try:
            #     (process_output, process_error) = await asyncio.wait_for(
            #         process.communicate(), timeout=timeout
            #     )
            # except asyncio.exceptions.TimeoutError:
            #     process.kill()
            #     await process.communicate()
            #     output.write(b"Adaguc server processs timed out")
            #     return HTTP_STATUSCODE_500_TIMEOUT, [], None
            # status = await process.wait()

        # Status code is stored in the last 4 bytes from the received data
        status = int.from_bytes(process_output[-4:], byteorder="little")
        print("@ status", status, process_output[-4:])

        # Split headers from body using a regex
        headersEndAt = -2
        headers = ""
        if isCGI == True:
            pattern = re.compile(b"\x0A\x0A")
            search = pattern.search(process_output)
            if search:
                headersEndAt = search.start()
                headers = (process_output[0 : headersEndAt - 1]).decode()
            else:
                output.write(
                    b"Error: No headers found in response from adaguc-server application, status was %d"
                    % status
                )
                return 1, [], None

        body = process_output[headersEndAt + 2 : -4]
        output.write(body)
        headersList = headers.split("\r\n")
        return status, [s for s in headersList if s != "\n" and ":" in s], process_error
