#include "CDebugger.h"

#include "CppUnitLite/TestHarness.h"
#include "CTString.h"
#include "CImageWarper.h"
#include "CImgRenderFieldVectors.h"
#include "f8vector.h"
#include "ProjCache.h"
#include "CDrawFunction.h"
#include "CCDFHDF5IO.h"
#include "CCDFObject.h"
#include "CDataSource.h"
#include "CProj4ToCF.h"
#include "CServerConfig_CPPXSD.h"
#include "CServerParams.h"
#include "CPGSQLDB.h"
#include "Types/CPointTypes.h"
#include "Types/GeoParameters.h"
#include "GenericDataWarper/gdwDrawTriangle.h"
#include <vector>
#include <chrono>
#include <cstdint>

// To test this file do in the ./bin folder of adaguc-server:
// cmake --build . --config Debug --target testadagucserver -j 10 -- && ctest --verbose

int main() {

  TestResult tr;
  TestRegistry::runAllTests(tr);

  proj_clear_cache();

  if (tr.failureCount != 0) {
    return 1;
  }
  return 0;
}

TEST(alreadyMeter, CImageWarper) {
  std::string projStringIn("+proj=stere +lat_0=90 +lon_0=0 +lat_ts=60 +a=6378140 +b=6356750 +x_0=0 y_0=0");
  std::string projStringOut;
  double scaling;
  std::tie(projStringOut, scaling) = CImageWarper::fixProjection(projStringIn);
  CHECK(scaling == 1.0);
  CHECK(projStringOut == projStringIn);
}

TEST(kilometerToMeter, CImageWarper) {
  std::string projStringIn("+proj=stere +lat_0=90 +lon_0=0 +lat_ts=60 +a=6378.14 +b=6356.75 +x_0=0 y_0=0");
  std::string projStringOutExpected("+proj=stere +lat_0=90.000000 +lon_0=0.000000 +lat_ts=60.000000 +a=6378140.000000 +b=6356750.000000 +x_0=0.000000 +y_0=0.000000 +ellps=WGS84 +datum=WGS84");
  std::string projStringOut;
  double scaling;
  std::tie(projStringOut, scaling) = CImageWarper::fixProjection(projStringIn);
  CDBDebug("projStringOut [%s]", projStringOut.c_str());
  CHECK(scaling == 1000.0);
  CHECK(projStringOut == projStringOutExpected);
}

TEST(kilometerToMeterObTran, CImageWarper) {
  std::string projStringIn("+proj=ob_tran +o_proj=longlat +lon_0=5.449997 +o_lat_p=37.250000 +o_lon_p=0.000000 +a=6378.14 +b=6356.75 +x_0=0.000000 +y_0=0.000000 +no_defs");
  std::string projStringOutExpected("+proj=ob_tran +o_proj=longlat +lon_0=5.449997 +o_lat_p=37.250000 +o_lon_p=0.000000 +a=6378140.000000 +b=6356750.000000 +x_0=0.000000 +y_0=0.000000 +no_defs");
  std::string projStringOut;
  double scaling;
  std::tie(projStringOut, scaling) = CImageWarper::fixProjection(projStringIn);
  CHECK(scaling == 1000.0);
  CHECK(projStringOut == projStringOutExpected);
}

TEST(meterToMeterObTran, CImageWarper) {
  std::string projStringIn("+proj=ob_tran +o_proj=longlat +lon_0=5.449997 +o_lat_p=37.250000 +o_lon_p=0.000000 +a=6378140.000000 +b=6356750.000000 +x_0=0.000000 +y_0=0.000000 +no_defs");
  std::string projStringOutExpected("+proj=ob_tran +o_proj=longlat +lon_0=5.449997 +o_lat_p=37.250000 +o_lon_p=0.000000 +a=6378140.000000 +b=6356750.000000 +x_0=0.000000 +y_0=0.000000 +no_defs");
  std::string projStringOut;
  double scaling;
  std::tie(projStringOut, scaling) = CImageWarper::fixProjection(projStringIn);
  CHECK(scaling == 1);
  CHECK(projStringOut == projStringOutExpected);
}

TEST(radians, CImageWarper) {
  std::string projStringIn("+proj=geos +lon_0=0.000000 +lat_0=0.000000 +h=1.000000 +a=0.178231 +b=0.177633 +sweep=y");
  std::string projStringOut;
  double scaling;
  std::tie(projStringOut, scaling) = CImageWarper::fixProjection(projStringIn);
  CHECK(scaling == 1.0);
  CHECK(projStringOut == projStringIn);
}

// From DINI point to latlon
TEST(CImgRenderFieldVectors, jacobianTransformUWCWDini) {
  std::string crs = "+proj=ob_tran +o_proj=longlat +lon_0=-8.0 +o_lat_p=35.0 +o_lon_p=0.0 +a=6367470 +e=0 +no_defs";

  GeoParameters geo;
  geo.crs = crs;
  std::vector<CServerConfig::XMLE_Projection> v;

  // Init warper
  CImageWarper warper;
  int status = warper.initreproj(crs.c_str(), geo, &v);
  CHECK(status == 0);

  // Grid cell to do the calculation for
  f8point gridCoordLL = {.x = 1.1, .y = 1.4};
  f8point gridCoordUR = {.x = 1.15, .y = 1.45};

  // Make speed vector
  f8component speedVector = {.u = -6.222803, .v = 4.235688};
  DOUBLES_EQUAL(speedVector.magnitude(), 7.527, 0.001);
  DOUBLES_EQUAL(speedVector.angledeg(), 124.242, 0.001);
  DOUBLES_EQUAL(speedVector.direction(), 2.543957, 0.001);

  // Check if projection transformation from model to latlon works:
  f8point gridCoordLLtoLatLon = gridCoordLL;
  warper.reprojModelToLatLon(gridCoordLLtoLatLon);
  DOUBLES_EQUAL(gridCoordLLtoLatLon.x, -6.013390, 0.001);
  DOUBLES_EQUAL(gridCoordLLtoLatLon.y, 56.384378, 0.001);

  f8point gridCoordURtoLatLon = gridCoordUR;
  warper.reprojModelToLatLon(gridCoordURtoLatLon);
  DOUBLES_EQUAL(gridCoordURtoLatLon.x, -5.920456, 0.001);
  DOUBLES_EQUAL(gridCoordURtoLatLon.y, 56.432904, 0.001);

  f8component compGridRel = jacobianTransform(speedVector, gridCoordLL, gridCoordUR, &warper, true);
  DOUBLES_EQUAL(compGridRel.u, -6.099962, 0.001);
  DOUBLES_EQUAL(compGridRel.v, 4.410759, 0.001);
  DOUBLES_EQUAL(compGridRel.magnitude(), 7.527571, 0.001);
  DOUBLES_EQUAL(compGridRel.direction(), 2.515544, 0.001);

  f8component compNoGridRel = jacobianTransform(speedVector, gridCoordLL, gridCoordUR, &warper, false);
  DOUBLES_EQUAL(compNoGridRel.u, -6.222803, 0.001);
  DOUBLES_EQUAL(compNoGridRel.v, 4.235688, 0.001);
  DOUBLES_EQUAL(compNoGridRel.magnitude(), 7.527571, 0.001);
  DOUBLES_EQUAL(compNoGridRel.direction(), 2.543957, 0.001);

  // Magnitudes should be the same
  DOUBLES_EQUAL(compNoGridRel.magnitude(), compGridRel.magnitude(), 0.001);
  DOUBLES_EQUAL(speedVector.magnitude(), compGridRel.magnitude(), 0.001);

  // Direction for non grid rel should be the same
  DOUBLES_EQUAL(speedVector.direction(), compNoGridRel.direction(), 0.001);
}

// From latlon to latlon
TEST(CImgRenderFieldVectors, jacobianTransformLatLon) {
  std::string crs = "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs";

  GeoParameters geo;
  geo.crs = crs;
  std::vector<CServerConfig::XMLE_Projection> v;

  // Init warper
  CImageWarper warper;
  int status = warper.initreproj(crs.c_str(), geo, &v);
  CHECK(status == 0);

  // Grid cell to do the calculation for
  f8point gridCoordLL = {.x = 1.1, .y = 1.4};
  f8point gridCoordUR = {.x = 1.15, .y = 1.45};

  // Make speed vector
  f8component speedVector = {.u = -6.222803, .v = 4.235688};
  DOUBLES_EQUAL(speedVector.magnitude(), 7.527, 0.001);
  DOUBLES_EQUAL(speedVector.angledeg(), 124.242, 0.001);
  DOUBLES_EQUAL(speedVector.direction(), 2.543957, 0.001);

  // Check if projection transformation from model to latlon works:
  f8point gridCoordLLtoLatLon = gridCoordLL;
  warper.reprojModelToLatLon(gridCoordLLtoLatLon);
  DOUBLES_EQUAL(gridCoordLLtoLatLon.x, 1.1, 0.001);
  DOUBLES_EQUAL(gridCoordLLtoLatLon.y, 1.4, 0.001);

  f8point gridCoordURtoLatLon = gridCoordUR;
  warper.reprojModelToLatLon(gridCoordURtoLatLon);
  DOUBLES_EQUAL(gridCoordURtoLatLon.x, 1.15, 0.001);
  DOUBLES_EQUAL(gridCoordURtoLatLon.y, 1.45, 0.001);

  f8component compGridRel = jacobianTransform(speedVector, gridCoordLL, gridCoordUR, &warper, true);
  DOUBLES_EQUAL(compGridRel.u, -6.224651, 0.001);
  DOUBLES_EQUAL(compGridRel.v, 4.232972, 0.001);
  DOUBLES_EQUAL(compGridRel.magnitude(), 7.527571, 0.001);
  DOUBLES_EQUAL(compGridRel.direction(), 2.544393, 0.001);

  f8component compNoGridRel = jacobianTransform(speedVector, gridCoordLL, gridCoordUR, &warper, false);
  DOUBLES_EQUAL(compNoGridRel.u, -6.222803, 0.001);
  DOUBLES_EQUAL(compNoGridRel.v, 4.235688, 0.001);
  DOUBLES_EQUAL(compNoGridRel.magnitude(), 7.527571, 0.001);
  DOUBLES_EQUAL(compNoGridRel.direction(), 2.543957, 0.001);

  // Magnitudes should be the same
  DOUBLES_EQUAL(compNoGridRel.magnitude(), compGridRel.magnitude(), 0.001);
  DOUBLES_EQUAL(speedVector.magnitude(), compGridRel.magnitude(), 0.001);

  // Direction for non grid rel should be the same
  DOUBLES_EQUAL(speedVector.direction(), compNoGridRel.direction(), 0.001);
}

f8component testDiniCoordinate(f8point pointToCheck, f8component speedVector) {
  std::string crs = "+proj=ob_tran +o_proj=longlat +lon_0=-8.0 +o_lat_p=35.0 +o_lon_p=0.0 +a=6367470 +e=0 +no_defs";

  GeoParameters geo;
  geo.crs = crs;
  std::vector<CServerConfig::XMLE_Projection> v;

  // Init warper
  CImageWarper warper;
  warper.initreproj(crs.c_str(), geo, &v);

  double cellSize = 0.05;
  f8point gridCoordLL = {.x = pointToCheck.x, .y = pointToCheck.y};
  f8point gridCoordUR = {.x = pointToCheck.x + cellSize, .y = pointToCheck.y + cellSize};

  // Make speed vector

  // Check if projection transformation from model to latlon works:
  f8point gridCoordLLtoLatLon = gridCoordLL;
  warper.reprojModelToLatLon(gridCoordLLtoLatLon);

  f8point gridCoordURtoLatLon = gridCoordUR;
  warper.reprojModelToLatLon(gridCoordURtoLatLon);

  f8component compGridRel = jacobianTransform(speedVector, gridCoordLL, gridCoordUR, &warper, true);
  return compGridRel;
}

// CHECK a more dini points
TEST(CImgRenderFieldVectors, jacobianTransformUWCWDiniMultiplePoints) {
  auto resultA = testDiniCoordinate({.x = 1.1, .y = 1.4}, {.u = -6.222803, .v = 4.235688});
  DOUBLES_EQUAL(resultA.u, -6.099962, 0.001);
  DOUBLES_EQUAL(resultA.v, 4.410759, 0.001);

  auto resultB = testDiniCoordinate({.x = -13.5, .y = -13.6}, {.u = -6.222803, .v = 4.235688});
  DOUBLES_EQUAL(resultB.u, -7.080669, 0.001);
  DOUBLES_EQUAL(resultB.v, 2.555084, 0.001);

  auto resultC = testDiniCoordinate({.x = -13.5, .y = 14.55}, {.u = -6.222803, .v = 4.235688});
  DOUBLES_EQUAL(resultC.u, -7.487193, 0.001);
  DOUBLES_EQUAL(resultC.v, 0.778638, 0.001);

  auto resultD = testDiniCoordinate({.x = 20.25, .y = 14.55}, {.u = -6.222803, .v = 4.235688});
  DOUBLES_EQUAL(resultD.u, -2.288372, 0.001);
  DOUBLES_EQUAL(resultD.v, 7.171310, 0.001);
}

TEST(CProj4ToCF, azimuthal_equidistant) {
  CProj4ToCF proj4ToCF;
  CDF::Variable var;
  var.setName("azimuthal_equidistant");
  var.setAttributeText("grid_mapping_name", "azimuthal_equidistant");
  double longitude_of_projection_origin = 174.7;
  double latitude_of_projection_origin = -41.2;
  double earth_radius = 6370997.0;
  double false_easting = 0.;
  double false_northing = 0.;
  var.setAttribute("longitude_of_projection_origin", CDF_DOUBLE, &longitude_of_projection_origin, 1);
  var.setAttribute("latitude_of_projection_origin", CDF_DOUBLE, &latitude_of_projection_origin, 1);
  var.setAttribute("earth_radius", CDF_DOUBLE, &earth_radius, 1);
  var.setAttribute("false_easting", CDF_DOUBLE, &false_easting, 1);
  var.setAttribute("false_northing", CDF_DOUBLE, &false_northing, 1);
  std::string projString = proj4ToCF.convertCFToProj(&var);
  CDBDebug("projString [%s]", projString.c_str());

  std::string expected = "+proj=aeqd +lat_0=-41.200000 +lon_0=174.700000 +k_0=1.0 +x_0=0.000000 +y_0=0.000000 +a=6378140.000000 +b=6378140.000000 ";
  CHECK(projString == expected);
}

TEST(string, knmiH5TimeToISOString) {
  CHECK("20210622T200000" == knmiH5TimeToISOString("22-JUN-2021;20:00:00.000"));
  CHECK("20210622T200000" == knmiH5TimeToISOString("  22-JUN-2021;20:00:00.000    "));
  CHECK("20210622T200000" == knmiH5TimeToISOString("  22-JUN-2021;20:00:00.000    "));
  CHECK("20210122T200000" == knmiH5TimeToISOString("  22-jan-2021;20:00:00.000    "));
  CHECK("20211222T200000" == knmiH5TimeToISOString("  22-dec-2021;20:00:00    "));
  CHECK("20211222T123456" == knmiH5TimeToISOString("22-dec-2021;12:34:56.789    "));

  auto t = knmiH5TimeToISOString("22-dec-2021;12:34:56.789    ");
  t[13] = '0';
  t[14] = '0';
  CDBDebug("t [%s]", t.c_str());
  CHECK("20211222T123400" == t);
}

TEST(CDrawFunction, determinePixelColorFromValue) {
  GDWDrawFunctionSettings settings = {
      .dfNodataValue = -9999,
      .legendValueRange = 0,
      .legendLowerRange = 0,
      .legendUpperRange = 0,
      .isUsingShadeIntervals = true,
      .bgColorDefined = false,
      .shadeInterval = 0,
      .legendLog = 0,
      .legendLogAsLog = 0,
      .legendScale = 0,
      .legendOffset = 0,
      .bgColor = CColor(0, 0, 0, 255),
      .hasNodataValue = true,
      .drawgridboxoutline = false,
      .drawgrid = true,
      .interpolationMethod = InterpolationMethodNearest,
      .intervals = {},
      .drawImage = nullptr,
      .destinationDataType = CDF_FLOAT,
      .destinationGrid = nullptr,
      .smoothingFiter = 0,
      .smoothingMemo = {},
  };
  // Test if it returns default color when there are no shadeintervals
  CHECK(determinePixelColorFromValue(0, &settings).c_str() == CColor(0, 0, 0, 0).c_str());

  settings.intervals.push_back({.min = -1, .max = 0, .color = CColor(50, 50, 50, 50)});
  settings.intervals.push_back({.min = 0, .max = 1, .color = CColor(100, 100, 100, 100)});
  settings.intervals.push_back({.min = 1, .max = 2, .color = CColor(150, 150, 150, 150)});

  // Test if correct colors are returned based on shade intervals
  CHECK(determinePixelColorFromValue(-2, &settings).c_str() == CColor(0, 0, 0, 0).c_str());
  CHECK(determinePixelColorFromValue(-1, &settings).c_str() == CColor(50, 50, 50, 50).c_str());
  CHECK(determinePixelColorFromValue(0, &settings).c_str() == CColor(100, 100, 100, 100).c_str());
  CHECK(determinePixelColorFromValue(1, &settings).c_str() == CColor(150, 150, 150, 150).c_str());
  CHECK(determinePixelColorFromValue(2, &settings).c_str() == CColor(0, 0, 0, 0).c_str());
}

// Regression test for the SQL injection fix in CDBAdapterPostgreSQL::getFilesAndIndicesForDimensions /
// getReferenceTime: a request-supplied dimension value (e.g. TIME=...) used to be spliced unescaped into a
// single-quoted SQL literal. CPGSQLDB::escapeString() must now be used at those call sites.
//
// This test connects to the same local test database used by the python test suite. If no database is
// reachable (e.g. when ctest runs during a Docker build stage without a postgres service), the test is
// skipped rather than failed, since this is an infrastructure limitation, not a code regression.
TEST(escapeStringPreventsSQLInjection, CPGSQLDB) {
  CPGSQLDB db;
  if (db.connect("user=adaguc password=adaguc host=localhost dbname=postgres port=54321") != 0) {
    CDBDebug("Skipping escapeStringPreventsSQLInjection: no test database reachable");
    return;
  }

  // A classic UNION-based SQL injection payload. If this string were interpolated into
  // `CT::printf("SELECT '%s' as val", payload.c_str())` unescaped, it would terminate the string
  // literal early and inject a second SELECT, making the query return two rows.
  std::string payload = "x' UNION SELECT 'pwned";

  // Demonstrate the vulnerability still exists if a value is NOT escaped (the old, fixed code path):
  // this mirrors exactly how the value used to be interpolated before the fix.
  std::string unescapedQuery = CT::printf("SELECT '%s' as val", payload.c_str());
  CDBStore::Store *vulnerableResult = db.queryToStore(unescapedQuery.c_str(), true);
  CHECK(vulnerableResult != NULL);
  CHECK(vulnerableResult->records.size() == 2); // proves the UNION injection fired
  delete vulnerableResult;

  // Now demonstrate the fix: escape the same payload before interpolating it, exactly like
  // CDBAdapterPostgreSQL.cpp now does via DB->escapeString(...).
  std::string escapedQuery = CT::printf("SELECT '%s' as val", db.escapeString(payload).c_str());
  CDBStore::Store *fixedResult = db.queryToStore(escapedQuery.c_str(), true);
  CHECK(fixedResult != NULL);
  CHECK(fixedResult->records.size() == 1); // no injection: a single literal value came back
  CHECK(fixedResult->records[0].values[0] == payload); // and it round-trips to the exact original value
  delete fixedResult;

  // A benign value must still round-trip unchanged.
  std::string benign = "2024-01-01T00:00:00Z";
  std::string benignQuery = CT::printf("SELECT '%s' as val", db.escapeString(benign).c_str());
  CDBStore::Store *benignResult = db.queryToStore(benignQuery.c_str(), true);
  CHECK(benignResult != NULL);
  CHECK(benignResult->records.size() == 1);
  CHECK(benignResult->records[0].values[0] == benign);
  delete benignResult;
}

// Regression test for the unvalidated-JSONP-callback fix in CRequest.cpp: the JSONP query parameter is
// reflected verbatim into the response body as `<callback>(<data>)`. It must now be restricted to a safe
// identifier charset before being accepted, using the same checkForValidTokens() helper as here.
TEST(jsonpCallbackCharsetIsRestricted, CRequest) {
  const std::string allowedJSONPTokens = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_$.";

  // Legitimate callback names must be accepted.
  CHECK(checkForValidTokens("myCallback", allowedJSONPTokens) == true);
  CHECK(checkForValidTokens("jQuery123_456", allowedJSONPTokens) == true);
  CHECK(checkForValidTokens("ns.callback", allowedJSONPTokens) == true);

  // Payloads attempting to break out of the `callback(...)` context must be rejected.
  CHECK(checkForValidTokens("</script><script>alert(1)</script>", allowedJSONPTokens) == false);
  CHECK(checkForValidTokens("alert(1)", allowedJSONPTokens) == false);
  CHECK(checkForValidTokens("a');alert(1);//", allowedJSONPTokens) == false);
  CHECK(checkForValidTokens("callback;DROP TABLE x", allowedJSONPTokens) == false);
}
// Draws a mesh of skewed quads (two triangles each, like CGenericDataWarper) and counts pixel writes.
// Must partition pixels exactly: double-written stipples, un-written leaves holes.
static void drawSkewedMeshAndCountWrites(int cellsX, int cellsY, double cellSize, double skew, double jitter, std::vector<int> &writes, int W, int H, double originX, double originY, int &dxOutOfRange, bool useTriangles = false) {
  GDWState state = {};
  state.destGridWidth = W;
  state.destGridHeight = H;
  writes.assign(W * H, 0);
  dxOutOfRange = 0;
  // Deterministic pseudo random jitter of the mesh nodes
  unsigned int seed = 12345;
  auto rnd = [&seed]() {
    seed = seed * 1664525u + 1013904223u;
    return ((seed >> 8) & 0xFFFF) / 65535.0 * 2 - 1;
  };
  std::vector<double> nodeX((cellsX + 1) * (cellsY + 1)), nodeY((cellsX + 1) * (cellsY + 1));
  for (int j = 0; j <= cellsY; j++) {
    for (int i = 0; i <= cellsX; i++) {
      nodeX[i + j * (cellsX + 1)] = originX + i * cellSize + j * skew + rnd() * jitter;
      nodeY[i + j * (cellsX + 1)] = originY + j * cellSize + i * skew * 0.5 + rnd() * jitter;
    }
  }
  auto drawFunction = [&](int x, int y, float, GDWState &s) {
    if (x < 0 || y < 0 || x >= W || y >= H) return;
    writes[x + y * W]++;
    if (s.sourceTileDx < -1e-6 || s.sourceTileDx > 1 + 1e-6 || s.sourceTileDy < -1e-6 || s.sourceTileDy > 1 + 1e-6) dxOutOfRange++;
  };
  for (int j = 0; j < cellsY; j++) {
    for (int i = 0; i < cellsX; i++) {
      int n00 = i + j * (cellsX + 1), n10 = n00 + 1, n01 = n00 + (cellsX + 1), n11 = n01 + 1;
      const double qx[4] = {nodeX[n00], nodeX[n10], nodeX[n11], nodeX[n01]};
      const double qy[4] = {nodeY[n00], nodeY[n10], nodeY[n11], nodeY[n01]};
      if (useTriangles) {
        const double ax[3] = {qx[0], qx[1], qx[2]}, ay[3] = {qy[0], qy[1], qy[2]};
        const double bx[3] = {qx[2], qx[0], qx[3]}, by[3] = {qy[2], qy[0], qy[3]};
        gdwDrawTriangle(ax, ay, 0.0f, false, state, drawFunction);
        gdwDrawTriangle(bx, by, 0.0f, true, state, drawFunction);
      } else {
        gdwDrawQuad(qx, qy, 0.0f, state, drawFunction);
      }
    }
  }
}

TEST(gdwDrawTriangleNeverWritesAPixelTwice, GenericDataWarper) {
  const int W = 120, H = 120;
  struct Config {
    double cellSize, skew, jitter;
  };
  // Skew/jitter varies edge slope and fractional offset; jitter stays small enough that rounded quads stay convex (a reflex quad self-overlaps).
  const Config configs[] = {{6.0, 0.0, 0.0}, {6.3, 1.7, 0.8}, {4.4, 2.3, 0.4}, {9.7, 3.1, 1.5}, {3.3, 0.9, 0.3}, {12.5, 5.5, 2.5}};
  for (const auto &c: configs) {
    std::vector<int> writes;
    int outOfRange = 0;
    int cells = (int)(80 / c.cellSize);
    drawSkewedMeshAndCountWrites(cells, cells, c.cellSize, c.skew, c.jitter, writes, W, H, 10.3, 10.7, outOfRange);
    int twice = 0, total = 0;
    for (int v: writes) {
      if (v > 1) twice++;
      if (v > 0) total++;
    }
    CHECK(twice == 0);
    CHECK(total > 0);
  }
}

TEST(gdwDrawTriangleLeavesNoHolesInsideTheMesh, GenericDataWarper) {
  const int W = 120, H = 120;
  struct Config {
    double cellSize, skew, jitter;
  };
  const Config configs[] = {{6.0, 0.0, 0.0}, {6.3, 1.7, 0.8}, {4.4, 2.3, 0.4}, {9.7, 3.1, 1.5}};
  for (const auto &c: configs) {
    std::vector<int> writes;
    int outOfRange = 0;
    int cells = (int)(60 / c.cellSize);
    drawSkewedMeshAndCountWrites(cells, cells, c.cellSize, c.skew, c.jitter, writes, W, H, 10.3, 10.7, outOfRange);
    // Skew turns the mesh into a parallelogram, so only check a margin-reduced interior box.
    double margin = c.skew * cells + 2 * c.jitter + 3;
    int holes = 0;
    for (int y = (int)(10.7 + c.skew * 0.5 * cells + 3 * c.jitter + 2); y < (int)(10.7 + cells * c.cellSize - c.skew * 0.5 * cells - 3 * c.jitter - 2); y++) {
      for (int x = (int)(10.3 + margin); x < (int)(10.3 + cells * c.cellSize - margin); x++) {
        if (writes[x + y * W] == 0) holes++;
      }
    }
    CHECK(holes == 0);
  }
}

TEST(gdwDrawTriangleKeepsBarycentricWeightsInRange, GenericDataWarper) {
  const int W = 120, H = 120;
  std::vector<int> writes;
  int outOfRange = 0;
  drawSkewedMeshAndCountWrites(12, 12, 6.3, 1.7, 0.8, writes, W, H, 10.3, 10.7, outOfRange);
  CHECK(outOfRange == 0);
}

// Benchmark of gdwDrawTriangle, only runs when ADAGUC_BENCH is set, e.g.: ADAGUC_BENCH=1 ./adagucserverEC/testadagucserver
// Checksum covers every written pixel and its weights; an optimization must not change it.
static void benchmarkTriangleMesh(const char *name, int cells, double destSize, double rotation) {
  const int W = (int)destSize, H = (int)destSize;
  GDWState state = {};
  state.destGridWidth = W;
  state.destGridHeight = H;
  const int N = cells + 1;
  std::vector<double> nodeX(N * N), nodeY(N * N);
  const double step = destSize / cells * 0.95, c = cos(rotation), s = sin(rotation);
  for (int j = 0; j < N; j++) {
    for (int i = 0; i < N; i++) {
      // A rotated, slightly curved grid, like a reprojected source grid
      double u = i * step, v = j * step + 0.002 * destSize * sin(i * 0.05);
      nodeX[i + j * N] = destSize * 0.02 + u * c - v * s;
      nodeY[i + j * N] = destSize * 0.30 + u * s + v * c;
    }
  }
  uint64_t checksum = 0, writes = 0;
  auto drawFunction = [&](int x, int y, float, GDWState &st) {
    writes++;
    checksum += (uint64_t)(x * 31 + y * 17) + (uint64_t)(st.sourceTileDx * 1000) * 7 + (uint64_t)(st.sourceTileDy * 1000) * 13;
  };
  double best = 1e30;
  for (int run = 0; run < 5; run++) {
    checksum = 0;
    writes = 0;
    auto t0 = std::chrono::steady_clock::now();
    for (int j = 0; j < cells; j++) {
      for (int i = 0; i < cells; i++) {
        int n00 = i + j * N, n10 = n00 + 1, n01 = n00 + N, n11 = n01 + 1;
        const double qx[4] = {nodeX[n00], nodeX[n10], nodeX[n11], nodeX[n01]}, qy[4] = {nodeY[n00], nodeY[n10], nodeY[n11], nodeY[n01]};
        gdwDrawQuad(qx, qy, 0.0f, state, drawFunction);
      }
    }
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    if (ms < best) best = ms;
  }
  printf("BENCH %-28s %5d x %-5d cells -> %dx%d: %8.2f ms  (%.1f Mtriangles/s, %llu pixel writes, checksum %llu)\n", name, cells, cells, W, H, best, 2.0 * cells * cells / best / 1000.0,
         (unsigned long long)writes, (unsigned long long)checksum);
}

TEST(gdwDrawTriangleBenchmark, GenericDataWarper) {
  if (getenv("ADAGUC_BENCH") == nullptr) return;
  benchmarkTriangleMesh("huge grid, subpixel triangles", 1500, 1024, 0.3);
  benchmarkTriangleMesh("huge grid, large destination", 1500, 4096, 0.3);
  benchmarkTriangleMesh("coarse grid, big triangles", 100, 1024, 0.3);
  benchmarkTriangleMesh("very coarse grid, huge tris", 20, 2048, 0.3);
  CHECK(true);
}

// Quad rounds/culls once per quad, triangle once per triangle: must draw identical pixels and weights.
TEST(gdwDrawQuadDrawsTheSamePixelsAsTwoTriangles, GenericDataWarper) {
  const int W = 120, H = 120;
  std::vector<int> writesQuad, writesTriangles;
  int outOfRange = 0;
  drawSkewedMeshAndCountWrites(14, 14, 6.3, 1.7, 0.8, writesQuad, W, H, -9.4, -7.2, outOfRange, false);
  drawSkewedMeshAndCountWrites(14, 14, 6.3, 1.7, 0.8, writesTriangles, W, H, -9.4, -7.2, outOfRange, true);
  CHECK(writesQuad == writesTriangles);
}
