from os import environ, getenv
from pathlib import Path
import subprocess
import sys
import resource


def determine_db_host():
    test_env = getenv("TEST_IN_CONTAINER", "").strip()

    if test_env == "github_build":
        return "localhost"
    elif test_env == "local_build":
        return "host.docker.internal"
    else:
        return "localhost"


DB_HOST = determine_db_host()
ADAGUC_DB = f"user=adaguc password=adaguc host={DB_HOST} dbname=adaguc_test port=54321"


def set_env_vars():
    if "ADAGUC_CONFIG" in environ:
        environ.pop("ADAGUC_CONFIG")
    if "QUERY_STRING" in environ:
        environ.pop("QUERY_STRING")

    ADAGUC_PATH = Path(__file__).parent.parent.as_posix() + "/"
    environ["ADAGUC_PATH"] = ADAGUC_PATH
    environ["ADAGUC_DATASET_DIR"] = f"{ADAGUC_PATH}/data/config/datasets/"
    environ["ADAGUC_DATA_DIR"] = f"{ADAGUC_PATH}/data/datasets/"
    environ["ADAGUC_AUTOWMS_DIR"] = f"{ADAGUC_PATH}/data/datasets/"
    environ["ADAGUC_DB"] = ADAGUC_DB
    environ["ADAGUC_ENABLELOGBUFFER"] = "FALSE"
    environ["ADAGUC_TRACE_TIMINGS"] = "FALSE"
    environ["ADAGUC_TMP"] = f"{ADAGUC_PATH}/tests/tmp/"
    environ["ADAGUC_FONT"] = f"{ADAGUC_PATH}/data/fonts/FreeSans.ttf"
    environ["ADAGUC_ONLINERESOURCE"] = ""
    environ["ADAGUC_LOGFILE"] = f"{ADAGUC_PATH}/tests/log/adaguc-server.log"
    Path(environ["ADAGUC_LOGFILE"]).parent.mkdir(parents=True, exist_ok=True)


def ensure_adaguc_test_db():
    # We can only drop/recreate adaguc_test when connected to a different dbname
    maintenance_db = (
        f"user=adaguc password=adaguc host={DB_HOST} dbname=postgres port=54321"
    )

    # Verify testing db is running
    try:
        subprocess.run(["psql", maintenance_db, "-c", "SELECT 1"], check=True)
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
    subprocess.run(["psql", maintenance_db], input=sql, text=True, check=True)


set_env_vars()
ensure_adaguc_test_db()

# Set coredump size to unlimited, equivalent of `ulimit -c unlimited`
resource.setrlimit(
    resource.RLIMIT_CORE, (resource.RLIM_INFINITY, resource.RLIM_INFINITY)
)
