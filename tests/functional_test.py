import unittest
from AdagucTests.TestWMS import TestWMS
from AdagucTests.TestOpenDAP import TestOpenDAP

suite = unittest.TestLoader().loadTestsFromTestCase(TestWMS)
unittest.TextTestRunner(verbosity=2).run(suite)

suite = unittest.TestLoader().loadTestsFromTestCase(TestOpenDAP)
unittest.TextTestRunner(verbosity=2).run(suite)
