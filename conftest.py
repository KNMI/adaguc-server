from os import environ
import os
from pathlib import Path
import subprocess
import sys

import pytest

ADAGUC_PATH = Path(__file__).parent.as_posix()
environ["ADAGUC_PATH"] = ADAGUC_PATH
environ["ADAGUC_DATASET_DIR"] = f"{ADAGUC_PATH}/data/config/datasets/"
environ["ADAGUC_DATA_DIR"] = f"{ADAGUC_PATH}/data/datasets/"
environ["ADAGUC_AUTOWMS_DIR"] = f"{ADAGUC_PATH}/data/datasets/"
environ["ADAGUC_CONFIG"] = (
    f"{ADAGUC_PATH}/python/lib/adaguc/adaguc-server-config-python-postgres.xml"
)
# environ["ADAGUC_DB"] = "user=adaguc password=adaguc host=localhost dbname=adaguc_test"
environ["ADAGUC_DB"] = (
    "user=adaguc password=adaguc host=host.docker.internal dbname=adaguc_test"
)
environ["ADAGUC_ENABLELOGBUFFER"] = "FALSE"
environ["ADAGUCENV_ENABLECLEANUP"] = "FALSE"
environ["ADAGUC_LOGFILE"] = f"{ADAGUC_PATH}/tests/log/adaguc-server.log"

environ["ADAGUC_TMP"] = f"{ADAGUC_PATH}/tests/tmp/"
environ["ADAGUC_FONT"] = f"{ADAGUC_PATH}/data/fonts/FreeSans.ttf"
environ["ADAGUC_ONLINERESOURCE"] = ""

Path(environ["ADAGUC_LOGFILE"]).parent.mkdir(parents=True, exist_ok=True)


sys.path.append(f"{ADAGUC_PATH}python/lib/")


@pytest.fixture(scope="session", autouse=True)
def global_setup():
    adaguc_db = os.getenv("ADAGUC_DB", None)
    if adaguc_db and not adaguc_db.endswith(".db"):
        subprocess.run(
            [
                "psql",
                adaguc_db,
                "-c",
                "DROP SCHEMA public CASCADE; CREATE SCHEMA public;",
            ]
        )
