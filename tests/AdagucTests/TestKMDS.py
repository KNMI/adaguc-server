import json
import os
import pytest
from adaguc.AdagucTestTools import AdagucTestTools

from conftest import make_adaguc_env, update_db

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestKMDS:
    testresultspath = "testresults/TestKMDS/"
    expectedoutputsspath = "expectedoutputs/TestKMDS/"

    AdagucTestTools().mkdir_p(testresultspath)

    @pytest.mark.parametrize(
        ("filename", "layers", "styles"),
        [
            ("test_kmds_alle_stations_10001_10Mwind_barballpoint.png", "10M%2Fwind", "barball%2Fpoint"),
            ("test_kmds_alle_stations_10001_10Mwind_barbthinnedpoint.png", "10M%2Fwind", "barbthinned%2Fpoint"),
            ("test_kmds_alle_stations_10001_10Mwind_barbpoint.png", "10M%2Fwind", "barb%2Fpoint"),
            ("test_kmds_alle_stations_10001_10Mwind_barbdisc.png", "10M%2Fwind", "barbdisc%2Fpoint"),
            ("test_kmds_alle_stations_10001_10Mwind_barbvector.png", "10M%2Fwind", "barbvector%2Fpoint"),
            ("test_kmds_alle_stations_10001_10Mwind_barballvector.png", "10M%2Fwind", "barballvector%2Fpoint"),
            ("test_kmds_alle_stations_10001_10Mwindbft_bftalldisc.png", "10M%2Fwind_bft", "bftalldisc%2Fpoint"),
            ("test_kmds_alle_stations_10001_10Mwindms_barballpoint.png", "10M%2Fwind_mps", "barballpoint%2Fpoint"),
            ("test_kmds_alle_stations_10001_10Mta_observation-temperature.png", "10M%2Fta", "observation.temperature%2Fpoint"),
            # Animated gif KDP_WWWRADARTEMP_loop
            ("test_kmds_alle_stations_10001_10M_animgif_ta_temperaturedisc.png", "10M%2Fta", "temperaturedisc%2Fpoint"),
            # Animated gif KDP_WWWRADARBFT_loop, still uses rendermethod barb
            ("test_kmds_alle_stations_10001_10M_animgif_windbft_bftdiscbarb.png", "10M%2Fwind_bft", "bftdisc%2Fbarb"),
            # Animated gif KDP_WWWRADARWIND_loop, still uses rendermethod barb
            ("test_kmds_alle_stations_10001_10M_animgif_windmps_barbdiscbarb.png", "10M%2Fwind_mps", "barbdisc%2Fbarb"),
            # <SymbolInterval>
            ("test_kmds_alle_stations_10001_10M_nc_symbolinterval_okta.png", "10M%2Fnc", "observation.okta"),
        ],
    )
    def test_kmds_alle_stations_10001_getmap_requests(self, filename: str, layers: str, styles: str):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("adaguc.test.kmds_alle_stations_10001.xml")
        update_db(env)

        status, data, _ = AdagucTestTools().runADAGUCServer(
            f"LAYERS=baselayer,{layers},overlay&STYLES=default,{styles},default&DATASET=adaguc.test.kmds_alle_stations_10001&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&WIDTH=400&HEIGHT=600&CRS=EPSG%3A3857&BBOX=269422.313123934,6357145.5563671775,939865.5563671777,7457638.879961043&FORMAT=image/png32&TRANSPARENT=FALSE&&time=2025-11-06T09%3A20%3A00Z",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())

        assert status == 0
        assert AdagucTestTools().compareImage(
            self.expectedoutputsspath + filename, self.testresultspath + filename, maxAllowedColorDifference=2
        )

    @pytest.mark.parametrize(
        ("filename", "layers", "units"),
        [
            ("test_kmds_alle_stations_10001_10M_animgif_wind.json", "10M%2Fwind", "kts"),  # Should be kts
            ("test_kmds_alle_stations_10001_10M_animgif_windbft.json", "10M%2Fwind_bft", "bft"),  # Should be bft
            ("test_kmds_alle_stations_10001_10M_animgif_windmps.json", "10M%2Fwind_mps", "m s-1"),  # Should be m/s
        ],
    )
    def test_kmds_alle_stations_10001_getfeatureinfo_requests(self, filename: str, layers: str, units: str):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("adaguc.test.kmds_alle_stations_10001.xml")
        update_db(env)

        status, data, _ = AdagucTestTools().runADAGUCServer(
            f"dataset=adaguc.test.kmds_alle_stations_10001&&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS={layers}&QUERY_LAYERS={layers}&CRS=EPSG%3A3857&BBOX=-141702.05839427316,6126251.383284947,1317478.0003938763,7367966.542989185&WIDTH=1550&HEIGHT=1319&I=765&J=585&INFO_FORMAT=application/json&STYLES=&&time=2025-11-06T09%3A20%3A00Z",
            env=env,
        )
        AdagucTestTools().writetojson(self.testresultspath + filename, data.getvalue())
        assert status == 0
        assert json.loads(data.getvalue())[0]["units"] == units

        # TODO: Value does not seem to be stored in the json.
        assert AdagucTestTools().compareFile(self.expectedoutputsspath + filename, self.testresultspath + filename)

    def test_kmds_alle_stations_10001_getmetadata_adjusted_units(self):
        AdagucTestTools().cleanTempDir()

        env = make_adaguc_env("adaguc.test.kmds_alle_stations_10001.xml")
        update_db(env)

        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.test.kmds_alle_stations_10001&&service=WMS&request=getmetadata&format=application/json&layers=10M/wind_adjust",
            env=env,
        )
        assert status == 0

        layer = json.loads(data.getvalue())["adaguc.test.kmds_alle_stations_10001"]["10M/wind_adjust"]["layer"]
        assert layer["variables"][0]["units"] == "mym s -1"
        assert layer["variables"][1]["units"] == "mydegrees"
