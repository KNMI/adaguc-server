"""
Setup adaguc
"""
import logging
import os
import sys

from adaguc.runAdaguc import runAdaguc

logger = logging.getLogger(__name__)


def setup_adaguc(displaylogging=True):
    """
    Setup adaguc
    """
    # Check if environment is specified
    if not os.getenv("ADAGUC_PATH"):
        print("ADAGUC_PATH not set")
        sys.exit(1)
    if not os.getenv("ADAGUC_DATASET_DIR"):
        print("ADAGUC_DATASET_DIR not set")
        sys.exit(1)
    if not os.getenv("ADAGUC_DATA_DIR"):
        print("ADAGUC_DATA_DIR not set")
        sys.exit(1)
    if not os.getenv("ADAGUC_AUTOWMS_DIR"):
        print("ADAGUC_AUTOWMS_DIR not set")
        sys.exit(1)

    # Get the location of the binaries
    adaguc_server_home = os.getenv("ADAGUC_PATH") + "/"
    if adaguc_server_home is None or len(adaguc_server_home) < 1:
        print(
            "Your ADAGUC_PATH environment variable is not set! "
            "It should point to the adaguc-server folder."
        )
        sys.exit(1)

    # Get the location of the adaguc-server configuration file
    adaguc_server_config = os.getenv(
        "ADAGUC_CONFIG",
        adaguc_server_home
        + "/python/lib/adaguc/adaguc-server-config-python-postgres.xml",
    )
    if adaguc_server_config is None or len(adaguc_server_config) < 1:
        print(
            "Your ADAGUC_CONFIG environment variable is not set!"
            "It should point to a adaguc-server config file."
        )
        sys.exit(1)

    # if displaylogging is True:
    #     logger.info("Using config file %s", adaguc_server_config)

    adaguc_instance = runAdaguc()
    adaguc_instance.setAdagucPath(adaguc_server_home)
    adaguc_instance.setConfiguration(adaguc_server_config)
    return adaguc_instance
