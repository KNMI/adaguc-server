from os import environ
import os
from pathlib import Path
import subprocess

if "ADAGUC_CONFIG" in os.environ:
    os.environ.pop("ADAGUC_CONFIG")
if "QUERY_STRING" in os.environ:
    os.environ.pop("QUERY_STRING")

ADAGUC_PATH = Path(__file__).parent.parent.as_posix() + "/"
environ["ADAGUC_PATH"] = ADAGUC_PATH
environ["ADAGUC_DATASET_DIR"] = f"{ADAGUC_PATH}/data/config/datasets/"
environ["ADAGUC_DATA_DIR"] = f"{ADAGUC_PATH}/data/datasets/"
environ["ADAGUC_AUTOWMS_DIR"] = f"{ADAGUC_PATH}/data/datasets/"
environ["ADAGUC_DB"] = (
    "user=adaguc password=adaguc host=localhost dbname=adaguc_test port=54321"
)
environ["ADAGUC_ENABLELOGBUFFER"] = "FALSE"
environ["ADAGUC_TRACE_TIMINGS"] = "FALSE"
environ["ADAGUC_LOGFILE"] = f"{ADAGUC_PATH}/tests/log/adaguc-server.log"

environ["ADAGUC_TMP"] = f"{ADAGUC_PATH}/tests/tmp/"
environ["ADAGUC_FONT"] = f"{ADAGUC_PATH}/data/fonts/FreeSans.ttf"
environ["ADAGUC_ONLINERESOURCE"] = ""

Path(environ["ADAGUC_LOGFILE"]).parent.mkdir(parents=True, exist_ok=True)

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
