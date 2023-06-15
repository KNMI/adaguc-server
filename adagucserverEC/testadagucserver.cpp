#include "CDebugger.h"
#include "CGenericDataWarperTools.h"
#include "CppUnitLite/TestHarness.h"
#include "CTString.h"
#include "CImageWarper.h"

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
