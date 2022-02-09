"""
  Running integration tests for adaguc-server.
  The adaguc-server executable is ran with various settings and configuration files.
  Results are checked against expected results and should not differ.
"""
import unittest
import sys
from IntegrationTests.testLib import TestWMSGetCapabilities

suites = []
TestLoader = unittest.TestLoader
suites.append(TestLoader().loadTestsFromTestCase(TestWMSGetCapabilities))
result = unittest.TextTestRunner(verbosity=2).run(unittest.TestSuite(suites))


sys.exit(not result.wasSuccessful())
