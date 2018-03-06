import unittest
import sys
from AdagucTests.TestWMS import TestWMS
from AdagucTests.TestWMSDocumentCache import TestWMSDocumentCache
from AdagucTests.TestOpenDAPServer import TestOpenDAPServer
from AdagucTests.TestWMSTiling import TestWMSTiling
from AdagucTests.TestWMSPolylineRenderer import TestWMSPolylineRenderer

testsuites=[];
testsuites.append(unittest.TestLoader().loadTestsFromTestCase(TestWMS))
testsuites.append(unittest.TestLoader().loadTestsFromTestCase(TestWMSDocumentCache))
testsuites.append(unittest.TestLoader().loadTestsFromTestCase(TestOpenDAPServer))
testsuites.append(unittest.TestLoader().loadTestsFromTestCase(TestWMSPolylineRenderer))
#testsuites.append(unittest.TestLoader().loadTestsFromTestCase(TestWMSTiling))
result=unittest.TextTestRunner(verbosity=2).run(unittest.TestSuite(testsuites))

sys.exit(not result.wasSuccessful())
