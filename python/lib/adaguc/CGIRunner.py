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

ADAGUC_NUMPARALLELPROCESSES = os.getenv("ADAGUC_NUMPARALLELPROCESSES", "4")
sem = asyncio.Semaphore(int(ADAGUC_NUMPARALLELPROCESSES))


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

        # Execute adaguc-server binary
        ON_POSIX = "posix" in sys.builtin_module_names
        async with sem:
            process = await asyncio.create_subprocess_exec(
                *cmds,
                stdout=PIPE,
                stderr=PIPE,
                env=localenv,
                close_fds=ON_POSIX,
            )
            try:
                (process_output, process_error) = await asyncio.wait_for(
                    process.communicate(), timeout=timeout
                )
            except asyncio.exceptions.TimeoutError:
                process.kill()
                await process.communicate()
                output.write(b"TimeOut")
                return 1, [], None
            status = await process.wait()

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

        body = process_output[headersEndAt + 2 :]
        output.write(body)
        headersList = headers.split("\r\n")
        return status, [s for s in headersList if s != "\n" and ":" in s], process_error
