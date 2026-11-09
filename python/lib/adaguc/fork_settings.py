import os
from pathlib import Path

ADAGUC_FORK_SOCKET_NAME = "adaguc.socket"
# Large enough for a STYLESHEET with many SymbolIntervals, url encoded in QUERY_STRING
MAX_FORK_REQUEST_BYTES = 65536
# The fork server ends every response with this marker and a 4-byte exit status, see FORK_EXIT_STATUS_MARKER in fork_server.cpp
FORK_EXIT_STATUS_MARKER = b"ADAGUC_EXIT:"
FORK_EXIT_STATUS_BYTES = 4
FORK_EXIT_TRAILER_BYTES = len(FORK_EXIT_STATUS_MARKER) + FORK_EXIT_STATUS_BYTES


def get_fork_socket_path() -> str:
    adaguc_path = os.getenv("ADAGUC_PATH", "./")
    return str(Path(adaguc_path) / ADAGUC_FORK_SOCKET_NAME)


def is_fork_enabled() -> bool:
    return os.getenv("ADAGUC_FORK_ENABLE", "FALSE") == "TRUE"
