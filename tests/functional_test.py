import unittest
from AdagucTests.TestWMS import TestWMS
from AdagucTests.TestOpenDAPServer import TestOpenDAPServer
from AdagucTests.TestWMSTiling import TestWMSTiling
from AdagucTests.TestWMSPolylineRenderer import TestWMSPolylineRenderer

testsuites=[];
testsuites.append(unittest.TestLoader().loadTestsFromTestCase(TestWMS))
testsuites.append(unittest.TestLoader().loadTestsFromTestCase(TestOpenDAPServer))
testsuites.append(unittest.TestLoader().loadTestsFromTestCase(TestWMSPolylineRenderer))
unittest.TextTestRunner(verbosity=2).run(unittest.TestSuite(testsuites))


#testsuites.append(unittest.TestLoader().loadTestsFromTestCase(TestWMSTiling))
