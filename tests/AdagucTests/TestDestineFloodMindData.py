"""
The following tests if adaguc works properly with a file with only a dimension and no dimension variable. Adaguc will use an index for ensemble in this case.
"""

import os
import unittest
from adaguc.AdagucTestTools import AdagucTestTools


from conftest import (
    make_adaguc_autoresource_env,
    run_adaguc_and_compare_getcapabilities,
    run_adaguc_and_compare_image,
    run_adaguc_and_compare_json,
)

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestDestineFloodmindData(unittest.TestCase):
    """
    The following tests if adaguc works properly with a file with only a dimension and no dimension variable. Adaguc will use an index for ensemble in this case.
    """

    testresultspath = "testresults/TestDestineFloodmindData/"
    expectedoutputsspath = "expectedoutputs/TestDestineFloodmindData/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    # //data/datasets/destine_floodmind/netcdf_hdsr_small_no_ensemble_variable.nc
    AdagucTestTools().mkdir_p(testresultspath)

    def test_getcapabilities_netcdf_withensembledim_withoutensemblevariable(self):
        """
        The following tests if adaguc works properly with a file with only a dimension and no dimension variable. Adaguc will use an index for ensemble in this case.
        """
        env = make_adaguc_autoresource_env(self.testresultspath, self.expectedoutputsspath)
        run_adaguc_and_compare_getcapabilities(
            env,
            "test_GetCapabilities_NetCDF_WithEnsembleDim_WithoutEnsembleVariable.xml",
            "source=destine_floodmind/netcdf_hdsr_small_no_ensemble_variable.nc&service=WMS&request=GetCapabilities",
        )

    def test_getmap_netcdf_withensembledim_withoutensemblevariable(self):
        """
        The following tests if adaguc GetMap works properly with a file with only a dimension and no dimension variable. Adaguc will use an index for ensemble in this case.
        """
        env = make_adaguc_autoresource_env(self.testresultspath, self.expectedoutputsspath)
        run_adaguc_and_compare_image(
            env,
            "test_GetMap_NetCDF_WithEnsembleDim_WithoutEnsembleVariable.png",
            "source=destine_floodmind/netcdf_hdsr_small_no_ensemble_variable.nc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=realwaterdepth&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=550407.2218721084,6827106.217354822,553123.1857119965,6828901.88467553&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&time=2020-01-01T22%3A00%3A00Z&DIM_ensemble=2&COLORSCALERANGE=0,1.2&",
        )

    def test_getgfitimeseries_netcdf_withensembledim_withoutensemblevariable(self):
        """
        The following tests if adaguc GetFeatureInfo timeseries works properly with a file with only a dimension and no dimension variable. Adaguc will use an index for ensemble in this case.
        """
        env = make_adaguc_autoresource_env(self.testresultspath, self.expectedoutputsspath)
        run_adaguc_and_compare_json(
            env,
            "test_GetGFITimeSeries_NetCDF_WithEnsembleDim_WithoutEnsembleVariable.json",
            "source=destine_floodmind/netcdf_hdsr_small_no_ensemble_variable.nc&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=realwaterdepth&query_layers=realwaterdepth&crs=EPSG%3A3857&bbox=550412.4549430908%2C6826278.445858715%2C553128.4187829789%2C6829729.656171637&width=1038&height=1319&i=110&j=666&format=image%2Fgif&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_ensemble=*&",
        )
        run_adaguc_and_compare_json(
            env,
            "test_GetGFITimeSeries_NetCDF_WithEnsembleDim_WithoutEnsembleVariable_ensemble_1.json",
            "source=destine_floodmind/netcdf_hdsr_small_no_ensemble_variable.nc&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=realwaterdepth&query_layers=realwaterdepth&crs=EPSG%3A3857&bbox=550412.4549430908%2C6826278.445858715%2C553128.4187829789%2C6829729.656171637&width=1038&height=1319&i=110&j=666&format=image%2Fgif&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_ensemble=1&",
        )
        run_adaguc_and_compare_json(
            env,
            "test_GetGFITimeSeries_NetCDF_WithEnsembleDim_WithoutEnsembleVariable_ensemble_2.json",
            "source=destine_floodmind/netcdf_hdsr_small_no_ensemble_variable.nc&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=realwaterdepth&query_layers=realwaterdepth&crs=EPSG%3A3857&bbox=550412.4549430908%2C6826278.445858715%2C553128.4187829789%2C6829729.656171637&width=1038&height=1319&i=110&j=666&format=image%2Fgif&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_ensemble=2&",
        )
        run_adaguc_and_compare_json(
            env,
            "test_GetGFITimeSeries_NetCDF_WithEnsembleDim_WithoutEnsembleVariable_ensemble_3.json",
            "source=destine_floodmind/netcdf_hdsr_small_no_ensemble_variable.nc&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=realwaterdepth&query_layers=realwaterdepth&crs=EPSG%3A3857&bbox=550412.4549430908%2C6826278.445858715%2C553128.4187829789%2C6829729.656171637&width=1038&height=1319&i=110&j=666&format=image%2Fgif&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_ensemble=3&",
        )
