# pylint: disable=invalid-name,missing-function-docstring
"""
This class contains tests to test the adaguc-server binary executable file. This is similar to black box testing, it tests the behaviour of the server software. It configures the server and checks if the response is OK.
"""

import os
from adaguc.AdagucTestTools import AdagucTestTools
import pytest

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestGraticules:
    """
    TestGraticules class to test WMS with <Layer type="grid">
    """

    testresultspath = "testresults/TestGraticules/"
    expectedoutputsspath = "expectedoutputs/TestGraticules/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    @pytest.mark.parametrize(
        ("layer_name"),
        [
            ("grid1"),
            ("grid10"),
        ],
    )
    def test_Graticules(self, layer_name: str):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml,adaguc.tests.graticules.xml"
        env = {"ADAGUC_CONFIG": config}

        status, data, _ = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        assert status == 0

        filename = f"test_Graticules_{layer_name}.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            f"DATASET=adaguc.tests.graticules&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS={layer_name}&WIDTH=1143&HEIGHT=1135&CRS=EPSG%3A3857&BBOX=-19133920.704845816,-19000000,19133920.704845816,19000000&STYLES=&FORMAT=image/png&TRANSPARENT=TRUE&&time=2026-01-29T15%3A20%3A00Z",
            env=env,
            showLog=True,
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        assert status == 0
        assert data.getvalue() == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)
