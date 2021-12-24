import os
from adaguc.runAdaguc import runAdaguc
import logging

logger = logging.getLogger(__name__)


def setupAdaguc(displayLogging=True):
  # Check if environment is specified
  if not os.getenv('ADAGUC_PATH'):
    print("ADAGUC_PATH not set")
    exit(1)
  if not os.getenv('ADAGUC_DATASET_DIR'):
    print("ADAGUC_DATASET_DIR not set")
    exit(1)
  if not os.getenv('ADAGUC_DATA_DIR'):
    print("ADAGUC_DATA_DIR not set")
    exit(1)
  if not os.getenv('ADAGUC_AUTOWMS_DIR'):
    print("ADAGUC_AUTOWMS_DIR not set")
    exit(1)

  # Get the location of the binaries
  adagucServerHome = os.getenv('ADAGUC_PATH') + '/'
  if adagucServerHome is None or len(adagucServerHome) < 1:
    print('Your ADAGUC_PATH environment variable is not set! It should point to the adaguc-server folder.')
    exit(1)

  # Get the location of the adaguc-server configuration file
  adagucServerConfig = os.getenv('ADAGUC_CONFIG', adagucServerHome +
                                 "/python/lib/adaguc/adaguc-server-config-python-postgres.xml")
  if adagucServerConfig is None or len(adagucServerConfig) < 1:
    print('Your ADAGUC_CONFIG environment variable is not set! It should point to a adaguc-server config file.')
    exit(1)

  if displayLogging == True:
    logger.info("Using config file %s" % adagucServerConfig)

  adagucInstance = runAdaguc()
  adagucInstance.setAdagucPath(adagucServerHome)
  adagucInstance.setConfiguration(adagucServerConfig)
  return adagucInstance
