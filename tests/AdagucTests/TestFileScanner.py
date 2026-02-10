# pylint: disable=invalid-name,missing-function-docstring
"""
This class contains tests to test the adaguc-server binary executable file. This is similar to black box testing, it tests the behaviour of the server software. It configures the server and checks if the response is OK.
"""

import os
import shutil
import time
import unittest
import subprocess
from adaguc.CGIRunner import SCAN_EXITCODE_DATASETNOEXIST, SCAN_EXITCODE_FILENOEXIST, SCAN_EXITCODE_FILENOMATCH, SCAN_EXITCODE_SCANERROR
import psycopg2
from adaguc.AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestFileScanner(unittest.TestCase):
    """
    TestFileScanner class to thest Web Map Service behaviour of adaguc-server.
    """

    testresultspath = "testresults/TestFileScanner/"
    expectedoutputsspath = "expectedoutputs/TestFileScanner/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_FileScanner_MultiVarCleanup(self):
        AdagucTestTools().cleanTempDir()
        AdagucTestTools().cleanPostgres()
        config = ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"
        # Copy the test file (it will be removed)
        filetocopy = os.path.join(
            os.getenv("ADAGUC_PATH", "/"),
            "data/datasets/arcus_multivar/uwcw_ha43_dini_testfile_multivar_5x6.nc",
        )
        filetoclean = os.path.join(
            os.getenv("ADAGUC_PATH", "/"),
            "data/datasets/arcus_multivar/uwcw_ha43_dini_testfile_multivar_5x6_clean.nc",
        )
        shutil.copyfile(filetocopy, filetoclean)
        myenv = self.env.copy()
        myenv["ADAGUCENV_ENABLECLEANUP"] = "false"
        args = [
            "--updatedb",
            "--config",
            config + ",adaguc.test.multivarcleanup.xml",
        ]
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=args,
            env=myenv,
            showLog=False,
            isCGI=False,
        )
        self.assertEqual(status, 0)
        # Now the file should be still there
        assert os.path.exists(filetoclean) is True
        # Now enable cleanup
        myenv = self.env.copy()
        myenv["ADAGUCENV_ENABLECLEANUP"] = "true"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=args,
            env=myenv,
            showLog=False,
            isCGI=False,
        )
        self.assertEqual(status, 0)
        # Check if tables are empty
        connection = psycopg2.connect(os.getenv("ADAGUC_DB")).cursor()
        connection.execute(
            "SELECT tablename from pathfiltertablelookup_v2_0_23 where filter = 'F_uwcw_ha43_dini_testfile_multivar_5x6_clean.*.nc$' and dimension != '';"
        )
        record = connection.fetchall()
        for row in record:
            table = row[0]
            connection.execute(f"SELECT * from {table};")
            rows = connection.fetchall()
            # There should be no files listed in the tables
            assert len(rows) is 0
        # In addition the file should be removed
        assert os.path.exists(filetoclean) is False

    def test_FileScanner_MultiVarCleanupFiledate(self):

        AdagucTestTools().cleanTempDir()
        AdagucTestTools().cleanPostgres()
        config = ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"
        # Copy the test file (it will be removed)
        filetocopy = os.path.join(
            os.getenv("ADAGUC_PATH", "/"),
            "data/datasets/arcus_multivar/uwcw_ha43_dini_testfile_multivar_5x6.nc",
        )
        filetoclean = os.path.join(
            os.getenv("ADAGUC_PATH", "/"),
            "data/datasets/arcus_multivar/uwcw_ha43_dini_testfile_multivar_5x6_clean.nc",
        )
        shutil.copyfile(filetocopy, filetoclean)
        current_time = time.time()
        creation_time = current_time - 3600
        modification_time = current_time - 2000
        os.utime(filetoclean, (creation_time, modification_time))

        myenv = self.env.copy()
        myenv["ADAGUCENV_ENABLECLEANUP"] = "false"
        myenv["ADAGUCENV_RETENTIONPERIOD"] = "PT10M"
        myenv["ADAGUCENV_RETENTIONTYPE"] = "filedate"
        args = [
            "--updatedb",
            "--config",
            config + ",adaguc.test.multivarcleanup.xml",
        ]
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=args,
            env=myenv,
            showLog=False,
            isCGI=False,
        )
        self.assertEqual(status, 0)
        # Now the file should be still there
        assert os.path.exists(filetoclean) is True
        # Now enable cleanup
        myenv = self.env.copy()
        myenv["ADAGUCENV_ENABLECLEANUP"] = "true"
        myenv["ADAGUCENV_RETENTIONPERIOD"] = "PT10M"
        myenv["ADAGUCENV_RETENTIONTYPE"] = "filedate"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=args,
            env=myenv,
            showLog=False,
            isCGI=False,
        )
        self.assertEqual(status, 0)
        # Check if tables are empty
        connection = psycopg2.connect(os.getenv("ADAGUC_DB")).cursor()
        connection.execute(
            "SELECT tablename from pathfiltertablelookup_v2_0_23 where filter = 'F_uwcw_ha43_dini_testfile_multivar_5x6_clean.*.nc$' and dimension != '';"
        )
        record = connection.fetchall()
        for row in record:
            table = row[0]
            connection.execute(f"SELECT * from {table};")
            rows = connection.fetchall()
            # There should be no files listed in the tables
            assert len(rows) is 0
        # In addition the file should be removed
        assert os.path.exists(filetoclean) is False

    def test_FileScanner_ExitCode_FileDoesNotExist(self):
        """
        Description: Exit code of scan process should return exit code SCAN_EXITCODE_FILENOEXIST
        The reason for this status code is that the file is not on the filesystem.
        """
        my_env = os.environ.copy()
        my_env["ADAGUC_CONFIG"] = ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"
        proc = subprocess.run(
            [ADAGUC_PATH + "/scripts/scan.sh", "-f", "doesnotexist"], capture_output=True, text=True, check=False, env=my_env
        )
        assert proc.returncode == SCAN_EXITCODE_FILENOEXIST

    def test_FileScanner_ExitCode_ConfigError(self):
        """
        Description: Exit code of scan process should return exit code SCAN_EXITCODE_DATASETNOEXIST
        The reason for this status code is that the dataset configuration file does not exist.
        """
        my_env = os.environ.copy()
        my_env["ADAGUC_CONFIG"] = ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"
        proc = subprocess.run(
            [ADAGUC_PATH + "/scripts/scan.sh", "-d", "not_existing_odataset"], capture_output=True, text=True, check=False, env=my_env
        )
        assert proc.returncode == SCAN_EXITCODE_DATASETNOEXIST

    def test_FileScanner_ExitCode_ScanError(self):
        """
        Description: Exit code of scan process should return exit code SCAN_EXITCODE_SCANERROR
        The reason for this status code is that the dataset configuration file contains errors and the scan process cannot continue.
        """
        my_env = os.environ.copy()
        my_env["ADAGUC_CONFIG"] = ADAGUC_PATH + "/data/config/adaguc.dataset.xml"
        proc = subprocess.run(
            [ADAGUC_PATH + "/scripts/scan.sh", "-d", "adaguc.tests.unknownlayertype.xml"],
            capture_output=True,
            text=True,
            check=False,
            env=my_env,
        )
        assert proc.returncode == SCAN_EXITCODE_SCANERROR

    def test_FileScanner_ExitCode_FileDoesExist(self):
        """
        Description: Exit code of scan process should return exit code SCAN_EXITCODE_FILENOMATCH
        The reason for this status code is that the file does not match any of the available datasets.
        """
        my_env = os.environ.copy()
        my_env["ADAGUC_CONFIG"] = ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"
        proc = subprocess.run(
            [ADAGUC_PATH + "/scripts/scan.sh", "-f", ADAGUC_PATH + "data/datasets/members.nc"],
            capture_output=True,
            text=True,
            check=False,
            env=my_env,
        )
        assert proc.returncode == SCAN_EXITCODE_FILENOMATCH
