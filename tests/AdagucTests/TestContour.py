import os
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

    def test_contour_air_temperature_hagl(self):
        """
        Contour lines (every 2 degrees) drawn over a real, high resolution (575x668)
        UWCW HA43 (DINI) air temperature field, to check that contour tracing
        (CImageOperators/drawContour.cpp) does not leave gaps on realistic data.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_temperature_hagl.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_temperature_hagl_contourtest&STYLES=temperature_contour"
            "&WIDTH=575&HEIGHT=668&CRS=EPSG:32661"
            "&BBOX=-3384546.659457,1075289.936021,-713528.390451,3374842.088145"
            "&FORMAT=image/png&TRANSPARENT=FALSE",
        )

    def test_contour_air_temperature_hagl_2x(self):
        """
        Same field and bbox as test_contour_air_temperature_hagl, rendered at twice the
        resolution (1150x1336). A finer destination grid changes where contour line
        tracing starts and how far apart distance-field pixels land, so this is a useful
        second data point for the same gap-tracing code at a different pixel density.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_temperature_hagl_2x.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_temperature_hagl_contourtest&STYLES=temperature_contour"
            "&WIDTH=1150&HEIGHT=1336&CRS=EPSG:32661"
            "&BBOX=-3384546.659457,1075289.936021,-713528.390451,3374842.088145"
            "&FORMAT=image/png&TRANSPARENT=FALSE",
        )

    def test_contour_air_temperature_hagl_2x_shaded_extrasmooth(self):
        """
        Same field, bbox and 2x resolution as test_contour_air_temperature_hagl_2x, but with
        the temperature_shaded classed palette (the 25-class, -52 to 48 multicolor legend)
        instead of plain contour lines, a heavier SmoothingFilter of 6, and blue contour lines
        every 2 degrees overlaid (temperature_shaded_extrasmooth).
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_temperature_hagl_2x_shaded_extrasmooth.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_temperature_hagl_contourtest&STYLES=temperature_shaded_extrasmooth"
            "&WIDTH=1150&HEIGHT=1336&CRS=EPSG:32661"
            "&BBOX=-3384546.659457,1075289.936021,-713528.390451,3374842.088145"
            "&FORMAT=image/png&TRANSPARENT=FALSE&SHOWLEGEND=TRUE",
        )

    def test_contour_air_temperature_hagl_smoothed(self):
        """
        Same field, bbox and 2x resolution as test_contour_air_temperature_hagl_2x, but with
        a SmoothingFilter applied. The unsmoothed field has real grid-scale noise (5.5km
        convection-permitting model output), which turns into thousands of tiny disconnected
        contour loops rather than a bug - SmoothingFilter is the normal way to get readable
        isotherms from this kind of data, exercised here on the same tracer.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_temperature_hagl_smoothed.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_temperature_hagl_contourtest&STYLES=temperature_contour_smooth"
            "&WIDTH=1150&HEIGHT=1336&CRS=EPSG:32661"
            "&BBOX=-3384546.659457,1075289.936021,-713528.390451,3374842.088145"
            "&FORMAT=image/png&TRANSPARENT=FALSE",
        )

    def test_contour_air_temperature_hagl_dashed(self):
        """
        Same field as the other contour tests, but with a dashed, no-fill contour style
        (two ContourLine definitions with different intervals and dash patterns, drawgrid
        disabled) - the same pattern as the production "pressure_cwk" style used for isobars,
        applied here to the temperature field so it's exercised with checked-in test data.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_temperature_hagl_dashed.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_temperature_hagl_contourtest&STYLES=temperature_contour_dashed"
            "&WIDTH=1150&HEIGHT=1336&CRS=EPSG:32661"
            "&BBOX=-3384546.659457,1075289.936021,-713528.390451,3374842.088145"
            "&FORMAT=image/png&TRANSPARENT=TRUE",
        )

    def test_contour_air_temperature_hagl_dashed_opaque(self):
        """
        Same as test_contour_air_temperature_hagl_dashed, but rendered opaque with a white
        background (TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF) instead of on a transparent canvas.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_temperature_hagl_dashed_opaque.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_temperature_hagl_contourtest&STYLES=temperature_contour_dashed"
            "&WIDTH=1150&HEIGHT=1336&CRS=EPSG:32661"
            "&BBOX=-3384546.659457,1075289.936021,-713528.390451,3374842.088145"
            "&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF",
        )

    def test_contour_air_temperature_hagl_dashed_zoomed_mercator(self):
        """
        Same field and dashed style as test_contour_air_temperature_hagl_dashed, but reprojected
        to EPSG:3857 and zoomed in tight (a ~39x60km bbox) on a single contour line's sharp turn -
        close-up, high-curvature zoom in a different CRS than the other contour tests.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_temperature_hagl_dashed_zoomed_mercator.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_temperature_hagl_contourtest&STYLES=temperature_contour_dashed"
            "&WIDTH=686&HEIGHT=1057&CRS=EPSG:3857"
            "&BBOX=849140.1753254525,7851535.092767621,888288.2674508916,7911855.112267022"
            "&FORMAT=image/png&TRANSPARENT=TRUE",
        )

    def test_contour_air_temperature_hagl_shaded(self):
        """
        Same field as the other contour tests, but rendered with a ShadeInterval-based
        classed style (temperature_shaded) instead of contour lines - 25 fixed 4-degree
        classes from -52 to 48 with an explicit fillcolor per class, exercising ShadeInterval
        legend generation rather than the contour tracer. SHOWLEGEND=TRUE renders the legend
        onto the image so the generated classes are also visually verified.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_temperature_hagl_shaded.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_temperature_hagl_contourtest&STYLES=temperature_shaded"
            "&WIDTH=575&HEIGHT=668&CRS=EPSG:32661"
            "&BBOX=-3384546.659457,1075289.936021,-713528.390451,3374842.088145"
            "&FORMAT=image/png&TRANSPARENT=FALSE&SHOWLEGEND=TRUE",
        )

    def test_contour_air_temperature_hagl_shaded_contour(self):
        """
        Same classed style as temperature_shaded, but with a SmoothingFilter of 4 and blue
        contour lines every 2 degrees overlaid (temperature_shaded_contour) - checks that
        ShadeInterval classes and the contour tracer combine correctly on a smoothed field.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_temperature_hagl_shaded_contour.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_temperature_hagl_contourtest&STYLES=temperature_shaded_contour"
            "&WIDTH=575&HEIGHT=668&CRS=EPSG:32661"
            "&BBOX=-3384546.659457,1075289.936021,-713528.390451,3374842.088145"
            "&FORMAT=image/png&TRANSPARENT=FALSE&SHOWLEGEND=TRUE",
        )

    def test_contour_air_temperature_hagl_dashed_zoomed_mercator_opaque(self):
        """
        Same as test_contour_air_temperature_hagl_dashed_zoomed_mercator, but rendered opaque
        with a white background (TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF) instead of transparent.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_temperature_hagl_dashed_zoomed_mercator_opaque.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_temperature_hagl_contourtest&STYLES=temperature_contour_dashed"
            "&WIDTH=686&HEIGHT=1057&CRS=EPSG:3857"
            "&BBOX=849140.1753254525,7851535.092767621,888288.2674508916,7911855.112267022"
            "&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF",
        )
