"""Configures logging for the adaguc-server python wrapper"""

import os
import sys

log_mode = os.getenv("ADAGUC_ENABLELOGBUFFER", "DISABLELOGGING").strip()

DEBUG_LEVEL_TRACE = 35


def configure_logging(logging):
    """Configures logging for the adaguc-server python wrapper"""
    logging.getLogger().handlers.clear()
    level = logging.INFO
    if (log_mode == "DISABLELOGGING"):
        level = logging.WARN
    root = logging.getLogger()
    root.setLevel(level)
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(level)
    formatter = logging.Formatter(
        "applicationlog" + " %(asctime)s - %(name)s - %(levelname)s - %(message)s"
    )
    #Only used to log tracing info
    logging.addLevelName(DEBUG_LEVEL_TRACE, "TRACE")
    def debugv(self, message, *args, **kws):
        if self.isEnabledFor(DEBUG_LEVEL_TRACE):
            # Yes, logger takes its '*args' as 'args'.
            self._log(DEBUG_LEVEL_TRACE, message, args, **kws) 
    logging.Logger.debugv = debugv
    handler.setFormatter(formatter)
   
    root.addHandler(handler)
