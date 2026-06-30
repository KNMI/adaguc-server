import os
from pathlib import Path

ADAGUC_FORK_SOCKET_NAME = "adaguc.socket"


def get_fork_socket_path() -> str:
    adaguc_path = os.getenv("ADAGUC_PATH", "./")
    return str(Path(adaguc_path) / ADAGUC_FORK_SOCKET_NAME)


def is_fork_enabled() -> bool:
    return os.getenv("ADAGUC_FORK_ENABLE", "FALSE") == "TRUE"
