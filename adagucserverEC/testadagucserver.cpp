#include "CDebugger.h"
#include "CGenericDataWarperTools.h"
#include "CppUnitLite/TestHarness.h"
#include "CTString.h"
#include "CImageWarper.h"
#include "CImgRenderFieldVectors.h"

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

TEST(CImgRenderFieldVectors, jacobianTransform) {
  CT::string crs = "+proj=ob_tran +o_proj=longlat +lon_0=-8.0 +o_lat_p=35.0 +o_lon_p=0.0 +a=6367470 +e=0 +no_defs";

  CGeoParams geo;
  geo.CRS = crs;
  std::vector<CServerConfig::XMLE_Projection *> v;

  // Init warper
  CImageWarper warper;
  int status = warper.initreproj(crs.c_str(), &geo, &v);
  CHECK(status == 0);

  // Make speed vector
  f8component speedVector = {.u = -6.222803, .v = 4.235688};
  DOUBLES_EQUAL(speedVector.magnitude(), 7.527, 0.001);
  DOUBLES_EQUAL(speedVector.angledeg(), 124.242, 0.001);

  // Grid cell to do the calculation for
  f8point gridCoordUL = {.x = 1.1, .y = 1.4};
  f8point gridCoordLR = {.x = 1.15, .y = 1.45};

  // Check if projection transformation from model to latlon works:
  f8point gridCoordULtoLatLon = gridCoordUL;
  warper.reprojModelToLatLon(gridCoordULtoLatLon);
  DOUBLES_EQUAL(gridCoordULtoLatLon.x, -6.013390, 0.001);
  DOUBLES_EQUAL(gridCoordULtoLatLon.y, 56.384378, 0.001);

  f8point gridCoordLRtoLatLon = gridCoordLR;
  warper.reprojModelToLatLon(gridCoordLRtoLatLon);
  DOUBLES_EQUAL(gridCoordLRtoLatLon.x, -5.920456, 0.001);
  DOUBLES_EQUAL(gridCoordLRtoLatLon.y, 56.432904, 0.001);

  f8component compGridRel = jacobianTransform(speedVector, gridCoordUL, gridCoordLR, &warper, true);
  CDBDebug("compGridRel %f %f %f %f", compGridRel.u, compGridRel.v, compGridRel.magnitude(), compGridRel.direction());
  DOUBLES_EQUAL(compGridRel.u, -6.099962, 0.001);
  DOUBLES_EQUAL(compGridRel.v, 4.410759, 0.001);
  DOUBLES_EQUAL(compGridRel.magnitude(), 7.527571, 0.001);
  DOUBLES_EQUAL(compGridRel.direction(), 2.515544, 0.001);

  f8component compNoGridRel = jacobianTransform(speedVector, gridCoordUL, gridCoordLR, &warper, false);
  CDBDebug("compNoGridRel %f %f %f %f", compNoGridRel.u, compNoGridRel.v, compNoGridRel.magnitude(), compNoGridRel.direction());
  DOUBLES_EQUAL(compNoGridRel.u, -6.222803, 0.001);
  DOUBLES_EQUAL(compNoGridRel.v, 4.235688, 0.001);
  DOUBLES_EQUAL(compNoGridRel.magnitude(), 7.527571, 0.001);
  DOUBLES_EQUAL(compNoGridRel.direction(), 2.543957, 0.001);

  // Magnitudes should be the same
  DOUBLES_EQUAL(compNoGridRel.magnitude(), compGridRel.magnitude(), 0.001);
  DOUBLES_EQUAL(speedVector.magnitude(), compGridRel.magnitude(), 0.001);
}
