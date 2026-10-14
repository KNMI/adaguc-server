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
        Contour lines (every 2 degrees) drawn over a real, high resolution (338x282)
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
            "&WIDTH=700&HEIGHT=620&CRS=EPSG:32661"
            "&BBOX=-3943161.614,-527345.786,8971.971,3938619.014"
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
            "&WIDTH=1400&HEIGHT=1240&CRS=EPSG:32661"
            "&BBOX=-3943161.614,-527345.786,8971.971,3938619.014"
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
            "&WIDTH=1400&HEIGHT=1240&CRS=EPSG:32661"
            "&BBOX=-3943161.614,-527345.786,8971.971,3938619.014"
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
            "&WIDTH=1400&HEIGHT=1240&CRS=EPSG:32661"
            "&BBOX=-3943161.614,-527345.786,8971.971,3938619.014"
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
            "&WIDTH=1400&HEIGHT=1240&CRS=EPSG:32661"
            "&BBOX=-3943161.614,-527345.786,8971.971,3938619.014"
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
            "&WIDTH=1400&HEIGHT=1240&CRS=EPSG:32661"
            "&BBOX=-3943161.614,-527345.786,8971.971,3938619.014"
            "&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF",
        )

    def test_contour_air_temperature_hagl_outlined(self):
        """
        Same field as the other contour tests, smoothed and rendered with a solid, non-dashed
        black contour line style - the same pattern as the production "pressure_cwk" isobar
        style - plus a white textstrokecolor/textstrokewidth on the labels, so they stay legible
        when drawn over a busy/colored background (e.g. a shaded pressure field or map tiles),
        like the labels on a synoptic weather chart. Rendered opaque over a sea-colored
        background (TRANSPARENT=FALSE&BGCOLOR=0xADD8E6) so the white label outline is visible.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_temperature_hagl_outlined.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_temperature_hagl_contourtest&STYLES=temperature_contour_outlined"
            "&WIDTH=1400&HEIGHT=1240&CRS=EPSG:32661"
            "&BBOX=-3943161.614,-527345.786,8971.971,3938619.014"
            "&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xADD8E6",
        )

    def test_contour_air_pressure_msl_outlined(self):
        """
        Mean sea level pressure field (300x300, KNMI UWCW HA43), rendered with the same
        outlined/gap-free contour line style as test_contour_air_temperature_hagl_outlined, but
        at the 4 hPa isobar interval conventionally used on synoptic weather charts - this is
        meant to visually mimic a standard pressure chart (isobars, L/H pressure centers, fronts).
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_pressure_msl_outlined.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_pressure_msl_contourtest&STYLES=pressure_msl_contour_outlined"
            "&WIDTH=1000&HEIGHT=1000&CRS=EPSG:5041"
            "&BBOX=-527345.786,-3943161.614,3938619.014,8971.971"
            "&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xADD8E6",
        )

    def test_contour_air_pressure_msl_outlined_with_country_outlines(self):
        """
        Same as test_contour_air_pressure_msl_outlined, with a light land-fill layer
        (country_land_contourtest, pale off-white land / transparent sea) and a light gray
        country-outline layer (country_outlines_contourtest - same countries.geojson used by the
        shared "overlay" layer, but with a lighter/thinner style so it doesn't visually compete
        with the isobars) drawn underneath - meant to mimic a full synoptic weather chart
        composition. Note: there's a known thin seam artifact (a stray line of the BGCOLOR/sea
        showing through) at the central meridian of this polar stereographic projection - this is
        a pre-existing limitation of the GeoJSON-to-raster rasterization technique, reproduced
        identically with the production "baselayer" style, not something specific to this test.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_pressure_msl_outlined_with_country_outlines.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=country_land_contourtest,country_outlines_contourtest,air_pressure_msl_contourtest"
            "&STYLES=,,pressure_msl_contour_outlined"
            "&WIDTH=1000&HEIGHT=1000&CRS=EPSG:5041"
            "&BBOX=-527345.786,-3943161.614,3938619.014,8971.971"
            "&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xADD8E6",
        )

    def test_contour_air_pressure_msl_outlined_with_country_outlines_mercator(self):
        """
        Same composition as test_contour_air_pressure_msl_outlined_with_country_outlines
        (isobars + light land fill + country outlines), but requested in EPSG:3857 (web
        Mercator) instead of the data's native EPSG:5041 polar stereographic projection. The
        country land-fill layer (country_land_contourtest) renders a thin seam artifact in
        EPSG:5041, at the central meridian of that projection - a pre-existing limitation of the
        GeoJSON-to-raster rasterization technique (reproduced identically with the production
        "baselayer" style, not something specific to this dataset). Mercator doesn't cross that
        seam for this extent, so the land fill renders cleanly here. BGCOLOR is a muted,
        slightly blue-tinted gray (0xD5D9DD, sampled from the reference chart's own sea color)
        rather than sea-blue, and the 1000 hPa isobar is drawn bold (boldvalues/boldwidth on
        pressure_msl_contour_outlined). Lines/text are a bit bigger, labels denser (textdistance
        100) and their white halo more pronounced (textstrokewidth 2.2) than the plain
        temperature_contour_outlined style, and the BBOX's west edge is cropped close enough that
        the isobars run off the left edge rather than floating in open ocean - all to better
        mimic the reference chart's framing and style.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_pressure_msl_outlined_with_country_outlines_mercator.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=country_land_contourtest,country_outlines_contourtest,air_pressure_msl_contourtest"
            "&STYLES=,,pressure_msl_contour_outlined"
            "&WIDTH=1310&HEIGHT=812&CRS=EPSG:3857"
            "&BBOX=-2878501.827,5504614.352290839,4306070.437,9957952.27470916"
            "&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xD5D9DD",
        )

    def test_contour_air_pressure_msl_outlined_with_country_outlines_mercator_extrasmooth(self):
        """
        Same as test_contour_air_pressure_msl_outlined_with_country_outlines_mercator, but with
        pressure_msl_contour_outlined_extrasmooth (SmoothingFilter 10 instead of 4) - a heavier
        smoothing pass for an even cleaner synoptic-chart look.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_pressure_msl_outlined_with_country_outlines_mercator_extrasmooth.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=country_land_contourtest,country_outlines_contourtest,air_pressure_msl_contourtest"
            "&STYLES=,,pressure_msl_contour_outlined_extrasmooth"
            "&WIDTH=1310&HEIGHT=812&CRS=EPSG:3857"
            "&BBOX=-2878501.827,5504614.352290839,4306070.437,9957952.27470916"
            "&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xD5D9DD",
        )

    def test_contour_air_pressure_msl_blue_with_country_outlines_mercator(self):
        """
        Same composition and field as test_contour_air_pressure_msl_outlined_with_country_outlines_mercator,
        but with pressure_msl_contour_blue - an alternate marine/shipping-forecast style palette
        (plain blue isobars and labels, no text halo) instead of the gray/outlined-label style.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_pressure_msl_blue_with_country_outlines_mercator.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=country_land_contourtest,country_outlines_contourtest,air_pressure_msl_contourtest"
            "&STYLES=,,pressure_msl_contour_blue"
            "&WIDTH=1310&HEIGHT=812&CRS=EPSG:3857"
            "&BBOX=-2878501.827,5504614.352290839,4306070.437,9957952.27470916"
            "&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xD5D9DD",
        )

    def test_contour_air_pressure_msl_outlined_with_green_land_mercator(self):
        """
        Same composition and field as test_contour_air_pressure_msl_outlined_with_country_outlines_mercator,
        with country_land_green (the classic map green, #CCFFCC - the same color used by the
        production "baselayer" style) instead of country_land_light's pale off-white, for a more
        traditional-map look.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_pressure_msl_outlined_with_green_land_mercator.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=country_land_contourtest,country_outlines_contourtest,air_pressure_msl_contourtest"
            "&STYLES=country_land_green,,pressure_msl_contour_outlined"
            "&WIDTH=1310&HEIGHT=812&CRS=EPSG:3857"
            "&BBOX=-2878501.827,5504614.352290839,4306070.437,9957952.27470916"
            "&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xD5D9DD",
        )

    def test_contour_air_pressure_msl_outlined_extrasmooth_with_green_land_mercator(self):
        """
        Same composition as test_contour_air_pressure_msl_outlined_with_green_land_mercator, with
        pressure_msl_contour_outlined_extrasmooth (SmoothingFilter 10 instead of 4) instead of
        pressure_msl_contour_outlined - the heavier-smoothing style combined with the green land
        fill instead of the gray.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_pressure_msl_outlined_extrasmooth_with_green_land_mercator.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=country_land_contourtest,country_outlines_contourtest,air_pressure_msl_contourtest"
            "&STYLES=country_land_green,,pressure_msl_contour_outlined_extrasmooth"
            "&WIDTH=1310&HEIGHT=812&CRS=EPSG:3857"
            "&BBOX=-2878501.827,5504614.352290839,4306070.437,9957952.27470916"
            "&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xD5D9DD",
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
            "&BBOX=84666,9813383,124666,9873383"
            "&FORMAT=image/png&TRANSPARENT=TRUE",
        )

    def test_contour_air_temperature_hagl_wow(self):
        """
        Same field as the other contour tests, rendered with temperature_wow - a bilinear-
        smoothed, filled-contour style using a local copy of the "wow.temperature" legend
        (the UK Met Office WOW citizen-weather-station palette, from geoweb-legends.include.xml)
        instead of contour lines. It's first in the layer's Styles list, so it's also the
        default style for this layer when none is requested. SHOWLEGEND=TRUE renders the legend
        onto the image so the palette is also visually verified.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_air_temperature_hagl_wow.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_temperature_hagl_contourtest&STYLES=temperature_wow"
            "&WIDTH=700&HEIGHT=620&CRS=EPSG:32661"
            "&BBOX=-3943161.614,-527345.786,8971.971,3938619.014"
            "&FORMAT=image/png&TRANSPARENT=FALSE&SHOWLEGEND=TRUE",
        )

    def test_contour_total_precipitation_rate_wow(self):
        """
        Total precipitation rate field, on the same grid/projection as the other contourtest
        layers, rendered with precipitation_wow - a classed, bilinear-smoothed ShadeInterval
        style derived from the "wow.precipitation" legend (the UK Met Office WOW
        citizen-weather-station palette, from geoweb-legends.include.xml), with the standard
        logarithmic (half-decade) precipitation class boundaries: 0.1-0.3-1-3-10-30-100 mm/h,
        plus contour lines (ContourLine classes=) traced on those same boundaries. SHOWLEGEND=TRUE
        renders the legend onto the image so the palette is also visually verified.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_total_precipitation_rate_wow.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=total_precipitation_rate_contourtest&STYLES=precipitation_wow"
            "&WIDTH=700&HEIGHT=620&CRS=EPSG:32661"
            "&BBOX=-3943161.614,-527345.786,8971.971,3938619.014"
            "&FORMAT=image/png&TRANSPARENT=FALSE&SHOWLEGEND=TRUE",
        )

    def test_contour_combined_temperature_precipitation_pressure(self):
        """
        A synoptic-style composition combining all three contourtest fields in one GetMap request,
        in the data's native EPSG:5041 polar stereographic projection: air_temperature_hagl_contourtest
        (temperature_wow, filled contours) drawn first as the base layer, total_precipitation_rate_contourtest
        (precipitation_wow, classed ShadeIntervals with contour lines) drawn on top of that, and
        air_pressure_msl_contourtest (pressure_msl_contour_blue, transparent isobars) drawn last on
        top of everything - the same layer stacking order used in the ADAGUC Data Explorer.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_contour_combined_temperature_precipitation_pressure.png",
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=air_temperature_hagl_contourtest,total_precipitation_rate_contourtest,air_pressure_msl_contourtest"
            "&STYLES=temperature_wow,precipitation_wow,pressure_msl_contour_blue"
            "&WIDTH=1000&HEIGHT=1000&CRS=EPSG:5041"
            "&BBOX=-527345.786,-3943161.614,3938619.014,8971.971"
            "&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xADD8E6",
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
            "&WIDTH=700&HEIGHT=620&CRS=EPSG:32661"
            "&BBOX=-3943161.614,-527345.786,8971.971,3938619.014"
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
            "&WIDTH=700&HEIGHT=620&CRS=EPSG:32661"
            "&BBOX=-3943161.614,-527345.786,8971.971,3938619.014"
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
            "&BBOX=84666,9813383,124666,9873383"
            "&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF",
        )
