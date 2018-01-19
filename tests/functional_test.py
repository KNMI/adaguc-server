import unittest
from AdagucTests.TestWMS import TestWMS
from AdagucTests.TestOpenDAPServer import TestOpenDAPServer

#suite = unittest.TestLoader().loadTestsFromTestCase(TestWMS)
#unittest.TextTestRunner(verbosity=2).run(suite)

suite = unittest.TestLoader().loadTestsFromTestCase(TestOpenDAPServer)
unittest.TextTestRunner(verbosity=2).run(suite)
