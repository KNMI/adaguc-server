# pylint: disable=invalid-name

"""Configures logging for the adaguc-server python wrapper"""
import sys
from functools import wraps

def run_once(f):
    """Runs a function (successfully) only once.
    The running can be reset by setting the `has_run` attribute to False
    """
    @wraps(f)
    def wrapper(*args, **kwargs):
        if not f.has_run:
            f.has_run = True
            return f(*args, **kwargs)

    f.has_run = False
    return wrapper


@run_once
def configure_logging(logging):
    """Configures logging for the adaguc-server python wrapper"""
    root = logging.getLogger()
    root.setLevel(logging.DEBUG)
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(logging.DEBUG)
    formatter = logging.Formatter(
        "applicationlog %(asctime)s - %(name)s - %(levelname)s - %(message)s"
    )
    handler.setFormatter(formatter)
    root.addHandler(handler)
