# pylint: disable=line-too-long
# pylint: disable=unused-variable
# pylint: disable=invalid-name
# pylint: disable=unused-argument

"""
 Run test for ADAGUC Feature functions
"""
import unittest
import os
from adaguc.ADAGUCFeatureFunctions import ADAGUCFeatureCombineNuts

from adaguc.AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ['ADAGUC_PATH']


class TestADAGUCFeatureFunctions(unittest.TestCase):
    """
    Run test for ADAGUC Feature functions
    """
    testresultspath = "{ADAGUC_PATH}/tests/testresults/TestADAGUCFeatureFunctions/"
    expectedoutputsspath = "{ADAGUC_PATH}/tests/expectedoutputs/TestADAGUCFeatureFunctions/"
    env = {'ADAGUC_CONFIG': ADAGUC_PATH +
           "/data/config/adaguc.autoresource.xml"}
    testresultspath = testresultspath.replace("{ADAGUC_PATH}/", ADAGUC_PATH)
    expectedoutputsspath = expectedoutputsspath.replace(
        "{ADAGUC_PATH}/", ADAGUC_PATH)

    AdagucTestTools().mkdir_p(testresultspath)

    def test_ADAGUCFeatureFunctions_testdatanc(self):
        """
        Check if adaguc feature functions still work as expected
        """
        AdagucTestTools().cleanTempDir()
        filenamencraster = "test_ADAGUCFeatureFunctions_testdata_raster.nc"
        filenamencpoint = "test_ADAGUCFeatureFunctions_testdata_point.nc"
        filenamecsv = "test_ADAGUCFeatureFunctions_testdata.csv"

        def progressCallback(message, percentage):
            # print "testCallback:: "+message+" "+str(percentage)
            return

        ADAGUCFeatureCombineNuts(
            featureNCFile="countries.geojson",
            #dataNCFile = "myfile.nc",
            dataNCFile="testdata.nc",
            bbox="0,50,10,55",
            time=None,
            variable="testdata",
            # variable="index",
            width=800,
            height=800,
            outncrasterfile=self.testresultspath + filenamencraster,
            outncpointfile=self.testresultspath + filenamencpoint,
            outcsvfile=self.testresultspath + filenamecsv,
            tmpFolderPath="/tmp",
            callback=progressCallback)
        #AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        # Comparing binary NetCDF is difficult
        # self.assertEqual(
        # AdagucTestTools().readfromfile(self.testresultspath + filenamencraster), AdagucTestTools().readfromfile(self.expectedoutputsspath + filenamencraster))
        self.assertEqual(
            AdagucTestTools().readfromfile(self.testresultspath + filenamecsv), AdagucTestTools().readfromfile(self.expectedoutputsspath + filenamecsv))
