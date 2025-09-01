#include "CDebugger.h"
#include "CGenericDataWarperTools.h"
#include "CppUnitLite/TestHarness.h"
#include "CTString.h"
#include "CImageWarper.h"
#include "CImgRenderFieldVectors.h"
#include "f8vector.h"

// To test this file do in the ./bin folder of adaguc-server:
// cmake --build . --config Debug --target testadagucserver -j 10 -- && ctest --verbose

DEF_ERRORMAIN()

int main() {
  double dfSourceW = 1000;
  double dfSourceExtW = 360;
  double dfSourceOrigX = 0;
  double dfDestOrigX = 0;
  double dfDestExtW = 360;
  double dfDestW = 1000;
  for (double input = -4000; input < 4000; input += 1) {
    double result = roundedLinearTransform(input, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
    if (result != input) {
      CDBError("linearTransform [%f] != [%f] wrong result", input, result);
      throw __LINE__;
    }
  }

  double input, result, expected;

  input = 0.4;
  expected = 0.0;
  result = roundedLinearTransform(input, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
  if (result != expected) {
    CDBError("linearTransform [%f] != [%f] wrong result", result, expected);
    throw __LINE__;
  }
  input = 0.5;
  expected = 1.0;
  result = roundedLinearTransform(input, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
  if (result != expected) {
    CDBError("linearTransform [%f] != [%f] wrong result", result, expected);
    throw __LINE__;
  }
  input = -0.5;
  expected = 0.0;
  result = roundedLinearTransform(input, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
  if (result != expected) {
    CDBError("linearTransform [%f] != [%f] wrong result", result, expected);
    throw __LINE__;
  }
  input = -0.6;
  expected = -1.0;
  result = roundedLinearTransform(input, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
  if (result != expected) {
    CDBError("linearTransform [%f] != [%f] wrong result", result, expected);
    throw __LINE__;
  }

  // CDBDebug("OK %f", linearTransform(5.0, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW));

  TestResult tr;
  TestRegistry::runAllTests(tr);
  if (tr.failureCount != 0) {
    return 1;
  }
  return 0;
}

TEST(alreadyMeter, CImageWarper) {
  CT::string projStringIn("+proj=stere +lat_0=90 +lon_0=0 +lat_ts=60 +a=6378140 +b=6356750 +x_0=0 y_0=0");
  CT::string projStringOut;
  double scaling;
  std::tie(projStringOut, scaling) = CImageWarper::fixProjection(projStringIn);
  CHECK(scaling == 1.0);
  CHECK(projStringOut == projStringIn);
}

TEST(kilometerToMeter, CImageWarper) {
  CT::string projStringIn("+proj=stere +lat_0=90 +lon_0=0 +lat_ts=60 +a=6378.14 +b=6356.75 +x_0=0 y_0=0");
  CT::string projStringOutExpected("+proj=stere +lat_0=90.000000 +lon_0=0.000000 +lat_ts=60.000000 +a=6378140.000000 +b=6356750.000000 +x_0=0.000000 +y_0=0.000000 +ellps=WGS84 +datum=WGS84");
  CT::string projStringOut;
  double scaling;
  std::tie(projStringOut, scaling) = CImageWarper::fixProjection(projStringIn);
  CHECK(scaling == 1000.0);
  CHECK(projStringOut == projStringOutExpected);
}

TEST(kilometerToMeterObTran, CImageWarper) {
  CT::string projStringIn("+proj=ob_tran +o_proj=longlat +lon_0=5.449997 +o_lat_p=37.250000 +o_lon_p=0.000000 +a=6378.14 +b=6356.75 +x_0=0.000000 +y_0=0.000000 +no_defs");
  CT::string projStringOutExpected("+proj=ob_tran +o_proj=longlat +lon_0=5.449997 +o_lat_p=37.250000 +o_lon_p=0.000000 +a=6378140.000000 +b=6356750.000000 +x_0=0.000000 +y_0=0.000000 +no_defs");
  CT::string projStringOut;
  double scaling;
  std::tie(projStringOut, scaling) = CImageWarper::fixProjection(projStringIn);
  CHECK(scaling == 1000.0);
  CHECK(projStringOut == projStringOutExpected);
}

TEST(meterToMeterObTran, CImageWarper) {
  CT::string projStringIn("+proj=ob_tran +o_proj=longlat +lon_0=5.449997 +o_lat_p=37.250000 +o_lon_p=0.000000 +a=6378140.000000 +b=6356750.000000 +x_0=0.000000 +y_0=0.000000 +no_defs");
  CT::string projStringOutExpected("+proj=ob_tran +o_proj=longlat +lon_0=5.449997 +o_lat_p=37.250000 +o_lon_p=0.000000 +a=6378140.000000 +b=6356750.000000 +x_0=0.000000 +y_0=0.000000 +no_defs");
  CT::string projStringOut;
  double scaling;
  std::tie(projStringOut, scaling) = CImageWarper::fixProjection(projStringIn);
  CHECK(scaling == 1);
  CHECK(projStringOut == projStringOutExpected);
}

TEST(radians, CImageWarper) {
  CT::string projStringIn("+proj=geos +lon_0=0.000000 +lat_0=0.000000 +h=1.000000 +a=0.178231 +b=0.177633 +sweep=y");
  CT::string projStringOut;
  double scaling;
  std::tie(projStringOut, scaling) = CImageWarper::fixProjection(projStringIn);
  CHECK(scaling == 1.0);
  CHECK(projStringOut == projStringIn);
}

// From DINI point to latlon
TEST(CImgRenderFieldVectors, jacobianTransformUWCWDini) {
  CT::string crs = "+proj=ob_tran +o_proj=longlat +lon_0=-8.0 +o_lat_p=35.0 +o_lon_p=0.0 +a=6367470 +e=0 +no_defs";

  CGeoParams geo;
  geo.CRS = crs;
  std::vector<CServerConfig::XMLE_Projection *> v;

  // Init warper
  CImageWarper warper;
  int status = warper.initreproj(crs.c_str(), &geo, &v);
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
  // CDBDebug("compGridRel %f %f %f %f", compGridRel.u, compGridRel.v, compGridRel.magnitude(), compGridRel.direction());
  DOUBLES_EQUAL(compGridRel.u, -6.099962, 0.001);
  DOUBLES_EQUAL(compGridRel.v, 4.410759, 0.001);
  DOUBLES_EQUAL(compGridRel.magnitude(), 7.527571, 0.001);
  DOUBLES_EQUAL(compGridRel.direction(), 2.515544, 0.001);

  f8component compNoGridRel = jacobianTransform(speedVector, gridCoordLL, gridCoordUR, &warper, false);
  // CDBDebug("compNoGridRel %f %f %f %f", compNoGridRel.u, compNoGridRel.v, compNoGridRel.magnitude(), compNoGridRel.direction());
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
  CT::string crs = "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs";

  CGeoParams geo;
  geo.CRS = crs;
  std::vector<CServerConfig::XMLE_Projection *> v;

  // Init warper
  CImageWarper warper;
  int status = warper.initreproj(crs.c_str(), &geo, &v);
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
  // CDBDebug("compGridRel %f %f %f %f", compGridRel.u, compGridRel.v, compGridRel.magnitude(), compGridRel.direction());
  DOUBLES_EQUAL(compGridRel.u, -6.224651, 0.001);
  DOUBLES_EQUAL(compGridRel.v, 4.232972, 0.001);
  DOUBLES_EQUAL(compGridRel.magnitude(), 7.527571, 0.001);
  DOUBLES_EQUAL(compGridRel.direction(), 2.544393, 0.001);

  f8component compNoGridRel = jacobianTransform(speedVector, gridCoordLL, gridCoordUR, &warper, false);
  // CDBDebug("compNoGridRel %f %f %f %f", compNoGridRel.u, compNoGridRel.v, compNoGridRel.magnitude(), compNoGridRel.direction());
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
  CT::string crs = "+proj=ob_tran +o_proj=longlat +lon_0=-8.0 +o_lat_p=35.0 +o_lon_p=0.0 +a=6367470 +e=0 +no_defs";

  CGeoParams geo;
  geo.CRS = crs;
  std::vector<CServerConfig::XMLE_Projection *> v;

  // Init warper
  CImageWarper warper;
  warper.initreproj(crs.c_str(), &geo, &v);

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
  CT::string projString = proj4ToCF.convertCFToProj(&var);
  CDBDebug("projString [%s]", projString.c_str());

  CT::string expected = "+proj=aeqd +lat_0=-41.200000 +lon_0=174.700000 +k_0=1.0 +x_0=0.000000 +y_0=0.000000 +a=6378140.000000 +b=6378140.000000 ";
  CHECK(projString == expected);
}