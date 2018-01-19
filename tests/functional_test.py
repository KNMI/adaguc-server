import unittest
from AdagucTests.TestWMS import TestWMS
suite = unittest.TestLoader().loadTestsFromTestCase(TestWMS)
unittest.TextTestRunner(verbosity=2).run(suite)
