import os
import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import (
    make_adaguc_env,
    run_adaguc_and_compare_image,
    update_db,
)

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestContour:
    testresultspath = "testresults/TestContour/"
    expectedoutputsspath = "expectedoutputs/TestContour/"
    AdagucTestTools().mkdir_p(testresultspath)

    @pytest.mark.parametrize(
        ("filetocheck", "styles", "width", "height", "crs", "bbox", "tail"),
        [
            pytest.param(
                "test_contour_air_temperature_hagl.png",
                "temperature_contour",
                575,
                668,
                "EPSG:32661",
                "-3384546.659457,1075289.936021,-713528.390451,3374842.088145",
                "TRANSPARENT=FALSE",
                id="plain",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_2x.png",
                "temperature_contour",
                1150,
                1336,
                "EPSG:32661",
                "-3384546.659457,1075289.936021,-713528.390451,3374842.088145",
                "TRANSPARENT=FALSE",
                # A finer destination grid changes where contour line tracing starts and how far
                # apart distance-field pixels land, so this is a useful second data point for the
                # same gap-tracing code at a different pixel density.
                id="2x",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_2x_shaded_extrasmooth.png",
                "temperature_shaded_extrasmooth",
                1150,
                1336,
                "EPSG:32661",
                "-3384546.659457,1075289.936021,-713528.390451,3374842.088145",
                "TRANSPARENT=FALSE&SHOWLEGEND=TRUE",
                # The 25-class, -52 to 48 multicolor legend instead of plain contour lines, a
                # heavier SmoothingFilter of 6, and blue contour lines every 2 degrees overlaid.
                id="2x_shaded_extrasmooth",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_smoothed.png",
                "temperature_contour_smooth",
                1150,
                1336,
                "EPSG:32661",
                "-3384546.659457,1075289.936021,-713528.390451,3374842.088145",
                "TRANSPARENT=FALSE",
                # The unsmoothed field has real grid-scale noise (5.5km convection-permitting
                # model output), which turns into thousands of tiny disconnected contour loops
                # rather than a bug - SmoothingFilter is the normal way to get readable isotherms.
                id="smoothed",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_dashed.png",
                "temperature_contour_dashed",
                1150,
                1336,
                "EPSG:32661",
                "-3384546.659457,1075289.936021,-713528.390451,3374842.088145",
                "TRANSPARENT=TRUE",
                # Two ContourLine definitions with different intervals and dash patterns, drawgrid
                # disabled - the same pattern as the production "pressure_cwk" isobar style.
                id="dashed",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_dashed_opaque.png",
                "temperature_contour_dashed",
                1150,
                1336,
                "EPSG:32661",
                "-3384546.659457,1075289.936021,-713528.390451,3374842.088145",
                "TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF",
                id="dashed_opaque",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_dashed_zoomed_mercator.png",
                "temperature_contour_dashed",
                686,
                1057,
                "EPSG:3857",
                "849140.1753254525,7851535.092767621,888288.2674508916,7911855.112267022",
                "TRANSPARENT=TRUE",
                # Reprojected to EPSG:3857 and zoomed in tight (a ~39x60km bbox) on a single
                # contour line's sharp turn - close-up, high-curvature zoom in a different CRS.
                id="dashed_zoomed_mercator",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_dashed_zoomed_mercator_opaque.png",
                "temperature_contour_dashed",
                686,
                1057,
                "EPSG:3857",
                "849140.1753254525,7851535.092767621,888288.2674508916,7911855.112267022",
                "TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF",
                id="dashed_zoomed_mercator_opaque",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_shaded.png",
                "temperature_shaded",
                575,
                668,
                "EPSG:32661",
                "-3384546.659457,1075289.936021,-713528.390451,3374842.088145",
                "TRANSPARENT=FALSE&SHOWLEGEND=TRUE",
                # ShadeInterval-based classed style instead of contour lines - 25 fixed 4-degree
                # classes from -52 to 48 with an explicit fillcolor per class, exercising
                # ShadeInterval legend generation rather than the contour tracer.
                id="shaded",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_shaded_contour.png",
                "temperature_shaded_contour",
                575,
                668,
                "EPSG:32661",
                "-3384546.659457,1075289.936021,-713528.390451,3374842.088145",
                "TRANSPARENT=FALSE&SHOWLEGEND=TRUE",
                # Same classed style as temperature_shaded, but with a SmoothingFilter of 4 and
                # blue contour lines every 2 degrees overlaid - checks ShadeInterval classes and
                # the contour tracer combine correctly on a smoothed field.
                id="shaded_contour",
            ),
        ],
    )
    def test_contour_air_temperature_hagl(self, filetocheck: str, styles: str, width: int, height: int, crs: str, bbox: str, tail: str):
        """
        Contour lines drawn over a real, high resolution UWCW HA43 (DINI) air temperature field,
        to check that contour tracing (CImageOperators/drawContour.cpp) does not leave gaps on
        realistic data, across several styles, resolutions and projections. See the per-case
        comment above each pytest.param for what that specific case additionally exercises.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            filetocheck,
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            f"&LAYERS=air_temperature_hagl_contourtest&STYLES={styles}"
            f"&WIDTH={width}&HEIGHT={height}&CRS={crs}"
            f"&BBOX={bbox}"
            f"&FORMAT=image/png&{tail}",
        )
