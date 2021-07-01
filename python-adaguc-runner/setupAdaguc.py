import os
from adaguc.runAdaguc import runAdaguc

def setupAdaguc():
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

  # Get the location of the binaries
  adagucServerHome = os.getenv('ADAGUC_PATH') +'/'
  if adagucServerHome is None or len(adagucServerHome) < 1:
    print('Your ADAGUC_PATH environment variable is not set! It should point to the adaguc-server folder.')
    exit(1)

  adagucInstance = runAdaguc()
  adagucInstance.setAdagucPath(adagucServerHome)
  adagucInstance.setConfiguration(adagucServerHome + "/data/python/adaguc/adaguc-server-config-python-postgres.xml")
  return adagucInstance

