# pylint: disable=invalid-name,missing-function-docstring
"""
Pins the rendering of the legacy RenderMethod keywords of the removed CImgWarpBilinear renderer:
bilinear, shaded, contour and their combinations, plus the barb/vector variants. The generic warper
emulates these keywords for backwards compatibility; the modern way to configure the same output is
<RenderMethod>generic</RenderMethod> with RenderSettings / ShadeInterval / ContourLine / Vector, and
that is what the rest of the test suite uses. These tests only exist so the emulation keeps working.
"""

import os
import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import make_adaguc_env, run_adaguc_and_compare_image, update_db

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestsForLegacyBilinearRender:
    """
    Pins the output of the legacy CImgWarpBilinear RenderMethod keywords as emulated by the generic warper.
    """

    testresultspath = "testresults/TestsForLegacyBilinearRender/"
    expectedoutputsspath = "expectedoutputs/TestsForLegacyBilinearRender/"

    # Absolute: the harness only chdir()s into tests/ when the server is run, so a relative path here
    # would resolve against whatever directory pytest was started from.
    AdagucTestTools().mkdir_p(os.path.join(ADAGUC_PATH, "tests", testresultspath))

    def _env(self):
        env = make_adaguc_env("adaguc.tests.legacybilinearrender", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        return env

    # Contour labels are text, so those variants get the same antialiasing tolerance the other
    # contour tests use; plain raster variants are compared tightly.
    @pytest.mark.parametrize(
        ("render_method", "max_color_difference", "max_color_percentage"),
        [
            ("nearest", 1, 0.01),
            ("bilinear", 1, 0.01),
            ("shaded", 1, 0.01),
            ("contour", 30, 0.1),
            ("nearestcontour", 30, 0.1),
            ("shadedcontour", 30, 0.1),
        ],
    )
    def test_legacy_scalar_rendermethod(self, render_method: str, max_color_difference: int, max_color_percentage: float):
        run_adaguc_and_compare_image(
            self._env(),
            f"test_legacy_scalar_{render_method}.png",
            f"DATASET=adaguc.tests.legacybilinearrender&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=legacy_scalar%2F{render_method}&FORMAT=image/png&TRANSPARENT=TRUE",
            maxAllowedColorDifference=max_color_difference,
            maxAllowedColorPercentage=max_color_percentage,
        )

    @pytest.mark.parametrize(
        ("render_method", "max_color_difference", "max_color_percentage"),
        [
            ("shaded", 1, 0.01),
            ("shadedcontour", 30, 0.1),
        ],
    )
    def test_legacy_scalar_translucent_rendermethod(self, render_method: str, max_color_difference: int, max_color_percentage: float):
        """Translucent shade intervals must have a uniform alpha: no pixel may be blended twice at triangle seams."""
        run_adaguc_and_compare_image(
            self._env(),
            f"test_legacy_scalar_translucent_{render_method}.png",
            f"DATASET=adaguc.tests.legacybilinearrender&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=legacy_scalar_translucent%2F{render_method}&FORMAT=image/png&TRANSPARENT=TRUE",
            maxAllowedColorDifference=max_color_difference,
            maxAllowedColorPercentage=max_color_percentage,
        )

    @pytest.mark.parametrize(
        "render_method",
        ["barb", "vector", "barbshaded", "barbshadedcontour", "vectorshaded", "vectorshadedcontour"],
    )
    def test_legacy_wind_rendermethod(self, render_method: str):
        run_adaguc_and_compare_image(
            self._env(),
            f"test_legacy_wind_{render_method}.png",
            f"DATASET=adaguc.tests.legacybilinearrender&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind-hagl&WIDTH=512&HEIGHT=512&CRS=EPSG%3A3857&BBOX=-669486.4937633628,4378276.830549949,2026497.5462260891,7216268.010714424&STYLES=legacy_windbarbs_kts%2F{render_method}&FORMAT=image/png&TRANSPARENT=TRUE&DIM_wind_at_10m=10&time=2024-09-07T12%3A00%3A00Z&DIM_reference_time=2024-09-05T00%3A00%3A00Z",
            maxAllowedColorDifference=37,
            maxAllowedColorPercentage=0.1,
        )
