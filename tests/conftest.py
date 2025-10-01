from os import environ
from pathlib import Path
import subprocess
import sys

adaguc_db = "user=adaguc password=adaguc host=localhost dbname=adaguc_test port=54321"

if "ADAGUC_CONFIG" in environ:
    environ.pop("ADAGUC_CONFIG")
if "QUERY_STRING" in environ:
    environ.pop("QUERY_STRING")

ADAGUC_PATH = Path(__file__).parent.parent.as_posix() + "/"
environ["ADAGUC_PATH"] = ADAGUC_PATH
environ["ADAGUC_DATASET_DIR"] = f"{ADAGUC_PATH}/data/config/datasets/"
environ["ADAGUC_DATA_DIR"] = f"{ADAGUC_PATH}/data/datasets/"
environ["ADAGUC_AUTOWMS_DIR"] = f"{ADAGUC_PATH}/data/datasets/"
environ["ADAGUC_DB"] = adaguc_db
environ["ADAGUC_ENABLELOGBUFFER"] = "FALSE"
environ["ADAGUC_TRACE_TIMINGS"] = "FALSE"
environ["ADAGUC_TMP"] = f"{ADAGUC_PATH}/tests/tmp/"
environ["ADAGUC_FONT"] = f"{ADAGUC_PATH}/data/fonts/FreeSans.ttf"
environ["ADAGUC_ONLINERESOURCE"] = ""

environ["ADAGUC_LOGFILE"] = f"{ADAGUC_PATH}/tests/log/adaguc-server.log"
Path(environ["ADAGUC_LOGFILE"]).parent.mkdir(parents=True, exist_ok=True)

# Verify testing db is running
try:
    subprocess.run(["psql", adaguc_db, "-c", "SELECT 1"])
except subprocess.CalledProcessError:
    print("No Adaguc test database available. Please run:\n")
    print("\tdocker compose -f Docker/docker-compose-test.yml up -Vd")
    sys.exit(1)

# Clear testing db and create initial database
sql = """
    DROP SCHEMA public CASCADE; CREATE SCHEMA public;
    DROP DATABASE IF EXISTS adaguc_test;
    CREATE DATABASE adaguc_test;
"""
subprocess.run(["psql", adaguc_db], input=sql, text=True, check=True)
