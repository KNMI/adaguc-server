import os
from adaguc.AdagucTestTools import AdagucTestTools
from conftest import make_adaguc_env, update_db

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestPointsAndVectors:
    """
    TestPointsAndVectors class to test rendering of points and vector elements.
    """

    testresultspath = "testresults/TestPointsAndVectors/"
    expectedoutputsspath = "expectedoutputs/TestPointsAndVectors/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_WMSGetMap_Barbs_example_windbarbs_from_pointdata_csv(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.vectorrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_Barbs_example_windbarbs_from_pointdata_csv.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.vectorrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_from_pointdata_csv&WIDTH=1024&HEIGHT=1024&CRS=EPSG%3A3857&BBOX=-577014.3843391966,-2101256.2330806395,4248210.613219857,2100649.425887959&STYLES=windbarbs_kts_shaded_withbarbs_for_points%2Fpoint&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xA0F080&&time=2018-12-04T12%3A00%3A00Z&title=How%20to%20render%20windbarbs%20from%20point%20data,%20like%20CSV,%20NetCDF%20or%20GeoJSON&showscalebar=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareImage(
            self.expectedoutputsspath + filename,
            self.testresultspath + filename,
            10,
        )

    def test_WMSGetMap_Discs_example_windbarbs_from_pointdata_csv(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.vectorrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_Discs_example_windbarbs_from_pointdata_csv.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.vectorrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_from_pointdata_csv&WIDTH=1024&HEIGHT=1024&CRS=EPSG%3A3857&BBOX=0.3843391966,-2101256.2330806395,2248210.613219857,2100649.425887959&STYLES=winddiscs_for_points%2Fpoint&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xA0F080&&time=2018-12-04T12%3A00%3A00Z&showscalebar=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareImage(
            self.expectedoutputsspath + filename,
            self.testresultspath + filename,
            10,
        )

    def test_WMSGetMap_Barbs_example_windbarbs_from_pointdata_csv_different_style_options(
        self,
    ):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.vectorrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_Barbs_example_windbarbs_from_pointdata_csv_different_style_options.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.vectorrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_from_pointdata_csv&WIDTH=1024&HEIGHT=1024&CRS=EPSG%3A3857&BBOX=-577014.3843391966,-2101256.2330806395,4248210.613219857,2100649.425887959&STYLES=windbarbs_kts_shaded_withbarbs_for_points_differentoptions&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xA0F080&&time=2018-12-04T12%3A00%3A00Z&title=How%20to%20render%20windbarbs%20from%20point%20data,%20like%20CSV,%20NetCDF%20or%20GeoJSON&showscalebar=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetMap_Barbs_example_windbarbs_on_gridded_netcdf(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.vectorrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_Barbs_example_windbarbs_on_gridded_netcdf.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.vectorrendering.xml&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_on_gridded_netcdf&WIDTH=1352&HEIGHT=1319&CRS=EPSG%3A3857&BBOX=-65800.30054674105,6226801.574078708,1326094.9902426978,7584723.089279351&STYLES=windbarbs_kts_shaded_withbarbs_for_grids%2Fnearestpoint&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xA0F080&&time=2023-09-30T06%3A00%3A00Z&DIM_reference_time=2023-09-28T06%3A00%3A00Z&0.26085315983231405&showlegend=true&title=Wind%20barb%20style%20demo!&showscalebar=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareImage(
            self.expectedoutputsspath + filename,
            self.testresultspath + filename,
            maxAllowedColorDifference=194,
            maxAllowedColorPercentage=0.14,
        )

    def test_WMSGetMap_Discs_example_windbarbs_on_gridded_netcdf(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.vectorrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_Discs_example_windbarbs_on_gridded_netcdf.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.vectorrendering.xml&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_on_gridded_netcdf&WIDTH=1352&HEIGHT=1319&CRS=EPSG%3A3857&BBOX=-65800.30054674105,6226801.574078708,1326094.9902426978,7584723.089279351&STYLES=winddiscs_for_grids%2Fpoint&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0xA0F080&&time=2023-09-30T06%3A00%3A00Z&DIM_reference_time=2023-09-28T06%3A00%3A00Z&0.26085315983231405&showlegend=false&title=Wind disc demo&showscalebar=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareImage(
            self.expectedoutputsspath + filename,
            self.testresultspath + filename,
            30,
        )

    def test_WMSGetMap_Arrows_example_windbarbs_on_gridded_netcdf(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.vectorrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_Arrows_example_windbarbs_on_gridded_netcdf.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.vectorrendering.xml&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_on_gridded_netcdf&WIDTH=1352&HEIGHT=1319&CRS=EPSG%3A3857&BBOX=-65800.30054674105,6226801.574078708,1326094.9902426978,7584723.089279351&STYLES=windarrows_for_grids%2Fpoint&FORMAT=image/png&TRANSPARENT=FALSE&BGCOLOR=0x80F0F0&&time=2023-09-30T06%3A00%3A00Z&DIM_reference_time=2023-09-28T06%3A00%3A00Z&0.26085315983231405&showlegend=false&title=Wind arrows demo&showscalebar=true",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareImage(
            self.expectedoutputsspath + filename,
            self.testresultspath + filename,
            maxAllowedColorDifference=56,
            maxAllowedColorPercentage=0.05,
        )

    def test_WMSGetMap_KNMI_WebSite_AnimatedGifImagery_temperature_styledisc(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.pointrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_KNMI_WebSite_AnimatedGifImagery_temperature_styledisc.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.pointrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=overlaymask,ta&WIDTH=512&HEIGHT=512&CRS=EPSG%3A3857&BBOX=288069.7108512885,6471755.331201249,894206.0083504317,7141909.376801726&STYLES=filledcountries%2Fnearest,discs&FORMAT=image/png&TRANSPARENT=TRUE&&0.8777664780631963",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetMap_KNMI_WebSite_AnimatedGifImagery_temperature_style_temperature(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.pointrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_KNMI_WebSite_AnimatedGifImagery_temperature_style_temperature.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.pointrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=overlaymask,ta&WIDTH=512&HEIGHT=512&CRS=EPSG%3A3857&BBOX=288069.7108512885,6471755.331201249,894206.0083504317,7141909.376801726&STYLES=filledcountries%2Fnearest,temperature&FORMAT=image/png&TRANSPARENT=TRUE&&0.8777664780631963",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetMap_KNMI_WebSite_AnimatedGifImagery_temperature_style_temperature_thinned(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.pointrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_KNMI_WebSite_AnimatedGifImagery_temperature_style_temperature_thinned.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.pointrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=overlaymask,ta&WIDTH=512&HEIGHT=512&CRS=EPSG%3A3857&BBOX=288069.7108512885,6471755.331201249,894206.0083504317,7141909.376801726&STYLES=filledcountries%2Fnearest,temperature_thinned&FORMAT=image/png&TRANSPARENT=TRUE&&0.8777664780631963",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetMap_select_temperature_as_point_from_grid(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.pointrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_select_temperature_as_point_from_grid.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.pointrendering&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=air_temperature_hagl&WIDTH=654&HEIGHT=513&CRS=EPSG:3857&BBOX=-127880.43405139455,6311494.158487529,1447830.4566477134,7547487.563577196&STYLES=temperature_selectpoint%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&time=2024-05-23T01:00:00Z&DIM_reference_time=2024-05-23T00%3A00%3A00Z&DIM_member=1",
            env=env,
            showLog=True,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetMap_windbarbs_on_gridded_netcdf_striding_offset(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.vectorrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_windbarbs_on_gridded_netcdf_striding_offset.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.vectorrendering&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=example_windbarbs_on_gridded_netcdf&WIDTH=443&HEIGHT=493&CRS=EPSG%3A3857&BBOX=-327757.8031974508,5679892.056878187,1599560.8554856647,7824741.038211767&STYLES=vectors_with_striding%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&time=2023-09-30T06%3A00%3A00Z&DIM_reference_time=2023-09-28T06%3A00%3A00Z",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareImage(
            self.expectedoutputsspath + filename,
            self.testresultspath + filename,
        )

    def test_WMSGetMap_windbarbs_kts_selectpoint_for_grids(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.pointrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_windbarbs_kts_selectpoint_for_grids.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.pointrendering&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=windbarbs_kts_selectpoint_for_grids&WIDTH=654&HEIGHT=513&CRS=EPSG:3857&BBOX=-127880.43405139455,6311494.158487529,1447830.4566477134,7547487.563577196&STYLES=windbarbs_kts_selectpoint_for_grids&FORMAT=image/png&TRANSPARENT=TRUE&time=2023-09-30T06:00:00Z&DIM_reference_time=2023-09-28T06:00:00Z",
            env=env,
            showLog=True,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetMap_wave_direction_vector_with_add_dataobject(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.vectorrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_wave_direction_vector_with_add_dataobject.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.vectorrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind_wave_direction&WIDTH=1151&HEIGHT=1135&CRS=EPSG%3A3857&BBOX=-61120.32818755589,6253361.75474204,1285634.726913556,7581395.62749596&STYLES=wave_direction&FORMAT=image/png&TRANSPARENT=TRUE&&time=2026-01-27T03%3A00%3A00Z&DIM_reference_time=2026-01-26T06%3A00%3A00Z&0.7000352259544694",
            env=env,
            showLog=True,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)

    def test_WMSGetMap_wave_direction_vector(self):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.vectorrendering.xml")
        update_db(env)

        filename = "test_WMSGetMap_wave_direction_vector.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.vectorrendering&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind_wave_direction_and_height&WIDTH=1151&HEIGHT=1135&CRS=EPSG%3A3857&BBOX=-61120.32818755589,6253361.75474204,1285634.726913556,7581395.62749596&STYLES=wave_direction&FORMAT=image/png&TRANSPARENT=TRUE&&time=2026-01-27T03%3A00%3A00Z&DIM_reference_time=2026-01-26T06%3A00%3A00Z",
            env=env,
            showLog=True,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert data == AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)
