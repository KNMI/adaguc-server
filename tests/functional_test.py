"""
  Running functional tests for adaguc-server.
  The adaguc-server executable is ran with various settings and configuration files.
  Results are checked against expected results and should not differ.
"""

import unittest
import sys
from AdagucTests.TestWMS import TestWMS
from AdagucTests.TestWCS import TestWCS
from AdagucTests.TestWMSSLD import TestWMSSLD
from AdagucTests.TestWMSDocumentCache import TestWMSDocumentCache
from AdagucTests.TestOpenDAPServer import TestOpenDAPServer
from AdagucTests.TestWMSTiling import TestWMSTiling
from AdagucTests.TestWMSPolylineRenderer import TestWMSPolylineRenderer
from AdagucTests.TestADAGUCFeatureFunctions import TestADAGUCFeatureFunctions
from AdagucTests.TestCSV import TestCSV
from AdagucTests.TestGeoJSON import TestGeoJSON
from AdagucTests.TestMetadataService import TestMetadataService
from AdagucTests.TestWMSVolScan import TestWMSVolScan
from AdagucTests.TestWMSPolylineLabel import TestWMSPolylineLabel
from AdagucTests.TestDataPostProcessor import TestDataPostProcessor
from AdagucTests.TestWMSTimeHeightProfiles import TestWMSTimeHeightProfiles

suites = []
TestLoader = unittest.TestLoader
suites.append(TestLoader().loadTestsFromTestCase(TestWMS))
suites.append(TestLoader().loadTestsFromTestCase(TestWCS))
suites.append(TestLoader().loadTestsFromTestCase(TestWMSSLD))
suites.append(TestLoader().loadTestsFromTestCase(TestWMSDocumentCache))
suites.append(TestLoader().loadTestsFromTestCase(TestOpenDAPServer))
suites.append(TestLoader().loadTestsFromTestCase(TestWMSPolylineRenderer))
suites.append(TestLoader().loadTestsFromTestCase(TestWMSTiling))
suites.append(TestLoader().loadTestsFromTestCase(TestADAGUCFeatureFunctions))
suites.append(TestLoader().loadTestsFromTestCase(TestCSV))
suites.append(TestLoader().loadTestsFromTestCase(TestGeoJSON))
suites.append(TestLoader().loadTestsFromTestCase(TestMetadataService))
suites.append(TestLoader().loadTestsFromTestCase(TestWMSVolScan))
suites.append(TestLoader().loadTestsFromTestCase(TestWMSPolylineLabel))
suites.append(TestLoader().loadTestsFromTestCase(TestDataPostProcessor))
suites.append(TestLoader().loadTestsFromTestCase(TestWMSTimeHeightProfiles))
result = unittest.TextTestRunner(verbosity=2).run(unittest.TestSuite(suites))


sys.exit(not result.wasSuccessful())
