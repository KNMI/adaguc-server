import filecmp
from os import environ, getenv
from pathlib import Path
import subprocess
import sys
import resource


import pytest


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
    maintenance_db = f"user=adaguc password=adaguc host={DB_HOST} dbname=postgres port=54321"

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
resource.setrlimit(resource.RLIMIT_CORE, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))


DATASETS_LOADED = set()


def clean_temp_dir():
    """
    Cleans the temporary folder
    """
    from adaguc.AdagucTestTools import AdagucTestTools

    AdagucTestTools().cleanTempDir()


def make_adaguc_autoresource_env(testresultspath_actual: str = "", testresultspath_expected: str = "") -> dict:
    """
    Return a dictionary with ADAGUC_CONFIG as key, and the value set to a default dataset + a given dataset.
    Format is ready to pass into update_db() and runADAGUCServer().
    """
    clean_temp_dir()
    ADAGUC_PATH = environ["ADAGUC_PATH"]
    config = f"{ADAGUC_PATH}/data/config/adaguc.autoresource.xml"
    return {"ADAGUC_CONFIG": config, "ADAGUC_TESTPATH_ACTUAL": testresultspath_actual, "ADAGUC_TESTPATH_EXPECTED": testresultspath_expected}


def make_adaguc_env(dataset: str = "", testresultspath_actual: str = "", testresultspath_expected: str = "") -> dict:
    """
    Return a dictionary with ADAGUC_CONFIG as key, and the value set to a default dataset + a given dataset.
    Format is ready to pass into update_db() and runADAGUCServer().
    """
    clean_temp_dir()
    ADAGUC_PATH = environ["ADAGUC_PATH"]

    config = f"{ADAGUC_PATH}/data/config/adaguc.tests.dataset.xml"

    if dataset:
        if not dataset.endswith(".xml"):
            dataset = dataset + ".xml"
        config = f"{config},{dataset}"

    return {"ADAGUC_CONFIG": config, "ADAGUC_TESTPATH_ACTUAL": testresultspath_actual, "ADAGUC_TESTPATH_EXPECTED": testresultspath_expected}


def reset_datasetsloaded():
    DATASETS_LOADED.clear()


def update_db(adaguc_env: dict, force_update: bool = False):
    """
    Run `--updatedb` if it has not been run before for the given environment. When executed multiple times
    with the same environment, no additional `--updatedb` calls will be made, unless `force_update` is enabled.

    - `adaguc_env`: Environment configuration with `ADAGUC_CONFIG` as key.
    - `force_update`: Always execute `force_update`, even if it has run before.
    """

    adaguc_config = adaguc_env.get("ADAGUC_CONFIG")

    if not force_update and adaguc_config in DATASETS_LOADED:
        return

    from adaguc.AdagucTestTools import AdagucTestTools

    args = ["--updatedb"]
    if adaguc_env:
        args += ["--config", adaguc_config]

    status, *_ = AdagucTestTools().runADAGUCServer(args=args, env=adaguc_env, isCGI=False)
    assert status == 0

    DATASETS_LOADED.add(adaguc_config)


def run_adaguc_and_compare_image(env, filename, query_string):
    """
    Compares a image for a given query_string to a filename in the expected testresult folder.
    """
    from adaguc.AdagucTestTools import AdagucTestTools

    status, data, headers = AdagucTestTools().runADAGUCServer(
        query_string,
        env=env,
    )
    assert status == 0

    AdagucTestTools().writetofile(env["ADAGUC_TESTPATH_ACTUAL"] + filename, data.getvalue())
    AdagucTestTools().compareImage(env["ADAGUC_TESTPATH_ACTUAL"] + filename, env["ADAGUC_TESTPATH_EXPECTED"] + filename)


def run_adaguc_and_compare_getcapabilities(env, filename, query_string):
    """
    Compares a XML GetCapabilities for a given query_string to a filename in the expected testresult folder.
    """
    from adaguc.AdagucTestTools import AdagucTestTools

    status, data, headers = AdagucTestTools().runADAGUCServer(
        query_string,
        env=env,
    )
    assert status == 0

    AdagucTestTools().writetofile(env["ADAGUC_TESTPATH_ACTUAL"] + filename, data.getvalue())
    AdagucTestTools().compareGetCapabilitiesXML(env["ADAGUC_TESTPATH_ACTUAL"] + filename, env["ADAGUC_TESTPATH_EXPECTED"] + filename)


def run_adaguc_and_compare_file(env, filename, query_string):
    """
    Compares a file for a given query_string to a filename in the expected testresult folder.
    """
    from adaguc.AdagucTestTools import AdagucTestTools

    status, data, headers = AdagucTestTools().runADAGUCServer(
        query_string,
        env=env,
    )
    assert status == 0

    AdagucTestTools().writetofile(env["ADAGUC_TESTPATH_ACTUAL"] + filename, data.getvalue())
    file_is_equal = filecmp.cmp(env["ADAGUC_TESTPATH_ACTUAL"] + filename, env["ADAGUC_TESTPATH_EXPECTED"] + filename, shallow=False)
    assert file_is_equal


def run_adaguc_and_compare_json(env, filename, query_string, expected_status_code=0):
    """
    Compares a json for a given query_string to a filename in the expected testresult folder.
    """
    from adaguc.AdagucTestTools import AdagucTestTools

    status, data, headers = AdagucTestTools().runADAGUCServer(
        query_string,
        env=env,
    )
    assert status == expected_status_code
    AdagucTestTools().writetofile(env["ADAGUC_TESTPATH_ACTUAL"] + filename, data.getvalue())
    AdagucTestTools().compareJson(env["ADAGUC_TESTPATH_ACTUAL"] + filename, env["ADAGUC_TESTPATH_EXPECTED"] + filename)
