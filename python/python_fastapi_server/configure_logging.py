"""Configures logging for the adaguc-server python wrapper"""

import sys


def configure_logging(logging):
    """Configures logging for the adaguc-server python wrapper"""
    logging.getLogger().handlers.clear()
    level =logging.WARN
    root = logging.getLogger()
    root.setLevel(level)
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(level)
    formatter = logging.Formatter(
        "applicationlog" + " %(asctime)s - %(name)s - %(levelname)s - %(message)s"
    )
    handler.setFormatter(formatter)
    root.addHandler(handler)
