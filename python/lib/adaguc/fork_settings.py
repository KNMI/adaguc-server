import os


ADAGUC_FORK_SOCKET_PATH = "adaguc.socket"


def is_fork_enabled() -> bool:
    return os.getenv("ADAGUC_FORK_ENABLE", "FALSE") == "TRUE"
