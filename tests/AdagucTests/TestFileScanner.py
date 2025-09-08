# pylint: disable=invalid-name,missing-function-docstring
"""
This class contains tests to test the adaguc-server binary executable file. This is similar to black box testing, it tests the behaviour of the server software. It configures the server and checks if the response is OK.
"""
import os
import shutil
import unittest
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
        if os.getenv("ADAGUC_DB").endswith(".db"):
            print("SKIP: Only PSQL")
            return
        AdagucTestTools().cleanTempDir()
        AdagucTestTools().cleanPostgres()
        config = ADAGUC_PATH + "data/config/adaguc.tests.dataset.xml"
        # Copy the test file (it will be removed)
        filetocopy = os.path.join(os.getenv("ADAGUC_PATH","/"),"data/datasets/arcus_multivar/uwcw_ha43_dini_testfile_multivar_5x6.nc")
        filetoclean = os.path.join(os.getenv("ADAGUC_PATH","/"),"data/datasets/arcus_multivar/uwcw_ha43_dini_testfile_multivar_5x6_clean.nc")
        shutil.copyfile(filetocopy, filetoclean)
        myenv=self.env.copy()
        myenv['ADAGUCENV_ENABLECLEANUP']="false"
        args=[
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
        myenv=self.env.copy()
        myenv['ADAGUCENV_ENABLECLEANUP']="true"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=args,
            env=myenv,
            showLog=False,
            isCGI=False,
        )
        self.assertEqual(status, 0)
        # Check if tables are empty
        connection = psycopg2.connect(os.getenv("ADAGUC_DB")).cursor()
        connection.execute("SELECT tablename from pathfiltertablelookup_v2_0_23 where filter = 'F_uwcw_ha43_dini_testfile_multivar_5x6_clean.*.nc$' and dimension != '';")
        record = connection.fetchall()
        for row in record:
            table = row[0]
            connection.execute(f"SELECT * from {table};")
            rows = connection.fetchall()
            # There should be no files listed in the tables
            assert len(rows) is 0
        # In addition the file should be removed
        assert os.path.exists(filetoclean) is False
                