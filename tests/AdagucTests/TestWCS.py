# pylint: disable=line-too-long
# pylint: disable=unused-variable
# pylint: disable=invalid-name

"""
Run test for WCS Server system of adaguc-server
"""

import io
import os
from pathlib import Path
import re
import netCDF4
import numpy as np
from netCDF4 import Dataset
from PIL import Image, TiffImagePlugin
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import make_adaguc_env, update_db

ADAGUC_PATH = os.environ["ADAGUC_PATH"]

# TIFF tags and GeoTIFF keys and values used in the GeoTIFF tests
TIFFTAG_BITS_PER_SAMPLE = 258
TIFFTAG_SAMPLES_PER_PIXEL = 277
TIFFTAG_PLANAR_CONFIG = 284
PLANARCONFIG_SEPARATE = 2
TIFFTAG_MODELPIXELSCALE = 33550
TIFFTAG_MODELTIEPOINT = 33922
TIFFTAG_GEOKEYDIRECTORY = 34735
TIFFTAG_GEODOUBLEPARAMS = 34736
TIFFTAG_GEOASCIIPARAMS = 34737
TIFFTAG_GDAL_METADATA = 42112
TIFFTAG_GDAL_NODATA = 42113
GT_MODEL_TYPE = 1024
GT_RASTER_TYPE = 1025
GEOGRAPHIC_TYPE = 2048
GEOG_SEMI_MAJOR_AXIS = 2057
GEOG_TOWGS84 = 2062
PROJECTED_CS_TYPE = 3072
PROJ_COORD_TRANS = 3075
PROJ_LINEAR_UNITS = 3076
PROJ_NAT_ORIGIN_LONG = 3080
PROJ_NAT_ORIGIN_LAT = 3081
PROJ_FALSE_EASTING = 3082
PROJ_FALSE_NORTHING = 3083
PROJ_SCALE_AT_NAT_ORIGIN = 3092
MODEL_TYPE_PROJECTED = 1
MODEL_TYPE_GEOGRAPHIC = 2
RASTER_PIXEL_IS_AREA = 1
USER_DEFINED = 32767
CT_OBLIQUE_STEREOGRAPHIC = 16
LINEAR_METER = 9001


def read_geotiff(data: bytes):
    """
    Returns the image, the TIFF tags and the GeoTIFF keys of a GeoTIFF. Short keys are ints, double and ascii keys tuples and strings.
    """
    image = Image.open(io.BytesIO(data))
    tags = image.tag_v2
    directory = tags[TIFFTAG_GEOKEYDIRECTORY]
    doubles = tags.get(TIFFTAG_GEODOUBLEPARAMS, ())
    ascii_params = tags.get(TIFFTAG_GEOASCIIPARAMS, "")
    geokeys = {}
    for i in range(1, directory[3] + 1):
        key, location, count, offset = directory[4 * i : 4 * i + 4]
        if location == 0:
            geokeys[key] = offset
        elif location == TIFFTAG_GEODOUBLEPARAMS:
            geokeys[key] = tuple(doubles[offset : offset + count])
        else:
            geokeys[key] = ascii_params[offset : offset + count]
    return image, tags, geokeys


def read_source_values(filename: str, variable: str):
    with Dataset(f"{ADAGUC_PATH}/data/datasets/{filename}") as dataset:
        return np.array(dataset.variables[variable][:])


class TestWCS:
    """
    Run test for WCS Server system of adaguc-server
    """

    testresultspath = "testresults/TestWCS/"
    expectedoutputsspath = "expectedoutputs/TestWCS/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_WCSGetCapabilities_testdatanc(self):
        """
        Check if WCS GetCapabilities for testdata.nc file is OK
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_WCSGetCapabilities_testdatanc.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WCS&request=getcapabilities", env=self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename)

    def test_WCSDescribeCoverage_Actuele10mindata(self):
        """
        Check if WCS DescribeCoverage for Actuele10mindataKNMIstations_20201220123000.nc file is OK
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_WCSDescribeCoverage_testdatanc.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=test/netcdfpointtimeseries/Actuele10mindataKNMIstations_20201220123000.nc&SERVICE=WCS&request=describecoverage&coverage=ff,dd,ta",
            env=self.env,
            maxLogFileSize=16384,
        )  # Silence log flood warning, datafile has lots of variables, each giving log output
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename)

    def test_WCSDescribeCoverage_Multidimdata(self):
        """
        Check if WCS DescribeCoverage shows information for multidimensional data
        """
        AdagucTestTools().cleanTempDir()
        # This file has the following dimensions: lon,lat,member,height, and time.
        filename = "test_WCSDescribeCoverage_Multidimdata.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=netcdf_5dims/netcdf_5dims_seq2/nc_5D_20170101001500-20170101002500.nc&SERVICE=WCS&request=describecoverage&coverage=data",
            env=self.env,
            maxLogFileSize=16384,
        )  # Silence log flood warning, datafile has lots of variables, each giving log output
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename)

    def test_WCSDescribeCoverage_Multidimdata_large_daily(self):
        """
        Check if WCS DescribeCoverage shows information for multidimensional data, with more than 200 equally spaced timestamps
        """
        AdagucTestTools().cleanTempDir()
        # This file has the following dimensions: lon,lat,member,height, and time.
        filename = "test_WCSDescribeCoverage_Multidimdata_large_daily.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=large_number_timesteps/210_daily_timesteps.nc&SERVICE=WCS&request=describecoverage&coverage=data",
            env=self.env,
            maxLogFileSize=16384,
        )  # Silence log flood warning, datafile has lots of variables, each giving log output
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename)

    def test_WCSDescribeCoverage_Multidimdata_large_2day_height(self):
        """
        Check if WCS DescribeCoverage shows information for multidimensional data, with more than 200  equally spaced timestamps
        and one extra dimension (height)
        """
        AdagucTestTools().cleanTempDir()
        # This file has the following dimensions: lon,lat,member,height, and time.
        filename = "test_WCSDescribeCoverage_Multidimdata_large_2day_height.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=large_number_timesteps/210_2day_timesteps_5_heights.nc&SERVICE=WCS&request=describecoverage&coverage=data",
            env=self.env,
            maxLogFileSize=16384,
        )  # Silence log flood warning, datafile has lots of variables, each giving log output
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename)

    def test_WCSDescribeCoverage_Multidimdata_99_timesteps(self):
        """
        Check if WCS DescribeCoverage shows information for multidimensional data, with 190 equally spaced timestamps
        """
        AdagucTestTools().cleanTempDir()
        # This file has the following dimensions: lon,lat,member,height, and time.
        filename = "test_WCSDescribeCoverage_Multidimdata_99_timesteps.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=large_number_timesteps/99_2day_timesteps.nc&SERVICE=WCS&request=describecoverage&coverage=data",
            env=self.env,
            maxLogFileSize=16384,
        )  # Silence log flood warning, datafile has lots of variables, each giving log output
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename)

    def test_WCSDescribeCoverage_Multidimdata_large_random(self):
        """
        Check if WCS DescribeCoverage shows information for multidimensional data, with more than 200 randomly spaced timestamps
        """
        AdagucTestTools().cleanTempDir()
        # This file has the following dimensions: lon,lat,member,height, and time.
        filename = "test_WCSDescribeCoverage_Multidimdata_large_random.xml"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=large_number_timesteps%2F210_random_timesteps.nc&&service=WCS&REQUEST=DescribeCoverage&COVERAGE=data",
            env=self.env,
            maxLogFileSize=16384,
            showLog=True,
        )  # Silence log flood warning, datafile has lots of variables, each giving log output
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename)

    def test_WCSGetCoverageAAIGRID_testdatanc(self):
        """
        Check if WCS GetCoverage for testdata.nc as AAIGRID file is OK
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_WCSGetCoverageAAIGRID_testdatanc.grd"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=aaigrid&BBOX=-180,-90,180,90&RESX=1&RESY=1",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        # The expected output was written by an older GDAL version, which used other spaces than the current output.
        # Compare in a way where any sequence of whitespace characters is equivalent
        # This means that newlines (which are important?) are not compared
        test_output = data
        expected_output = AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)
        test_output = " ".join(test_output.decode("utf-8").split())
        expected_output = " ".join(expected_output.decode("utf-8").split())
        assert test_output == expected_output

    def test_WCSGetCoverageNetCDF3_testdatanc(self):
        """
        Check if WCS GetCoverage for testdata.nc as NetCDF3 file is OK
        """
        AdagucTestTools().cleanTempDir()
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=NetCDF3&BBOX=-180,-90,180,90&RESX=1&RESY=1",
            env=self.env,
            args=["--report"],
        )
        assert status == 0
        assert data[0:10] == b"CDF\x02\x00\x00\x00\x00\x00\x00"

    def test_WCSGetCoverageNetCDF3_int64_dimension(self):
        """
        NetCDF4 file with int64 datatype cannot be written as netcdf3 because int64 is not supported. Don't write file but show error
        """
        env = make_adaguc_env("adaguc.tests.multi_reftime_temporal_extent.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.multi_reftime_temporal_extent&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=weather_code_gl"
            "&CRS=EPSG:4326&FORMAT=NetCDF3&BBOX=0,50,10,55&RESX=0.5&RESY=0.5",
            env=env,
        )
        assert status == 0
        assert b"variable 'gl' is a 64-bit integer, which NetCDF3 does not support" in data
        assert b"nc_def_var" not in data

    def test_WCSGetCoverageNetCDF4_int64_dimension_preserves_datatype(self):
        """
        Companion to test_WCSGetCoverageNetCDF3_int64_dimension: Ensure that datatype is preserved.
        """
        env = make_adaguc_env("adaguc.tests.multi_reftime_temporal_extent.xml", self.testresultspath, self.expectedoutputsspath)
        update_db(env)
        for format_name in ("netcdf", "NetCDF4"):
            status, data, _ = AdagucTestTools().runADAGUCServer(
                "DATASET=adaguc.tests.multi_reftime_temporal_extent&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=weather_code_gl"
                f"&CRS=EPSG:4326&FORMAT={format_name}&BBOX=0,50,10,55&RESX=0.5&RESY=0.5",
                env=env,
            )
            assert status == 0
            assert data[0:8] == b"\x89HDF\r\n\x1a\n"

            ds = netCDF4.Dataset("filename.nc", memory=data)
            assert str(ds.variables["gl"].dtype) == "int64"
            assert ds.variables["gl"][0] == 0

    def test_WCSGetCoverageNetCDF4_testdatanc(self):
        """
        Check if WCS GetCoverage for testdata.nc as NetCDF4 file is OK
        """
        AdagucTestTools().cleanTempDir()
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=NetCDF4&BBOX=-180,-90,180,90&RESX=1&RESY=1",
            env=self.env,
            args=["--report"],
        )
        assert status == 0
        assert data[0:6] == b"\x89HDF\r\n"

    def test_WCSGetCoverageNetCDF4_packed_data_levels(self):
        """
        Packed data (scale_factor 0.1) over several levels:
        - Every level must be scaled. Only the first level was: the variable is reused for the next level and was marked as scaled.
        - Cells outside the source grid must have the _FillValue. They got the unscaled _FillValue (-9999), while the _FillValue
          attribute was scaled (-999.9), so they were not seen as nodata.
        """
        AdagucTestTools().cleanTempDir()
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata_packed_fillvalue.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=packed&CRS=EPSG%3A4326&FORMAT=NetCDF4&BBOX=-5,45,15,60&RESX=1&RESY=1&elevation=1/3",
            env=self.env,
        )
        assert status == 0
        values = netCDF4.Dataset("filename.nc", memory=data).variables["packed"][:]
        assert values.shape == (3, 15, 20)
        # netCDF4 applies the scale_factor and masks the _FillValue, so these are the values the output must have
        source = netCDF4.Dataset(ADAGUC_PATH + "/data/datasets/testdata_packed_fillvalue.nc").variables["packed"][:]
        for level in range(3):
            # The output grid is larger than the source grid: the cells outside it, and the nodata cell of the source, must be nodata.
            # What is left are exactly the source values of this level.
            assert values[level].count() == source[level].count()
            assert np.allclose(sorted(values[level].compressed()), sorted(source[level].compressed()))

    def test_WCSGetCoverageGeoTiff_testdatanc(self):
        """
        Check if WCS GetCoverage for testdata.nc as TIFF file is OK
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_WCSGetCoverageGeoTiff_testdatanc.tiff"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=geotiff&BBOX=-180,-90,180,90&RESX=1&RESY=1",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        # Little endian TIFF, with the directory of 18 tags directly after the header, like GDAL writes it
        assert data[0:10] == b"II*\x00\x08\x00\x00\x00\x12\x00"

        image, tags, geokeys = read_geotiff(data)
        assert image.size == (360, 180)
        assert image.mode == "F"
        assert tags[TIFFTAG_MODELPIXELSCALE] == (1.0, 1.0, 0.0)
        assert tags[TIFFTAG_MODELTIEPOINT] == (0.0, 0.0, 0.0, -180.0, 90.0, 0.0)
        assert geokeys[GT_MODEL_TYPE] == MODEL_TYPE_GEOGRAPHIC
        assert geokeys[GT_RASTER_TYPE] == RASTER_PIXEL_IS_AREA
        assert geokeys[GEOGRAPHIC_TYPE] == 4326
        assert tags[TIFFTAG_GDAL_NODATA] == "-0.0099999997764825821"
        assert '<Item name="NETCDF_VARNAME" sample="0">testdata</Item>' in tags[TIFFTAG_GDAL_METADATA]

        # The warper copies values of source cells (nearest neighbour), the rest of the globe is nodata
        values = np.array(image)
        source = read_source_values("testdata.nc", "testdata")
        nodata = np.float32(-0.01)
        assert np.count_nonzero(values == nodata) > 0
        valid = values[values != nodata]
        assert valid.size > 0
        assert set(valid.tolist()) <= set(source.ravel().tolist())

    def test_WCSGetCoverageGeoTiff_NATIVE_testdatanc(self):
        """
        Check that a GeoTIFF on the native grid (a proj string with +towgs84) has the source values and a user defined CRS
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_WCSGetCoverageGeoTiff_NATIVE_testdatanc.tiff"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&FORMAT=geotiff&", env=self.env, args=["--report"]
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0

        image, tags, geokeys = read_geotiff(data)
        assert image.size == (29, 31)
        assert np.allclose(tags[TIFFTAG_MODELPIXELSCALE], (103448.276786, 96774.191667, 0.0))
        assert np.allclose(tags[TIFFTAG_MODELTIEPOINT], (0.0, 0.0, 0.0, -1500000.013393, 1999999.970833, 0.0))
        assert geokeys[GT_MODEL_TYPE] == MODEL_TYPE_PROJECTED
        assert geokeys[PROJECTED_CS_TYPE] == USER_DEFINED
        assert geokeys[PROJ_COORD_TRANS] == CT_OBLIQUE_STEREOGRAPHIC
        assert np.allclose(geokeys[PROJ_NAT_ORIGIN_LAT], 52.15616055555555)
        assert np.allclose(geokeys[PROJ_NAT_ORIGIN_LONG], 5.38763888888889)
        assert geokeys[PROJ_SCALE_AT_NAT_ORIGIN] == (0.9999079,)
        assert geokeys[PROJ_FALSE_EASTING] == (155000.0,)
        assert geokeys[PROJ_FALSE_NORTHING] == (463000.0,)
        assert geokeys[GEOG_SEMI_MAJOR_AXIS] == (6377397.155,)
        assert np.allclose(geokeys[GEOG_TOWGS84], (565.417, 50.3319, 465.552, -0.398957, 0.343988, -1.8774, 4.0725))

        # The native grid has the source values as they are, the file has the y dimension first
        source = read_source_values("testdata.nc", "testdata")
        assert np.array_equal(np.array(image), source.T)

    def test_WCSGetCoverageGeoTiff_EPSG3857_testdatanc(self):
        """
        Check if WCS GetCoverage for testdata.nc as GeoTIFF in a projected CRS has the EPSG code and the requested grid
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_WCSGetCoverageGeoTiff_EPSG3857_testdatanc.tiff"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A3857&FORMAT=geotiff&BBOX=-1000000,5000000,2000000,8000000&WIDTH=60&HEIGHT=60",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0

        image, tags, geokeys = read_geotiff(data)
        assert image.size == (60, 60)
        assert tags[TIFFTAG_MODELPIXELSCALE] == (50000.0, 50000.0, 0.0)
        assert tags[TIFFTAG_MODELTIEPOINT] == (0.0, 0.0, 0.0, -1000000.0, 8000000.0, 0.0)
        assert geokeys[GT_MODEL_TYPE] == MODEL_TYPE_PROJECTED
        assert geokeys[PROJECTED_CS_TYPE] == 3857
        assert geokeys[PROJ_LINEAR_UNITS] == LINEAR_METER
        assert GEOGRAPHIC_TYPE not in geokeys

        values = np.array(image)
        source = read_source_values("testdata.nc", "testdata")
        valid = values[values != np.float32(-0.01)]
        assert valid.size > 0
        assert set(valid.tolist()) <= set(source.ravel().tolist())

    def test_WCSGetCoverageGeoTiff_multiple_bands(self):
        """
        Check that a GeoTIFF of three time steps has three bands with their time in the band metadata, and that AAIGrid refuses it
        """
        AdagucTestTools().cleanTempDir()
        query = "source=multi_dim.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&BBOX=0,50,10,60&WIDTH=20&HEIGHT=20&TIME=2024-06-01T00:00:00Z/2024-06-01T03:00:00Z"
        status, data, _ = AdagucTestTools().runADAGUCServer(query + "&FORMAT=geotiff", env=self.env, args=["--report"])
        AdagucTestTools().writetofile(self.testresultspath + "test_WCSGetCoverageGeoTiff_multiple_bands.tiff", data)
        assert status == 0

        # Pillow cannot decode three float64 samples per pixel, but it can read the tags
        tags = TiffImagePlugin.ImageFileDirectory_v2(data[0:8])
        stream = io.BytesIO(data)
        stream.seek(8)
        tags.load(stream)
        assert tags[TIFFTAG_SAMPLES_PER_PIXEL] == 3
        assert tags[TIFFTAG_BITS_PER_SAMPLE] == (64, 64, 64)
        assert tags[TIFFTAG_PLANAR_CONFIG] == PLANARCONFIG_SEPARATE
        assert tags[TIFFTAG_GDAL_NODATA] == "nan"
        for sample, time in enumerate(["1.000000", "2.000000", "3.000000"]):
            assert f'<Item name="NETCDF_DIM_time" sample="{sample}">{time}</Item>' in tags[TIFFTAG_GDAL_METADATA]
        assert '<Item name="NETCDF_DIM_time_VALUES">{1.000000,2.000000,3.000000}</Item>' in tags[TIFFTAG_GDAL_METADATA]

        status, data, _ = AdagucTestTools().runADAGUCServer(query + "&FORMAT=aaigrid", env=self.env)
        assert b"This WCS format ('aaigrid') does not support multiple bands" in data

    def test_WCSDescribeCoverage_lists_only_supported_formats(self):
        """A WCSFormat with a driver that GetCoverage cannot write is not offered in DescribeCoverage"""
        env = make_adaguc_env("adaguc.tests.wcsdrivers")
        update_db(env)
        query = "dataset=adaguc.tests.wcsdrivers&SERVICE=WCS&REQUEST=DescribeCoverage&COVERAGE=testdata"
        status, data, _ = AdagucTestTools().runADAGUCServer(query, env=env)
        assert status == 0
        assert b"<formats>gdalnetcdf</formats>" in data
        assert b"<formats>geotiffwithoptions</formats>" in data
        assert b"<formats>png</formats>" not in data

    def test_WCSGetCoverage_unsupported_driver(self):
        """
        Check that a WCSFormat with a driver other than ADAGUCNetCDF, GTiff or AAIGrid gives a clear error,
        that the GDAL NetCDF driver name maps to the ADAGUC NetCDF writer, and that unsupported options give a warning.
        """
        env = make_adaguc_env("adaguc.tests.wcsdrivers")
        update_db(env)
        query = "dataset=adaguc.tests.wcsdrivers&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&BBOX=0,48,10,58&WIDTH=40&HEIGHT=50"

        # Like other failing GetCoverage requests, the error is reported in the response body
        status, data, _ = AdagucTestTools().runADAGUCServer(query + "&FORMAT=png", env=env)
        assert status == 0
        assert b'WCSFormat "png": driver "PNG" is not supported. Supported drivers are ADAGUCNetCDF, GTiff and AAIGrid' in data

        status, data, headers = AdagucTestTools().runADAGUCServer(query + "&FORMAT=gdalnetcdf", env=env)
        assert status == 0
        assert "Content-Type:application/netcdf" in headers
        assert data[0:4] == b"\x89HDF"
        assert (
            'options="WRITE_GDAL_TAGS=FALSE,FORMAT=NC4C" are not supported for driver NetCDF and are ignored'
            in AdagucTestTools().getLogFile()
        )

        status, data, headers = AdagucTestTools().runADAGUCServer(query + "&FORMAT=geotiffwithoptions", env=env)
        assert status == 0
        assert "Content-Type:image/tiff" in headers
        assert data[0:4] == b"II*\x00"
        assert 'options="COMPRESS=DEFLATE" are not supported for driver GTiff and are ignored' in AdagucTestTools().getLogFile()

    # TODO: This test produces incorrect results. Made a ticket in backlog.
    def test_WCSGetCoverageAAIGRID_NATIVE_testdatanc(self):
        """
        Check if WCS GetCoverage for testdata.nc as Native grid is OK
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_WCSGetCoverageAAIGRID_NATIVE_testdatanc.grd"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&FORMAT=aaigrid&", env=self.env, args=["--report"]
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        lines = str(data.decode("UTF-8")).splitlines()[0:6]
        parsed = [re.split(r"\s+", line) for line in lines]
        aaigrid_header_items = [(p[0], round(float(p[1]), 3)) for p in parsed]

        assert [
            ("ncols", 29.0),
            ("nrows", 31.0),
            ("xllcorner", -1500000.013),
            ("yllcorner", -999999.971),
            ("dx", 103448.277),
            ("dy", 96774.192),
        ] == aaigrid_header_items

    def test_WCSGetCoverageNetCDF4_testdatanc_adaguc_wcs_destgridspec_specifiedgridresxresy(self):
        """
        Check if WCS GetCoverage for specified settings returns correct grid spec
        """
        AdagucTestTools().cleanTempDir()
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=NetCDF4&BBOX=-180,-90,180,90&RESX=1&RESY=1",
            env=self.env,
            args=["--report"],
        )
        assert status == 0
        assert data[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data)

        expectedgridspec = "width=360&height=180&resx=1.000000&resy=1.000000&bbox=-180.000000,-90.000000,180.000000,90.000000&crs=EPSG:4326"
        foundgridspec = ds.getncattr("adaguc_wcs_destgridspec")
        assert foundgridspec == expectedgridspec
        projectionid = ds.variables["crs"].getncattr("id")
        assert "EPSG:4326" == projectionid

    def test_WCSGetCoverageNetCDF4_testdatanc_adaguc_wcs_destgridspec_specifiedgridwidthheight(self):
        """
        Check if WCS GetCoverage for specified settings returns correct grid spec
        """
        AdagucTestTools().cleanTempDir()
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=NetCDF4&BBOX=-180,-90,180,90&width=360&height=180",
            env=self.env,
            args=["--report"],
        )
        assert status == 0
        assert data[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data)

        expectedgridspec = "width=360&height=180&resx=1.000000&resy=1.000000&bbox=-180.000000,-90.000000,180.000000,90.000000&crs=EPSG:4326"
        foundgridspec = ds.getncattr("adaguc_wcs_destgridspec")
        assert foundgridspec == expectedgridspec
        projectionid = ds.variables["crs"].getncattr("id")
        assert "EPSG:4326" == projectionid

    def test_WCSGetCoverageNetCDF4_testdatanc_adaguc_wcs_destgridspec_native(self):
        """
        Check if WCS GetCoverage for specified settings returns correct grid spec
        """
        AdagucTestTools().cleanTempDir()
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&FORMAT=NetCDF4", env=self.env, args=["--report"]
        )
        assert status == 0
        assert data[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data)

        expectedgridspec = "width=29&height=31&resx=103448.276786&resy=96774.191667&bbox=-1500000.013393,-999999.970833,1500000.013393,1999999.970833&crs=+proj=sterea +lat_0=52.15616055555555 +lon_0=5.38763888888889 +k=0.9999079 +x_0=155000 +y_0=463000 +ellps=bessel +towgs84=565.417,50.3319,465.552,-0.398957,0.343988,-1.8774,4.0725 +units=m +no_defs"
        foundgridspec = ds.getncattr("adaguc_wcs_destgridspec")
        assert foundgridspec == expectedgridspec
        projectionid = ds.variables["crs"].getncattr("id")
        assert "unknown" == projectionid

    def test_WCSGetCoverageNetCDF4_testdatanc_adaguc_wcs_destgridspec_specifiedgrid_noresxnoresy_fullglobe(self):
        """
        Check if WCS GetCoverage for specified settings returns correct grid spec
        """
        AdagucTestTools().cleanTempDir()
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=NetCDF4&BBOX=-180,-90,180,90",
            env=self.env,
            args=["--report"],
        )
        assert status == 0
        assert data[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data)

        expectedgridspec = "width=174&height=196&resx=2.068966&resy=0.918367&bbox=-180.000000,-90.000000,180.000000,90.000000&crs=EPSG:4326"
        foundgridspec = ds.getncattr("adaguc_wcs_destgridspec")
        assert foundgridspec == expectedgridspec
        projectionid = ds.variables["crs"].getncattr("id")
        assert "EPSG:4326" == projectionid

    def test_WCSGetCoverageNetCDF4_testdatanc_adaguc_wcs_destgridspec_specifiedgrid_noresxnoresy_nl(self):
        """
        Check if WCS GetCoverage for specified settings returns correct grid spec
        """
        AdagucTestTools().cleanTempDir()
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=NetCDF4&BBOX=-10,40,20,60",
            env=self.env,
            args=["--report"],
        )
        assert status == 0
        assert data[0:6] == b"\x89HDF\r\n"
        ds = netCDF4.Dataset("filename.nc", memory=data)

        expectedgridspec = "width=15&height=22&resx=2.000000&resy=0.909091&bbox=-10.000000,40.000000,20.000000,60.000000&crs=EPSG:4326"
        foundgridspec = ds.getncattr("adaguc_wcs_destgridspec")
        assert foundgridspec == expectedgridspec
        projectionid = ds.variables["crs"].getncattr("id")
        assert "EPSG:4326" == projectionid

    def test_WCSGetCoverageNetCDF4_testdatanc_adaguc_wcs_destgridspec_specifiedgridwidthheight_responsecrs(self):
        """
        Check if WCS GetCoverage for specified settings returns correct grid spec
        """
        AdagucTestTools().cleanTempDir()
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=NetCDF4&BBOX=0,50,10,60&width=100&height=100&RESPONSE_CRS=EPSG:28992&",
            env=self.env,
            args=["--report"],
        )
        assert status == 0
        assert data[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data)

        expectedgridspec = "width=100&height=100&resx=7167.687040&resy=11262.730534&bbox=-231108.757898,223282.967525,485659.946091,1349556.020954&crs=EPSG:28992"
        foundgridspec = ds.getncattr("adaguc_wcs_destgridspec")
        assert foundgridspec == expectedgridspec
        projectionid = ds.variables["crs"].getncattr("id")
        assert "EPSG:28992" == projectionid

    def test_WCS_WithCaching(self, tmp_path: Path):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.tests.cacheheader.xml"

        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        assert status == 0

        status, data, headers = AdagucTestTools().runADAGUCServer("SERVICE=WCS&request=GetCapabilities", {"ADAGUC_CONFIG": config})
        assert status == 0
        assert headers, ["Content-Type:text/xml" == "Cache-Control:max-age=60"]

        status, data, headers = AdagucTestTools().runADAGUCServer("SERVICE=WCS&request=DescribeCoverage", {"ADAGUC_CONFIG": config})
        assert status == 0
        assert headers, ["Content-Type:text/xml" == "Cache-Control:max-age=60"]

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WCS&request=DescribeCoverage&coverage=data", {"ADAGUC_CONFIG": config}
        )
        assert status == 0
        assert headers, ["Content-Type:text/xml" == "Cache-Control:max-age=60"]

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WCS&request=GetCoverage&coverage=data&crs=EPSG%3A4326&format=NetCDF4&bbox=0,50,10,60&width=100&height=100",
            {"ADAGUC_CONFIG": config},
        )
        assert status == 0
        assert "Content-Type:application/netcdf" in headers
        assert "Cache-Control:max-age=60" in headers

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WCS&request=GetCoverage&coverage=data&crs=EPSG%3A4326&format=NetCDF4&bbox=0,50,10,60&width=100&height=100&time=2017-01-01T00:05:00Z&DIM_member=member3&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        assert status == 0
        assert "Content-Type:application/netcdf" in headers
        assert "Cache-Control:max-age=7200" in headers

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WCS&request=GetCoverage&coverage=data&crs=EPSG%3A4326&format=NetCDF4&bbox=0,50,10,60&width=100&height=100&time=2017-01-01T00:05:00Z&DIM_member=member3",
            {"ADAGUC_CONFIG": config},
        )
        assert status == 0
        assert "Content-Type:application/netcdf" in headers
        assert "Cache-Control:max-age=60" in headers

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WCS&request=GetCoverage&coverage=data&crs=EPSG%3A4326&format=NetCDF4&bbox=0,50,10,60&width=100&height=100&time=2017-01-01T00:05:00Z&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        assert status == 0
        assert "Content-Type:application/netcdf" in headers
        assert "Cache-Control:max-age=60" in headers

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WCS&request=GetCoverage&coverage=data&crs=EPSG%3A4326&format=NetCDF4&bbox=0,50,10,60&width=100&height=100&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        assert status == 0
        assert "Content-Type:application/netcdf" in headers
        assert "Cache-Control:max-age=60" in headers

        filename = tmp_path / "my_file1"
        filename.write_bytes(data)
        dataset = Dataset(filename, mode="r")
        assert dataset.variables["member"][0] == "member6"  # Largest value (default
        assert dataset.variables["height"][0] == 5000

        # Make sure the correct value ends up in the NetCDF when a dimension is fixed
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WCS&request=GetCoverage&coverage=data_with_fixed&crs=EPSG%3A4326&format=NetCDF4&bbox=0,50,10,60&width=100&height=100&time=2017-01-01T00:05:00Z&elevation=2000",
            {"ADAGUC_CONFIG": config},
        )
        assert status == 0
        assert "Content-Type:application/netcdf" in headers
        assert "Cache-Control:max-age=7200" in headers  # Fully specified with the fixed dimension and the query

        filename = tmp_path / "my_file2"
        filename.write_bytes(data)
        dataset = Dataset(filename, mode="r")
        assert dataset.variables["member"][0] == "member4"  # Fixed dimension
        assert dataset.variables["height"][0] == 2000

        # Test AAIgrid response format
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WCS&request=GetCoverage&coverage=data&crs=EPSG%3A4326&format=aaigrid&bbox=0,50,10,60&width=100&height=100",
            {"ADAGUC_CONFIG": config},
        )
        assert status == 0
        assert "Content-Type:text/plain" in headers
        assert "Cache-Control:max-age=60" in headers

        # Test AAIgrid response format
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "SERVICE=WCS&request=GetCoverage&coverage=data&crs=EPSG%3A4326&format=aaigrid&bbox=0,50,10,60&width=100&height=100&time=2017-01-01T00:05:00Z&DIM_member=member3&elevation=5000",
            {"ADAGUC_CONFIG": config},
        )
        assert status == 0
        assert "Content-Type:text/plain" in headers
        assert "Cache-Control:max-age=7200" in headers

    def test_WCSGetCoverage_error_on_wrong_coverage(self):
        """
        Check if WCS GetCoverage for specified settings returns corresponding error when wrong coverage is provided
        """
        AdagucTestTools().cleanTempDir()
        status, _, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=nonexisting&CRS=EPSG%3A4326&FORMAT=NetCDF4&BBOX=-10,40,20,60",
            env=self.env,
            args=["--report"],
        )

        # Should return 404 Not Found
        assert status == 404

    def test_WCSGetCoverage_error_on_wrong_time(self):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.tests.cacheheader.xml"

        status, _, _ = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        assert status == 0

        status, _, _ = AdagucTestTools().runADAGUCServer(
            "SERVICE=WCS&request=GetCoverage&coverage=data_with_fixed&crs=EPSG%3A4326&format=NetCDF4&bbox=0,50,10,60&width=100&height=100&time=2022-sdfsd-22T00:00:00Z/2022-07-23T00:00:00Z&elevation=2000",
            {"ADAGUC_CONFIG": config},
        )
        assert status == 404

    def test_WCSGetCoverage_no_error_on_correct_time(self):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.tests.cacheheader.xml"

        status, _, _ = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        assert status == 0

        status, _, _ = AdagucTestTools().runADAGUCServer(
            "SERVICE=WCS&request=GetCoverage&coverage=data_with_fixed&crs=EPSG%3A4326&format=NetCDF4&bbox=0,50,10,60&width=100&height=100&time=2017-01-01T00:00:00Z/2017-01-01T00:00:00Z&elevation=2000",
            {"ADAGUC_CONFIG": config},
        )
        assert status == 0

    def test_WCSGetCoverage_error_on_wrong_service(self):
        """
        Check if WCS GetCoverage returns an error status code when Service is wrong
        """
        AdagucTestTools().cleanTempDir()
        status, _, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCCS&REQUEST=GetCoverage&COVERAGE=testdata&CRS=EPSG%3A4326&FORMAT=geotiff&BBOX=-180,-90,180,90&RESX=1&RESY=1",
            env=self.env,
            args=["--report"],
        )
        assert status == 422

    def test_WCSGetCoverage_error_on_wrong_request(self):
        """
        Check if WCS GetCoverage returns an error status code when Request is wrong
        """
        AdagucTestTools().cleanTempDir()
        status, _, _ = AdagucTestTools().runADAGUCServer(
            "source=test/netcdfpointtimeseries/Actuele10mindataKNMIstations_20201220123000.nc&SERVICE=WCS&request=nonexisting&coverage=ff,dd,ta",
            env=self.env,
            args=["--report"],
        )
        assert status == 422

    def test_WCSGetCoverageNetCDF4_testdatanc_native_subset(self):
        """
        Check if WCS GetCoverage for specified settings returns correct grid spec
        """
        AdagucTestTools().cleanTempDir()
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&service=wcs&version=1.1.1&request=getcoverage&format=NetCDF4&crs=EPSG:4326&coverage=testdata&bbox=5.5,52.5,6.5,53.5&time=2024-06-01T01:00:00Z&dim_reference_time=2024-06-01T00:00:00Z&DIM_height=40",
            env=self.env,
            args=["--report"],
        )
        assert status == 0
        assert data[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data)

        expectedgridspec = "width=0&height=1&resx=inf&resy=1.000000&bbox=5.500000,52.500000,6.500000,53.500000&crs=EPSG:4326"
        foundgridspec = ds.getncattr("adaguc_wcs_destgridspec")
        assert foundgridspec == expectedgridspec
        projectionid = ds.variables["crs"].getncattr("id")
        assert "EPSG:4326" == projectionid

    def test_WCSGetCoverageNetCDF4_dataset_testcollection_native_subset(self):
        """
        Check if WCS GetCoverage for specified settings returns correct grid spec
        """
        AdagucTestTools().cleanTempDir()

        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml,testcollection"], env=self.env, isCGI=False
        )
        assert status == 0

        env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml,testcollection"}

        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=testcollection&service=wcs&version=1.1.1&request=getcoverage&format=NetCDF4&crs=EPSG:4326&coverage=testdata&bbox=5.5,52.5,6.5,53.5&time=2024-06-01T01:00:00Z&dim_reference_time=2024-06-01T00:00:00Z&DIM_height=40",
            env={"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml,testcollection"},
            args=["--report"],
            showLog=True,
        )
        assert status == 0

        assert data[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data)

        expectedgridspec = "width=1&height=1&resx=1.000000&resy=1.000000&bbox=5.500000,52.500000,6.500000,53.500000&crs=EPSG:4326"
        foundgridspec = ds.getncattr("adaguc_wcs_destgridspec")
        assert foundgridspec == expectedgridspec
        projectionid = ds.variables["crs"].getncattr("id")
        assert "EPSG:4326" == projectionid
