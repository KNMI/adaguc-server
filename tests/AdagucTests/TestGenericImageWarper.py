import io

import numpy as np
import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from PIL import Image
from conftest import (
    make_adaguc_env,
    run_adaguc_and_compare_image,
    update_db,
)


class TestGenericImageWarper:
    testresultspath = "testresults/TestGenericImageWarper/"
    expectedoutputsspath = "expectedoutputs/TestGenericImageWarper/"
    AdagucTestTools().mkdir_p(testresultspath)

    @pytest.mark.parametrize(
        ("filetocheck", "layers", "styles"),
        [
            (
                "ahn_utrechtseheuvelrug_500m_style_colormapped_nearest_elevation.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_colormapped_nearest_elevation",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_elevation.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_colormapped_bilinear_elevation",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_shaded_nearest_elevation.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_shaded_nearest_elevation",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_shaded_bilinear_elevation.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_shaded_bilinear_elevation",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_multicolor.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_colormapped_bilinear_multicolor",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_multicolor_contours.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_colormapped_bilinear_multicolor_contours",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_multicolor_contours_extrasmooth.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_colormapped_bilinear_multicolor_contours_extrasmooth",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_colormapped_bilinear_multicolor_contours_extrasmooth_discretized.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_colormapped_bilinear_multicolor_contours_extrasmooth_discretized",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_hillshaded_opaque.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_hillshaded_opaque/hillshaded",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_style_hillshaded_transparent.png",
                "ahn_utrechtseheuvelrug_500m",
                "style_hillshaded_transparent/hillshaded",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_hillshaded_style_colormapped_bilinear_elevation.png",
                "ahn_utrechtseheuvelrug_500m_hillshaded",
                "style_colormapped_bilinear_elevation",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_hillshaded_style_colormapped_bilinear_multicolor_contours.png",
                "ahn_utrechtseheuvelrug_500m_hillshaded",
                "style_colormapped_bilinear_multicolor_contours",
            ),
            (
                "ahn_utrechtseheuvelrug_500m_hillshaded_style_dutch_mountains.png",
                "ahn_utrechtseheuvelrug_500m_hillshaded",
                "style_dutch_mountains",
            ),
        ],
    )
    def test_GenericImageWarperOnAHNDataset(self, filetocheck: str, layers: str, styles: str):
        env = make_adaguc_env("adaguc.tests.ahn_utrechtse_heuvelrug_500m", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            filetocheck,
            f"LAYERS={layers}&STYLES={styles}&DATASET=adaguc.tests.ahn_utrechtse_heuvelrug_500m&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&WIDTH=512&HEIGHT=512&CRS=EPSG%3A3857&BBOX=590359.2826213799,6784102.0088095935,707368.84890462,6894816.225400406&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xFFFFFF&SHOWLEGEND=true",
            tolerance="antialiasing",
        )

    @pytest.mark.parametrize(
        ("filetocheck", "styles"),
        [
            ("ahn_utrechtseheuvelrug_500m_large_bilinear.png", "style_colormapped_bilinear_elevation"),
            ("ahn_utrechtseheuvelrug_500m_large_nearest.png", "style_colormapped_nearest_elevation"),
            ("ahn_utrechtseheuvelrug_500m_large_bilinear_contours.png", "style_colormapped_bilinear_multicolor_contours"),
        ],
    )
    def test_GenericImageWarperOnAHNDataset_large(self, filetocheck: str, styles: str):
        """Regression test for rendering images larger than one megapixel (bilinear, nearest, and contour-line styles), checked against a known-good baseline.
        The raster styles must match exactly; contour lines are antialiased."""
        env = make_adaguc_env("adaguc.tests.ahn_utrechtse_heuvelrug_500m", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            filetocheck,
            f"LAYERS=ahn_utrechtseheuvelrug_500m&STYLES={styles}&DATASET=adaguc.tests.ahn_utrechtse_heuvelrug_500m&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&WIDTH=1200&HEIGHT=1000&CRS=EPSG%3A3857&BBOX=590359.2826213799,6784102.0088095935,707368.84890462,6894816.225400406&FORMAT=image/png&TRANSPARENT=TRUE",
            tolerance="geometry" if "contours" in filetocheck else "exact",
        )

    def test_GenericImageWarperOnAHNDataset_contour_tight_curve_no_gap(self):
        """A contour line must not have a gap at the apex of a tight curve"""
        env = make_adaguc_env("adaguc.tests.ahn_utrechtse_heuvelrug_500m", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "ahn_utrechtseheuvelrug_500m_hillshaded_contour_tight_curve_no_gap.png",
            "DATASET=adaguc.tests.ahn_utrechtse_heuvelrug_500m&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            "&LAYERS=ahn_utrechtseheuvelrug_500m_hillshaded&WIDTH=686&HEIGHT=1057&CRS=EPSG%3A3857"
            "&BBOX=599744.071003962,6783137.928485779,696607.8022179123,6932387.146988906"
            "&STYLES=style_colormapped_bilinear_multicolor_contours&FORMAT=image/png&TRANSPARENT=TRUE",
        )

    def test_getmap_latlon_latdecreasing_and_increasing_genericbilinear(self):
        """
        Test rendering of data aligned upwards and downwards with bilinear interpolation
        """
        env = make_adaguc_env("adaguc.tests.genericrender.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_getmap_latlon_latdecreasing_and_increasing_genericbilinear.png",
            "DATASET=adaguc.tests.genericrender&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=latlon_latdecreasing_and_increasing_genericbilinear&WIDTH=256&HEIGHT=512&CRS=EPSG%3A3857&BBOX=471070.27730388555,6649427.860630986,982084.0646480684,7697681.648355679&STYLES=autobilinear&FORMAT=image/png&TRANSPARENT=TRUE&&time=2017-10-15T07%3A31%3A53Z&0.9458434770354436",
        )

    def test_getmap_latlon_latdecreasing_and_increasing_genericnearest(self):
        """
        Test rendering of data aligned upwards and downwards with nearest interpolation
        """
        env = make_adaguc_env("adaguc.tests.genericrender.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        run_adaguc_and_compare_image(
            env,
            "test_getmap_latlon_latdecreasing_and_increasing_genericnearest.png",
            "DATASET=adaguc.tests.genericrender&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=latlon_latdecreasing_and_increasing_genericnearest&WIDTH=256&HEIGHT=512&CRS=EPSG%3A3857&BBOX=471070.27730388555,6649427.860630986,982084.0646480684,7697681.648355679&STYLES=autogeneric&FORMAT=image/png&TRANSPARENT=TRUE&&time=2017-10-15T07%3A31%3A53Z&0.9458434770354436",
        )

    @pytest.mark.parametrize(
        ("layer", "bbox"),
        [
            # Ascending latitudes, data between 51 and 54 degrees north. The south edge of the first window is 0.8 of a cell away
            # from a cell border: the last rows of the image used to stay empty.
            pytest.param("latlon_latincreasing_linear", "52.2,5.3,53.3,7.8", id="latincreasing_south_edge_0.8_cell"),
            pytest.param("latlon_latincreasing_linear", "52.7,5.3,53.3,7.8", id="latincreasing_south_edge_0.3_cell"),
            # Descending latitudes, data between 54 and 57 degrees north, windows with the same alignment
            pytest.param("latlon_latdecreasing_linear", "55.2,5.3,56.3,7.8", id="latdecreasing_south_edge_0.8_cell"),
            pytest.param("latlon_latdecreasing_linear", "55.7,5.3,56.3,7.8", id="latdecreasing_south_edge_0.3_cell"),
        ],
    )
    def test_getmap_bilinear_same_crs_has_no_gaps_inside_the_data(self, layer, bbox):
        """
        With the CRS of the data (no reprojection, so the linear transform is used) and bilinear interpolation, every pixel
        of a window between the centres of the outermost grid cells has four source neighbours, so it must be drawn.
        This must hold for data with ascending and with descending latitudes, and for any alignment of the window with the grid.
        """
        env = make_adaguc_env("adaguc.tests.linearpixelspan.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.linearpixelspan&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap"
            f"&LAYERS={layer}&WIDTH=100&HEIGHT=44&CRS=EPSG%3A4326&BBOX={bbox}"
            "&STYLES=autobilinear&FORMAT=image/png&TRANSPARENT=TRUE&time=2017-10-15T07%3A31%3A53Z",
            env=env,
        )
        assert status == 0
        assert data[:4] == b"\x89PNG"
        alpha = np.array(Image.open(io.BytesIO(data)).convert("RGBA").getchannel("A"))
        empty_rows = np.flatnonzero((alpha == 0).all(axis=1)).tolist()
        empty_columns = np.flatnonzero((alpha == 0).all(axis=0)).tolist()
        assert (alpha == 255).all(), f"{(alpha == 0).sum()} transparent pixels, empty rows {empty_rows}, empty columns {empty_columns}"
