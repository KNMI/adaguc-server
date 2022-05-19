import os
import os.path
import unittest
from adaguc.AdagucTestTools import AdagucTestTools
import datetime

ADAGUC_PATH = os.environ['ADAGUC_PATH']
class TestOGCAPIFeatures(unittest.TestCase):
    testresultspath = "testresults/TestOGCAPI/"
    expectedoutputsspath = "expectedoutputs/TestOGCAPI/"
    env = {'ADAGUC_CONFIG': ADAGUC_PATH +
           "/data/config/adaguc.testogcapi.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def setenviron(self):
        os.environ['ADAGUC_AUTOWMS']=os.path.join(os.environ['ADAGUC_PATH'],'adaguc-autowms')
        os.environ['ADAGUC_DATASET_DIR']=os.path.join(os.environ['ADAGUC_PATH'],'adaguc-datasets')
        os.environ['ADAGUC_DATA_DIR']=os.path.join(os.environ['ADAGUC_PATH'], 'adaguc-data')
        os.environ['ADAGUC_ENABLELOGBUFFER']='FALSE'
        os.environ['ADAGUC_FONT']=os.path.join(os.environ['ADAGUC_PATH'], 'data/fonts/Roboto-Medium.ttf')

    def start_ogcapi_server(self):
        return None

    def stop_ogcapi_server():
        return None

    def test_WMSCMDUpdateDB(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ['ADAGUC_PATH']
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', ADAGUC_PATH + '/data/config/adaguc.testogcapi.xml'], isCGI=False, showLogOnError=False)
        self.assertEqual(status, 0)

        filename = "test_WMSGetCapabilities_timeseries_twofiles"
        status, data, headers = AdagucTestTools().runADAGUCServer("SERVICE=WMS&request=getcapabilities",
                                                                  {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.timeseries.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(
            self.testresultsparequests
    def test_(self):
        AdagucTestTools().cleanTempDir()
        self.start_ogcapi_server()
        ADAGUC_PATH = os.environ['ADAGUC_PATH']
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', ADAGUC_PATH + '/data/config/adaguc.testogcapi.xml'], isCGI=False, showLogOnError=False)
        self.assertEqual(status, 0)

        self.stop_ogcapi_server()

        return None