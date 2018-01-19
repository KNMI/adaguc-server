import unittest
from AdagucTests.TestWMS import TestWMS
from AdagucTests.TestOpenDAPServer import TestOpenDAPServer
from AdagucTests.TestWMSTiling import TestWMSTiling

testsuites=[];
testsuites.append(unittest.TestLoader().loadTestsFromTestCase(TestWMS))
testsuites.append(unittest.TestLoader().loadTestsFromTestCase(TestOpenDAPServer))
#testsuites.append(unittest.TestLoader().loadTestsFromTestCase(TestWMSTiling))
unittest.TextTestRunner(verbosity=2).run(unittest.TestSuite(testsuites))

#suite = unittest.TestLoader().loadTestsFromTestCase(TestOpenDAPServer)
#unittest.TextTestRunner(verbosity=2).run(suite)
