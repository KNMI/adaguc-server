# pylint: disable=line-too-long
# pylint: disable=unused-variable
# pylint: disable=invalid-name

"""
Run test for WCS Server system of adaguc-server
"""

import os
from pathlib import Path
import re
import netCDF4
from netCDF4 import Dataset
from adaguc.AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


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
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
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
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
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
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
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
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
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
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
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
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
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
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
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
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        assert status == 0
        # Different gdal versions give different spaces in the output.
        # Compare in a way where any sequence of whitespace characters is equivalent
        # This means that newlines (which are important?) are not compared
        test_output = data.getvalue()
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
        assert data.getvalue()[0:10] == b"CDF\x02\x00\x00\x00\x00\x00\x00"

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
        assert data.getvalue()[0:6] == b"\x89HDF\r\n"

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
        assert status == 0
        assert data.getvalue()[0:10] == b"II*\x00\x08\x00\x00\x00\x12\x00"

    # TODO: This test (which uses GDAL) produces incorrect results. Made a ticket in backlog.
    def test_WCSGetCoverageAAIGRID_NATIVE_testdatanc(self):
        """
        Check if WCS GetCoverage for testdata.nc as Native grid is OK
        """
        AdagucTestTools().cleanTempDir()
        filename = "test_WCSGetCoverageAAIGRID_NATIVE_testdatanc.grd"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=testdata.nc&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=testdata&FORMAT=aaigrid&", env=self.env, args=["--report"]
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        assert status == 0
        lines = str(data.getvalue().decode("UTF-8")).splitlines()[0:6]
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
        assert data.getvalue()[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data.getvalue())

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
        assert data.getvalue()[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data.getvalue())

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
        assert data.getvalue()[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data.getvalue())

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
        assert data.getvalue()[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data.getvalue())

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
        assert data.getvalue()[0:6] == b"\x89HDF\r\n"
        ds = netCDF4.Dataset("filename.nc", memory=data.getvalue())

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
        assert data.getvalue()[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data.getvalue())

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
        filename.write_bytes(data.getvalue())
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
        filename.write_bytes(data.getvalue())
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
        assert data.getvalue()[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data.getvalue())

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

        assert data.getvalue()[0:6] == b"\x89HDF\r\n"

        ds = netCDF4.Dataset("filename.nc", memory=data.getvalue())

        expectedgridspec = "width=1&height=1&resx=1.000000&resy=1.000000&bbox=5.500000,52.500000,6.500000,53.500000&crs=EPSG:4326"
        foundgridspec = ds.getncattr("adaguc_wcs_destgridspec")
        assert foundgridspec == expectedgridspec
        projectionid = ds.variables["crs"].getncattr("id")
        assert "EPSG:4326" == projectionid
