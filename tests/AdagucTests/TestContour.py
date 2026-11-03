import os
import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import (
    make_adaguc_env,
    run_adaguc_and_compare_image,
    update_db,
)

ADAGUC_PATH = os.environ["ADAGUC_PATH"]

TEMPERATURE = "air_temperature_hagl_contourtest"
PRESSURE = "air_pressure_msl_contourtest"
PRECIPITATION = "total_precipitation_rate_contourtest"
# Pale land fill and light gray country outlines underneath the isobars, like a synoptic weather chart
WITH_COUNTRIES = f"country_land_contourtest,country_outlines_contourtest,{PRESSURE}"

POLAR_BBOX = "-3943161.614,-527345.786,8971.971,3938619.014"  # EPSG:32661
POLAR_5041_BBOX = "-527345.786,-3943161.614,3938619.014,8971.971"  # EPSG:5041, same area
MERCATOR_BBOX = "-2878501.827,5504614.352290839,4306070.437,9957952.27470916"  # EPSG:3857, west edge cropped close
ZOOMED_MERCATOR_BBOX = "84666,9813383,124666,9873383"  # EPSG:3857, ~40x60km on a sharp turn of one contour line


class TestContour:
    testresultspath = "testresults/TestContour/"
    expectedoutputsspath = "expectedoutputs/TestContour/"
    AdagucTestTools().mkdir_p(testresultspath)

    @pytest.mark.parametrize(
        ("filetocheck", "layers", "styles", "width", "height", "crs", "bbox", "tail"),
        [
            pytest.param(
                "test_contour_air_temperature_hagl.png",
                TEMPERATURE,
                "temperature_contour",
                700,
                620,
                "EPSG:32661",
                POLAR_BBOX,
                "TRANSPARENT=FALSE",
                id="plain",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_2x.png",
                TEMPERATURE,
                "temperature_contour",
                1400,
                1240,
                "EPSG:32661",
                POLAR_BBOX,
                "TRANSPARENT=FALSE",
                # A finer destination grid changes where contour line tracing starts and how far
                # apart distance-field pixels land, so this is a useful second data point for the
                # same gap-tracing code at a different pixel density.
                id="2x",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_2x_shaded_extrasmooth.png",
                TEMPERATURE,
                "temperature_shaded_extrasmooth",
                1400,
                1240,
                "EPSG:32661",
                POLAR_BBOX,
                "TRANSPARENT=FALSE&SHOWLEGEND=TRUE",
                # The 25-class, -52 to 48 multicolor legend instead of plain contour lines, a
                # heavier SmoothingFilter of 6, and blue contour lines every 2 degrees overlaid.
                id="2x_shaded_extrasmooth",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_smoothed.png",
                TEMPERATURE,
                "temperature_contour_smooth",
                1400,
                1240,
                "EPSG:32661",
                POLAR_BBOX,
                "TRANSPARENT=FALSE",
                # The unsmoothed field has real grid-scale noise (5.5km convection-permitting
                # model output), which turns into thousands of tiny disconnected contour loops
                # rather than a bug - SmoothingFilter is the normal way to get readable isotherms.
                id="smoothed",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_dashed.png",
                TEMPERATURE,
                "temperature_contour_dashed",
                1400,
                1240,
                "EPSG:32661",
                POLAR_BBOX,
                "TRANSPARENT=TRUE",
                # Two ContourLine definitions with different intervals and dash patterns, drawgrid
                # disabled - the same pattern as the production "pressure_cwk" isobar style.
                id="dashed",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_dashed_opaque.png",
                TEMPERATURE,
                "temperature_contour_dashed",
                1400,
                1240,
                "EPSG:32661",
                POLAR_BBOX,
                "TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF",
                id="dashed_opaque",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_dashed_zoomed_mercator.png",
                TEMPERATURE,
                "temperature_contour_dashed",
                686,
                1057,
                "EPSG:3857",
                ZOOMED_MERCATOR_BBOX,
                "TRANSPARENT=TRUE",
                # Reprojected to EPSG:3857 and zoomed in tight on a single contour line's sharp
                # turn - close-up, high-curvature zoom in a different CRS.
                id="dashed_zoomed_mercator",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_dashed_zoomed_mercator_opaque.png",
                TEMPERATURE,
                "temperature_contour_dashed",
                686,
                1057,
                "EPSG:3857",
                ZOOMED_MERCATOR_BBOX,
                "TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF",
                id="dashed_zoomed_mercator_opaque",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_shaded.png",
                TEMPERATURE,
                "temperature_shaded",
                700,
                620,
                "EPSG:32661",
                POLAR_BBOX,
                "TRANSPARENT=FALSE&SHOWLEGEND=TRUE",
                # ShadeInterval-based classed style instead of contour lines - 25 fixed 4-degree
                # classes from -52 to 48 with an explicit fillcolor per class, exercising
                # ShadeInterval legend generation rather than the contour tracer.
                id="shaded",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_shaded_contour.png",
                TEMPERATURE,
                "temperature_shaded_contour",
                700,
                620,
                "EPSG:32661",
                POLAR_BBOX,
                "TRANSPARENT=FALSE&SHOWLEGEND=TRUE",
                # Same classed style as temperature_shaded, but with a SmoothingFilter of 4 and
                # blue contour lines every 2 degrees overlaid - checks ShadeInterval classes and
                # the contour tracer combine correctly on a smoothed field.
                id="shaded_contour",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_outlined.png",
                TEMPERATURE,
                "temperature_contour_outlined",
                1400,
                1240,
                "EPSG:32661",
                POLAR_BBOX,
                "TRANSPARENT=FALSE&BGCOLOR=0xADD8E6",
                # Solid black lines with white-outlined labels, legible over a busy background.
                id="outlined",
            ),
            pytest.param(
                "test_contour_air_pressure_msl_outlined.png",
                PRESSURE,
                "pressure_msl_contour_outlined",
                1000,
                1000,
                "EPSG:5041",
                POLAR_5041_BBOX,
                "TRANSPARENT=FALSE&BGCOLOR=0xADD8E6",
                # Isobars every 4 hPa as on a synoptic chart: sparser labels (textdistance), soft label
                # halos (textsofthalo) and a bold 1000 hPa line (boldvalues/boldwidth).
                id="pressure_outlined",
            ),
            pytest.param(
                "test_contour_air_pressure_msl_outlined_with_country_outlines.png",
                WITH_COUNTRIES,
                ",,pressure_msl_contour_outlined",
                1000,
                1000,
                "EPSG:5041",
                POLAR_5041_BBOX,
                "TRANSPARENT=FALSE&BGCOLOR=0xADD8E6",
                # Known: a thin seam in the land fill at the central meridian of this projection,
                # a limitation of the GeoJSON rasterization (same with the production "baselayer").
                id="pressure_outlined_with_countries",
            ),
            pytest.param(
                "test_contour_air_pressure_msl_outlined_with_country_outlines_mercator.png",
                WITH_COUNTRIES,
                ",,pressure_msl_contour_outlined",
                1310,
                812,
                "EPSG:3857",
                MERCATOR_BBOX,
                "TRANSPARENT=FALSE&BGCOLOR=0xD5D9DD",
                # Mercator avoids that seam. Used as the example image in doc/configuration/ContourLine.md.
                id="pressure_outlined_with_countries_mercator",
            ),
            pytest.param(
                "test_contour_air_pressure_msl_outlined_with_country_outlines_mercator_extrasmooth.png",
                WITH_COUNTRIES,
                ",,pressure_msl_contour_outlined_extrasmooth",
                1310,
                812,
                "EPSG:3857",
                MERCATOR_BBOX,
                "TRANSPARENT=FALSE&BGCOLOR=0xD5D9DD",
                # SmoothingFilter 10 instead of 4.
                id="pressure_outlined_with_countries_mercator_extrasmooth",
            ),
            pytest.param(
                "test_contour_air_pressure_msl_blue_with_country_outlines_mercator.png",
                WITH_COUNTRIES,
                ",,pressure_msl_contour_blue",
                1310,
                812,
                "EPSG:3857",
                MERCATOR_BBOX,
                "TRANSPARENT=FALSE&BGCOLOR=0xD5D9DD",
                # Plain blue isobars and labels without halo, a marine forecast style.
                id="pressure_blue_with_countries_mercator",
            ),
            pytest.param(
                "test_contour_air_pressure_msl_outlined_with_green_land_mercator.png",
                WITH_COUNTRIES,
                "country_land_green,,pressure_msl_contour_outlined",
                1310,
                812,
                "EPSG:3857",
                MERCATOR_BBOX,
                "TRANSPARENT=FALSE&BGCOLOR=0xD5D9DD",
                # Classic map green land fill (#CCFFCC) instead of the pale one.
                id="pressure_outlined_with_green_land_mercator",
            ),
            pytest.param(
                "test_contour_air_pressure_msl_outlined_extrasmooth_with_green_land_mercator.png",
                WITH_COUNTRIES,
                "country_land_green,,pressure_msl_contour_outlined_extrasmooth",
                1310,
                812,
                "EPSG:3857",
                MERCATOR_BBOX,
                "TRANSPARENT=FALSE&BGCOLOR=0xD5D9DD",
                id="pressure_outlined_extrasmooth_with_green_land_mercator",
            ),
            pytest.param(
                "test_contour_air_temperature_hagl_wow.png",
                TEMPERATURE,
                "temperature_wow",
                700,
                620,
                "EPSG:32661",
                POLAR_BBOX,
                "TRANSPARENT=FALSE&SHOWLEGEND=TRUE",
                # Filled contours with a copy of the WOW temperature legend (UK Met Office citizen weather stations).
                id="temperature_wow",
            ),
            pytest.param(
                "test_contour_total_precipitation_rate_wow.png",
                PRECIPITATION,
                "precipitation_wow",
                700,
                620,
                "EPSG:32661",
                POLAR_BBOX,
                "TRANSPARENT=FALSE&SHOWLEGEND=TRUE",
                # Logarithmic ShadeInterval classes (0.1-0.3-1-3-10-30-100 mm/h) with contour lines on the same boundaries.
                id="precipitation_wow",
            ),
            pytest.param(
                "test_contour_combined_temperature_precipitation_pressure.png",
                f"{TEMPERATURE},{PRECIPITATION},{PRESSURE}",
                "temperature_wow,precipitation_wow,pressure_msl_contour_blue",
                1000,
                1000,
                "EPSG:5041",
                POLAR_5041_BBOX,
                "TRANSPARENT=FALSE&BGCOLOR=0xADD8E6",
                # All three fields in one request: filled temperature, precipitation classes on top, isobars last.
                id="combined",
            ),
        ],
    )
    def test_contour(self, filetocheck: str, layers: str, styles: str, width: int, height: int, crs: str, bbox: str, tail: str):
        """
        Contour lines and filled contours drawn over real, high resolution UWCW HA43 (DINI) fields
        (air temperature, mean sea level pressure and precipitation rate), to check that contour
        tracing (CImageOperators/drawContour.cpp) does not leave gaps on realistic data, and that
        the synoptic-chart ContourLine options render as intended, across several styles,
        resolutions and projections. See the per-case comment above each pytest.param.
        """
        env = make_adaguc_env("adaguc.tests.contourtest.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            filetocheck,
            "DATASET=adaguc.tests.contourtest&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            f"&LAYERS={layers}&STYLES={styles}"
            f"&WIDTH={width}&HEIGHT={height}&CRS={crs}"
            f"&BBOX={bbox}"
            f"&FORMAT=image/png&{tail}",
        )
