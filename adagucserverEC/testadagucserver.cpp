#include "CDebugger.h"

#include "CppUnitLite/TestHarness.h"
#include "CTString.h"
#include "CImageWarper.h"
#include "CImgRenderFieldVectors.h"
#include "ProjCache.h"
#include "CDrawFunction.h"
#include "CCDFHDF5IO.h"
#include "CDataSource.h"
#include "CProj4ToCF.h"
#include "CServerConfig_CPPXSD.h"
#include "CServerParams.h"
#include "CPGSQLDB.h"
#include "Types/CPointTypes.h"
#include "Types/GeoParameters.h"
#include "GenericDataWarper/gdwDrawTriangle.h"
#include "CImageOperators/smoothRasterField.h"
#include "CUniqueRequests/trajectory.h"
#include <vector>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include "CXMLSerializerInterface.h"

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

TEST(crossedCells, Trajectory) {
  std::vector<i4point> cells;
  traverseTrajectoryCells({0.2, 1.2}, {3.2, 1.2}, 4, 3, cells);
  CHECK((cells == std::vector<i4point>{{0, 1}, {1, 1}, {2, 1}, {3, 1}}));
  cells.clear();
  traverseTrajectoryCells({3.2, 1.2}, {0.2, 1.2}, 4, 3, cells);
  CHECK((cells == std::vector<i4point>{{3, 1}, {2, 1}, {1, 1}, {0, 1}}));
}

TEST(clippingAndCorners, Trajectory) {
  std::vector<i4point> cells;
  traverseTrajectoryCells({-10, 0.5}, {10, 0.5}, 3, 3, cells);
  CHECK((cells == std::vector<i4point>{{0, 0}, {1, 0}, {2, 0}}));
  cells.clear();
  traverseTrajectoryCells({0.2, 0.2}, {2.8, 2.8}, 3, 3, cells);
  CHECK((cells == std::vector<i4point>{{0, 0}, {1, 1}, {2, 2}}));
  cells.clear();
  traverseTrajectoryCells({-5, -5}, {-1, -1}, 3, 3, cells);
  CHECK(cells.empty());
  traverseTrajectoryCells({1.2, 1.2}, {1.2, 1.2}, 3, 3, cells);
  CHECK((cells == std::vector<i4point>{{1, 1}}));
}

TEST(boundariesAndRevisits, Trajectory) {
  std::vector<i4point> cells;
  traverseTrajectoryCells({0.2, 1}, {2.8, 1}, 3, 3, cells);
  traverseTrajectoryCells({2.8, 1}, {0.2, 1}, 3, 3, cells);
  CHECK((cells == std::vector<i4point>{{0, 1}, {1, 1}, {2, 1}, {1, 1}, {0, 1}}));
}

TEST(subdivisionAtCorners, Trajectory) {
  std::vector<i4point> whole, subdivided;
  traverseTrajectoryCells({0.2, 2.8}, {2.8, 0.2}, 3, 3, whole);
  traverseTrajectoryCells({0.2, 2.8}, {1, 2}, 3, 3, subdivided);
  traverseTrajectoryCells({1, 2}, {2, 1}, 3, 3, subdivided);
  traverseTrajectoryCells({2, 1}, {2.8, 0.2}, 3, 3, subdivided);
  CHECK((whole == std::vector<i4point>{{0, 2}, {1, 1}, {2, 0}}));
  CHECK(whole == subdivided);
}

TEST(longitudeSeam, Trajectory) {
  std::vector<i4point> cells;
  traverseTrajectoryCells({-1.5, 1.5}, {2.5, 1.5}, 360, 3, cells, 360);
  CHECK((cells == std::vector<i4point>{{358, 1}, {359, 1}, {0, 1}, {1, 1}, {2, 1}}));
  cells.clear();
  traverseTrajectoryCells({2.5, 1.5}, {-1.5, 1.5}, 360, 3, cells, 360);
  CHECK((cells == std::vector<i4point>{{2, 1}, {1, 1}, {0, 1}, {359, 1}, {358, 1}}));
}

TEST(float32GridSpacing, Trajectory) {
  for (const auto &[origin, count, spacing]: std::vector<std::tuple<double, size_t, double>>{{-13.5, 676, 0.05}, {-13.6, 564, 0.05}, {14.55, 564, -0.05}, {50, 1001, 0.01}, {65, 401, 0.025}}) {
    std::vector<double> values(count);
    for (size_t i = 0; i < count; ++i) values[i] = float(origin + i * spacing);
    const auto [first, step] = regularTrajectoryAxis(values.data(), values.size(), true);
    CHECK(first == values.front());
    CHECK(std::abs(step - spacing) < 1e-8);
    values[count / 2] += 0.01;
    bool rejected = false;
    try {
      regularTrajectoryAxis(values.data(), values.size(), true);
    } catch (int) {
      rejected = true;
    }
    CHECK(rejected);
  }
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

  f8component compGridRel = jacobianTransform(speedVector, gridCoordLL, gridCoordUR.y - gridCoordLL.y, &warper, true);
  DOUBLES_EQUAL(compGridRel.u, -6.099962, 0.001);
  DOUBLES_EQUAL(compGridRel.v, 4.410759, 0.001);
  DOUBLES_EQUAL(compGridRel.magnitude(), 7.527571, 0.001);
  DOUBLES_EQUAL(compGridRel.direction(), 2.515544, 0.001);

  // A vector that is not grid relative is returned as is
  f8component compNoGridRel = jacobianTransform(speedVector, gridCoordLL, gridCoordUR.y - gridCoordLL.y, &warper, false);
  DOUBLES_EQUAL(speedVector.u, compNoGridRel.u, 0);
  DOUBLES_EQUAL(speedVector.v, compNoGridRel.v, 0);

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

  // On a lat/lon grid, grid north is true north, so a grid-relative vector must come back unrotated.
  // The tolerance only allows for rounding: measuring the angle along the chord instead of in the tangent plane was off by 1.8e-3 here.
  f8component compGridRel = jacobianTransform(speedVector, gridCoordLL, gridCoordUR.y - gridCoordLL.y, &warper, true);
  DOUBLES_EQUAL(speedVector.u, compGridRel.u, 1e-6);
  DOUBLES_EQUAL(speedVector.v, compGridRel.v, 1e-6);
  DOUBLES_EQUAL(speedVector.direction(), compGridRel.direction(), 1e-6);

  // A vector that is not grid relative is returned as is
  f8component compNoGridRel = jacobianTransform(speedVector, gridCoordLL, gridCoordUR.y - gridCoordLL.y, &warper, false);
  DOUBLES_EQUAL(speedVector.u, compNoGridRel.u, 0);
  DOUBLES_EQUAL(speedVector.v, compNoGridRel.v, 0);

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

  f8component compGridRel = jacobianTransform(speedVector, gridCoordLL, gridCoordUR.y - gridCoordLL.y, &warper, true);
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

TEST(CStyleConfiguration, negativeAlphaMeansOverwriteNotClampedBlend) {
  // Negative alpha (e.g. -1) -> magnitude -(alpha+1), marked overwrite, not blend.
  CServerConfig::XMLE_Legend legend;
  legend.attr.name = "testNegativeAlpha";
  legend.attr.type = "colorRange";
  CServerConfig::XMLE_palette p0, p240;
  p0.attr.index = 0;
  p0.attr.red = 0;
  p0.attr.green = 0;
  p0.attr.blue = 0;
  p0.attr.alpha = -1;
  p240.attr.index = 240;
  p240.attr.red = 0;
  p240.attr.green = 0;
  p240.attr.blue = 0;
  p240.attr.alpha = -1;
  legend.palette.push_back(p0);
  legend.palette.push_back(p240);

  CStyleConfiguration styleConfiguration;
  styleConfiguration.createPalette(&legend);

  CColor resolved = styleConfiguration.getCColorFromLegendPaletteIndex(100);
  CHECK(resolved.c_str() == CColor(1, 0, 0, 0).c_str()); // red bumped 0->1, avoids auto-transparency
  CHECK(styleConfiguration.isLegendPaletteIndexOverwrite(100) == true);
}

TEST(CStyleConfiguration, positiveAlphaIsNotOverwrite) {
  CServerConfig::XMLE_Legend legend;
  legend.attr.name = "testPositiveAlpha";
  legend.attr.type = "colorRange";
  CServerConfig::XMLE_palette p0, p240;
  p0.attr.index = 0;
  p0.attr.red = 10;
  p0.attr.green = 20;
  p0.attr.blue = 30;
  p0.attr.alpha = 128;
  p240.attr.index = 240;
  p240.attr.red = 10;
  p240.attr.green = 20;
  p240.attr.blue = 30;
  p240.attr.alpha = 128;
  legend.palette.push_back(p0);
  legend.palette.push_back(p240);

  CStyleConfiguration styleConfiguration;
  styleConfiguration.createPalette(&legend);

  CColor resolved = styleConfiguration.getCColorFromLegendPaletteIndex(100);
  CHECK(resolved.c_str() == CColor(10, 20, 30, 128).c_str());
  CHECK(styleConfiguration.isLegendPaletteIndexOverwrite(100) == false);
}

TEST(CStyleConfiguration, createPaletteInterpolatesColorRangeLinearly) {
  CServerConfig::XMLE_Legend legend;
  legend.attr.name = "testColorRangeInterpolation";
  legend.attr.type = "colorRange";
  CServerConfig::XMLE_palette p0, p240;
  p0.attr.index = 0;
  p0.attr.red = 0;
  p0.attr.green = 0;
  p0.attr.blue = 0;
  p0.attr.alpha = 255;
  p240.attr.index = 240;
  p240.attr.red = 240;
  p240.attr.green = 240;
  p240.attr.blue = 240;
  p240.attr.alpha = 255;
  legend.palette.push_back(p0);
  legend.palette.push_back(p240);

  CStyleConfiguration styleConfiguration;
  styleConfiguration.createPalette(&legend);

  CColor start = styleConfiguration.getCColorFromLegendPaletteIndex(0);
  CHECK(start.c_str() == CColor(1, 0, 0, 255).c_str()); // red bumped 0->1

  CColor midpoint = styleConfiguration.getCColorFromLegendPaletteIndex(120);
  CHECK(midpoint.c_str() == CColor(120, 120, 120, 255).c_str());

  CColor end = styleConfiguration.getCColorFromLegendPaletteIndex(239);
  CHECK(end.c_str() == CColor(239, 239, 239, 255).c_str());
}

TEST(CStyleConfiguration, getCColorFromLegendPaletteIndexOutOfRangeIsTransparent) {
  // renderContinuousLegend's ramp fill hits a genuine negative index on its last row.
  CStyleConfiguration styleConfiguration;
  styleConfiguration.createPalette(nullptr);

  CHECK(styleConfiguration.getCColorFromLegendPaletteIndex(-1).a == 0);
  CHECK(styleConfiguration.getCColorFromLegendPaletteIndex(256).a == 0);

  CStyleConfiguration noPalette; // no palette built at all behaves the same way
  CHECK(noPalette.getCColorFromLegendPaletteIndex(0).a == 0);
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

// Smooths a width x height grid with radius r and returns the smoothed value of every cell.
static std::vector<float> smoothGrid(std::vector<float> grid, int width, int height, int r, double nodataValue = -9999) {
  GDWState state = {};
  state.sourceGridWidth = width;
  state.sourceGridHeight = height;
  GDWDrawFunctionSettings settings{};
  settings.smoothingFiter = r;
  settings.hasNodataValue = true;
  settings.dfNodataValue = nodataValue;
  std::vector<float> result(grid.size());
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      result[x + y * width] = smoothingAtLocation<float>(x, y, grid.data(), state, settings);
    }
  }
  return result;
}

TEST(smoothRasterField, averagesWindowClippedAtEdges) {
  // 1 2 3 / 4 5 6 / 7 8 9 with radius 1: corners average 4 cells, edges 6 and the center 9.
  std::vector<float> expected = {3, 3.5, 4, 4.5, 5, 5.5, 6, 6.5, 7};
  CHECK(smoothGrid({1, 2, 3, 4, 5, 6, 7, 8, 9}, 3, 3, 1) == expected);
}

TEST(smoothRasterField, skipsNodataAndNaN) {
  std::vector<float> expected = {1, 1, 3, 4, 4};
  CHECK(smoothGrid({1, NAN, -9999, 3, 5}, 5, 1, 1) == expected);
}

TEST(smoothRasterField, nodataWhenWindowHasNoValues) {
  std::vector<float> expected = {-9999, -9999, -9999, 7, 7};
  CHECK(smoothGrid({-9999, -9999, -9999, -9999, 7}, 5, 1, 1) == expected);
}

TEST(smoothRasterField, radiusLargerThanGridAveragesEverything) {
  std::vector<float> expected(9, 5);
  CHECK(smoothGrid({1, 2, 3, 4, 5, 6, 7, 8, 9}, 3, 3, 40) == expected);
}

TEST(smoothRasterField, linearFieldUnchangedInInterior) {
  // Away from the edges, smoothing a linear field gives the field itself. Integer values keep the sums exact.
  const int width = 300, height = 200, r = 12;
  std::vector<float> grid(width * height);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      grid[x + y * width] = x + 1000 * y;
    }
  }
  auto smoothed = smoothGrid(grid, width, height, r);
  int differences = 0;
  for (int y = r; y < height - r; y++) {
    for (int x = r; x < width - r; x++) {
      if (smoothed[x + y * width] != grid[x + y * width]) differences++;
    }
  }
  CHECK(differences == 0);
}

TEST(smoothRasterField, infAndHugeValuesOnlyAffectTheirOwnWindow) {
  // Inf is skipped like nodata, and a huge undeclared fill value only changes the cells whose window contains it.
  std::vector<float> grid(200, 1);
  grid[10] = INFINITY;
  grid[100] = 9.96921e36f;
  auto smoothed = smoothGrid(grid, 200, 1, 2);
  int wrongOutsideWindows = 0;
  for (int x = 0; x < 200; x++) {
    bool nearHuge = x >= 98 && x <= 102;
    if (!nearHuge && smoothed[x] != 1) wrongOutsideWindows++;
  }
  CHECK(wrongOutsideWindows == 0);
}

TEST(smoothRasterField, matchesBruteForceAcrossTileBorders) {
  // Grid spans several 64x64 tiles, does not divide evenly, and contains nodata.
  const int width = 150, height = 97, r = 3;
  const float nodata = -9999;
  std::vector<float> grid(width * height);
  for (int i = 0; i < width * height; i++) {
    grid[i] = (i * 7919) % 13 == 0 ? nodata : (float)((i * 31) % 101) / 7.f;
  }
  auto smoothed = smoothGrid(grid, width, height, r, nodata);
  int mismatches = 0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      double sum = 0;
      int count = 0;
      for (int sy = std::max(0, y - r); sy <= std::min(height - 1, y + r); sy++) {
        for (int sx = std::max(0, x - r); sx <= std::min(width - 1, x + r); sx++) {
          float v = grid[sx + sy * width];
          if (v == nodata) continue;
          sum += v;
          count++;
        }
      }
      float expected = count > 0 ? (float)(sum / count) : nodata;
      if (std::fabs(smoothed[x + y * width] - expected) > 1e-4) mismatches++;
    }
  }
  CHECK(mismatches == 0);
}

TEST(CT, isSafeFloatFormat) {
  for (const char *safe: {"", " ", "%f", "%2.2f", "%.1f", "%0.0f", "%+08.3e", "%g", "%G", "%a", "%.1f degC", "100%%", "%% %5.1f %%"}) {
    CHECK(CT::isSafeFloatFormat(safe));
  }
  for (const char *unsafe: {"%s", "%p", "%n", "%d", "%x", "%c", "%*f", "%.*f", "%1$f", "%Lf", "%lf", "%f %f", "%123f", "%.123f", "%", "abc%"}) {
    CHECK(!CT::isSafeFloatFormat(unsafe));
  }
}

TEST(CT, formatFloatFallsBackForUnsafeFormats) {
  CHECK(CT::formatFloat("%.1f", 1.25) == "1.2");
  CHECK(CT::formatFloat("%.1f degC", 21.04) == "21.0 degC");
  CHECK(CT::formatFloat("%+08.3f", 1.5) == "+001.500");
  CHECK(CT::formatFloat("%e", 1500) == "1.500000e+03");
  CHECK(CT::formatFloat("100%%", 1.5) == "100%");
  CHECK(CT::formatFloat(" ", 1.5) == " ");
  CHECK(CT::formatFloat("", 1.5) == "");
  CHECK(CT::formatFloat("%s", 1.5) == "1.5");
  CHECK(CT::formatFloat("%p %p", 2) == "2");
}

TEST(CStyleConfiguration, paletteLoopIsBoundedToPaletteIndices) {
  // Indices far outside 0-239 used to loop over every index in between (billions of iterations)
  CServerConfig::XMLE_Legend legend;
  legend.attr.name = "testHugeIndexRange";
  legend.attr.type = "colorRange";
  CServerConfig::XMLE_palette pFrom, pTo;
  pFrom.attr.index = -2000000000;
  pFrom.attr.red = 0;
  pFrom.attr.green = 0;
  pFrom.attr.blue = 0;
  pFrom.attr.alpha = 255;
  pTo.attr.index = 2000000000;
  pTo.attr.red = 255;
  pTo.attr.green = 255;
  pTo.attr.blue = 255;
  pTo.attr.alpha = 255;
  legend.palette.push_back(pFrom);
  legend.palette.push_back(pTo);

  auto start = std::chrono::steady_clock::now();
  CStyleConfiguration styleConfiguration;
  styleConfiguration.createPalette(&legend);
  CHECK(std::chrono::steady_clock::now() - start < std::chrono::seconds(1));
  CHECK(styleConfiguration.getCColorFromLegendPaletteIndex(120).a == 255);
}

TEST(CColor, isValid) {
  CHECK(CColor::isValid("#00FF00"));
  CHECK(CColor::isValid("#00ff00AA"));
  CHECK(!CColor::isValid("red"));
  CHECK(!CColor::isValid("#00FF0"));
  CHECK(!CColor::isValid("#00FF00A"));
  CHECK(!CColor::isValid("#GGHHII"));
  CHECK(!CColor::isValid("00FF00AA"));
  CHECK(!CColor::isValid(""));
}

TEST(CColor, parse) {
  CHECK(CColor("#00ff80").c_str() == "#00FF80FF");
  CHECK(CColor("#00FF8040").c_str() == "#00FF8040");
  // Invalid colors are opaque black, also when they have the right length
  CHECK(CColor("#GGHHII").c_str() == "#000000FF");
  CHECK(CColor("red").c_str() == "#000000FF");
}

// Applies a STYLESHEET as CRequest does, to a configuration with the style testdata_style_manycontours and the legend rainbow
static int applyStyleSheet(const std::string &styleSheet) {
  CServerConfig config;
  parseConfig(&config,
              R"(<Configuration>
                   <Legend name="rainbow" type="colorRange"><palette index="0" red="0" green="0" blue="0"/><palette index="240" red="255" green="255" blue="255"/></Legend>
                   <Style name="testdata_style_manycontours"><Legend>rainbow</Legend><ContourLine classes="0.1"/></Style>
                 </Configuration>)",
              "test");
  CServerParams srvParams;
  srvParams.cfg = &config.Configuration[0];
  return srvParams.applyStyleSheet(styleSheet);
}

// Returns the stylesheets for which applyStyleSheet does not give the expected result, one per line
static std::string unexpectedStyleSheets(const std::vector<std::string> &styleSheets, bool expectAllowed) {
  std::string unexpected;
  for (const auto &styleSheet: styleSheets) {
    if ((applyStyleSheet(styleSheet) == 0) != expectAllowed) unexpected += (expectAllowed ? "\nrefused: " : "\nallowed: ") + styleSheet.substr(0, 200);
  }
  return unexpected;
}

#define CHECK_STYLESHEETS(styleSheets, expectAllowed)                                                                                                                              \
  {                                                                                                                                                                                \
    std::string unexpected = unexpectedStyleSheets(styleSheets, expectAllowed);                                                                                                    \
    if (!unexpected.empty()) FAIL(unexpected.c_str());                                                                                                                             \
  }

static std::string repeat(const std::string &text, int times) {
  std::string result;
  for (int i = 0; i < times; i++) result += text;
  return result;
}

TEST(CServerParams, applyStyleSheetRefusesDisallowedContent) {
  CHECK_STYLESHEETS(
      (std::vector<std::string>{
          // Only Style and Legend elements may be posted, and only one Style
          R"(<Layer type="database"><FilePath>/etc</FilePath><Variable>x</Variable></Layer>)",
          R"(<Style name="a"/><Style name="b"/>)",
          R"(<Style name=)",
          // File access and other layers
          R"(<Style name="s"><DataPostProc algorithm="addfeatures" a="/etc/passwd"/></Style>)",
          R"(<Style name="s"><DataPostProc algorithm="include_layer" name="testdata"/></Style>)",
          R"(<Style name="s"><DataPostProc algorithm="pointsfromgrid" name="x"/></Style>)",
          R"(<Style name="s"><DataPostProc algorithm="convert_units" name="x"/></Style>)",
          R"(<Style name="s"><Point fontfile="/etc/passwd"/></Style>)",
          R"(<Style name="s"><Point symbol="/etc/passwd"/></Style>)",
          R"(<Style name="s"><Point file="{ADAGUC_PATH}data/symbols/marker.png"/></Style>)",
          R"(<Style name="s"><FeatureInterval labelfontfile="/etc/passwd"/></Style>)",
          R"(<Legend name="l" type="svg" file="/etc/passwd"/>)",
          // Applied to strings and integers too, so no safe format exists
          R"(<Style name="s"><FeatureInterval labelpropertyformat="%s"/></Style>)",
          // Posted printf formats are accepted, but their length is limited
          R"(<Style name="s"><ContourLine textformatting=")" + repeat("%f", 33) + R"("/></Style>)",
          // IncludeStyle of a style that is not configured, like the posted style itself
          R"(<Style name="s"><IncludeStyle name="s"/></Style>)",
          R"(<Style name="s"><IncludeStyle name="does_not_exist"/></Style>)",
          // Unknown elements, attributes and markup
          R"(<Style name="s"><Foo/></Style>)",
          R"(<Style name="s"><ContourLine foo="1"/></Style>)",
          R"(<Style name="s"><?php echo 1; ?></Style>)",
          R"(<Configuration foo="1"><Style name="s"/></Configuration>)",
          R"(<?xml version="1.0"?><!DOCTYPE Configuration [<!ENTITY x "y">]><Configuration><Style name="&x;"/></Configuration>)",
          // Regex with back references
          R"(<Style name="s"><FeatureInterval match="\(a*\)*\1"/></Style>)",
          // Values that make loops endless or huge
          R"(<Style name="s"><Stippling distancex="-5"/></Style>)",
          R"(<Style name="s"><SmoothingFilter>1000</SmoothingFilter></Style>)",
          R"(<Style name="s"><RenderSettings striding="0"/></Style>)",
          R"(<Style name="s"><ContourLine interval="0"/></Style>)",
          R"(<Style name="s"><ContourLine interval="-1"/></Style>)",
          R"(<Style name="s"><ContourLine width="1e9"/></Style>)",
          R"(<Legend name="l" type="colorRange"><palette index="2000000000" red="0" green="0" blue="0"/></Legend>)",
          R"(<Legend name="l" type="interval"><palette min="0" max="2000000000" red="0" green="0" blue="0"/></Legend>)",
          // Every DataPostProc goes over the whole grid, and add_dataobject allocates a grid
          R"(<Style name="s">)" + repeat(R"(<DataPostProc algorithm="add_dataobject" name="m" a="1"/>)", 9) + "</Style>",
          R"(<Style name="s"><DataPostProc algorithm="add_dataobject" name="m" a="inf"/></Style>)",
          // Values read with atoi must be whole numbers: 20000e-3 is 20 as a number, but atoi reads 20000
          R"(<Style name="s"><SmoothingFilter>20000e-3</SmoothingFilter></Style>)",
          R"(<Style name="s"><Stippling distancex="100000e-3"/></Style>)",
          R"(<Style name="s"><RenderSettings striding="0.5"/></Style>)",
          R"(<Legend name="l" type="colorRange"><palette index="100000e-3" red="0" green="0" blue="0"/></Legend>)",
          // Malformed values
          R"(<Style name="s"><ContourLine linecolor="red"/></Style>)",
          R"(<Style name="s"><Min>abc</Min></Style>)",
          R"(<Style name="s"><Min>nan</Min></Style>)",
          R"(<Style name="s"><Max>inf</Max></Style>)",
          R"(<Style name="s"><ContourLine classes="0.1,abc"/></Style>)",
          R"(<Style name=")" + std::string(2000, 'x') + R"("/>)",
          R"(<Style name="s"><ShadeInterval>inf</ShadeInterval></Style>)",
          R"(<Style name="s"><ShadeInterval>-1</ShadeInterval></Style>)",
          R"(<Style name="s"><Log>-10</Log></Style>)",
          // drawContour has one bit per ContourLine
          R"(<Style name="s">)" + repeat(R"(<ContourLine interval="0.1"/>)", 33) + "</Style>",
          // A legend that is neither configured nor posted
          R"(<Style name="s"><Legend>does_not_exist</Legend></Style>)",
          // Only the text before a comment is read as value, so it must be the text that is checked
          R"(<Style name="s"><SmoothingFilter>100000<!---->e-4</SmoothingFilter></Style>)",
          R"(<Style name="s"><ContourIntervalL>-1<!---->e-999</ContourIntervalL></Style>)",
          // Every IncludeStyle adds the included elements again
          R"(<Style name="s">)" + repeat(R"(<IncludeStyle name="testdata_style_manycontours"/>)", 5) + "</Style>",
      }),
      false);
}

TEST(CServerParams, applyStyleSheetAcceptsAllowedContent) {
  CHECK_STYLESHEETS(
      (std::vector<std::string>{
          R"(<Style name="s"><Legend>rainbow</Legend><Min>0</Min><Max>0.5</Max></Style>)",
          R"(<?xml version="1.0"?><Configuration><Style name="s"/></Configuration>)",
          // Posted legends may be used by the posted style, and replace a configured legend with the same name
          R"(<Legend name="l" type="colorRange"><palette index="0" red="0" green="0" blue="0"/></Legend><Style name="s"><Legend>l</Legend></Style>)",
          R"(<Legend name="rainbow" type="colorRange"><palette index="0" red="0" green="0" blue="0"/></Legend>)",
          // A configured style may be included, also by a posted style with the same name
          R"(<Style name="testdata_style_manycontours"><IncludeStyle name="testdata_style_manycontours"/></Style>)",
          // The limits themselves
          R"(<Style name="s">)" + repeat(R"(<ContourLine interval="0.1"/>)", 32) + "</Style>",
          R"(<Style name="s">)" + repeat(R"(<DataPostProc algorithm="add_dataobject" name="m" a="1"/>)", 8) + "</Style>",
          R"(<Style name="s">)" + repeat(R"(<IncludeStyle name="testdata_style_manycontours"/>)", 4) + "</Style>",
          R"(<Style name="s"><ContourLine textformatting=")" + repeat("%f", 32) + R"("/></Style>)",
          // Values read with atoi accept whole numbers with an optional sign and surrounding whitespace
          R"(<Style name="s"><SmoothingFilter> 20 </SmoothingFilter></Style>)",
          R"(<Style name="s"><Stippling distancex="+10" distancey="10" discradius="2"/></Style>)",
          R"(<Legend name="l" type="colorRange"><palette index="0" red="0" green="0" blue="0" alpha="-1"/></Legend>)",
      }),
      true);
}

TEST(CServerParams, applyStyleSheetOnlyAllowsPngFilesInSymbolDirectories) {
  // Symbol directories are {ADAGUC_PATH}data/symbols, {ADAGUC_PATH}data/resources and {ADAGUC_DATASET_DIR}, including subdirectories
  namespace fs = std::filesystem;
  fs::path root = fs::temp_directory_path() / ("testadagucserver_symbols_" + std::to_string(getpid()));
  fs::remove_all(root);
  for (const char *dir: {"adaguc/data/symbols", "adaguc/data/resources/icons", "datasets/sub"}) fs::create_directories(root / dir);
  for (const char *file: {"adaguc/data/symbols/marker.png", "adaguc/data/resources/icons/icon.png", "adaguc/data/resources/locations.geojson", "datasets/sub/symbol.png",
                          "datasets/symbol.png", "datasets/dataset.xml", "outside.png"}) {
    std::ofstream(root / file) << "x";
  }
  fs::create_symlink(root / "outside.png", root / "adaguc/data/symbols/link.png");

  std::string adagucPath = getenv("ADAGUC_PATH") ? getenv("ADAGUC_PATH") : "", datasetDir = getenv("ADAGUC_DATASET_DIR") ? getenv("ADAGUC_DATASET_DIR") : "";
  setenv("ADAGUC_PATH", (root / "adaguc").c_str(), 1);
  setenv("ADAGUC_DATASET_DIR", (root / "datasets").c_str(), 1);

  auto withFile = [](const std::vector<std::string> &files) {
    std::vector<std::string> styleSheets;
    for (const auto &file: files) {
      styleSheets.push_back(R"(<Style name="s"><SymbolInterval file=")" + file + R"("/><Point pointstyle="symbol"/></Style>)");
      styleSheets.push_back(R"(<Style name="s"><LegendGraphic value=")" + file + R"("/></Style>)");
    }
    return styleSheets;
  };
  std::string refused = unexpectedStyleSheets(withFile({"{ADAGUC_PATH}data/symbols/marker.png", "{ADAGUC_PATH}data/resources/icons/icon.png",
                                                        "{ADAGUC_DATASET_DIR}/sub/symbol.png", "{ADAGUC_DATASET_DIR}/symbol.png"}),
                                              true);
  std::string allowed = unexpectedStyleSheets(withFile({"/etc/passwd", "/", "{ADAGUC_PATH}data/symbols/../../../../../etc/passwd", "{ADAGUC_PATH}data/symbols/does_not_exist.png",
                                                        "{ADAGUC_PATH}data/resources/locations.geojson", "{ADAGUC_PATH}data/symbols/../../outside.png",
                                                        "{ADAGUC_PATH}data/symbols/link.png", "{ADAGUC_DATASET_DIR}/dataset.xml", "{ADAGUC_DATASET_DIR}/../outside.png"}),
                                              false);

  adagucPath.empty() ? unsetenv("ADAGUC_PATH") : setenv("ADAGUC_PATH", adagucPath.c_str(), 1);
  datasetDir.empty() ? unsetenv("ADAGUC_DATASET_DIR") : setenv("ADAGUC_DATASET_DIR", datasetDir.c_str(), 1);
  fs::remove_all(root);
  if (!(refused + allowed).empty()) FAIL((refused + allowed).c_str());
}

// A base64 encoded STYLESHEET is checked like the same stylesheet in XML
TEST(CServerParams, applyStyleSheetBase64) {
  CHECK_STYLESHEETS((std::vector<std::string>{
                        // <Style name="s"><Legend>rainbow</Legend><Min>0</Min><Max>0.5</Max></Style>
                        "PFN0eWxlIG5hbWU9InMiPjxMZWdlbmQ+cmFpbmJvdzwvTGVnZW5kPjxNaW4+MDwvTWluPjxNYXg+MC41PC9NYXg+PC9TdHlsZT4=",
                        // <?xml version="1.0"?><Configuration><Style name="s"/></Configuration>
                        "PD94bWwgdmVyc2lvbj0iMS4wIj8+PENvbmZpZ3VyYXRpb24+PFN0eWxlIG5hbWU9InMiLz48L0NvbmZpZ3VyYXRpb24+",
                        // The first one URL safe without padding, and with spaces for the '+' characters a client did not url encode
                        "PFN0eWxlIG5hbWU9InMiPjxMZWdlbmQ-cmFpbmJvdzwvTGVnZW5kPjxNaW4-MDwvTWluPjxNYXg-MC41PC9NYXg-PC9TdHlsZT4",
                        "PFN0eWxlIG5hbWU9InMiPjxMZWdlbmQ cmFpbmJvdzwvTGVnZW5kPjxNaW4 MDwvTWluPjxNYXg MC41PC9NYXg PC9TdHlsZT4=",
                    }),
                    true);
  CHECK_STYLESHEETS((std::vector<std::string>{
                        // <Layer type="database"><FilePath>/etc</FilePath><Variable>x</Variable></Layer>
                        "PExheWVyIHR5cGU9ImRhdGFiYXNlIj48RmlsZVBhdGg+L2V0YzwvRmlsZVBhdGg+PFZhcmlhYmxlPng8L1ZhcmlhYmxlPjwvTGF5ZXI+",
                        // <Style name="s"><Point fontfile="/etc/passwd"/></Style>
                        "PFN0eWxlIG5hbWU9InMiPjxQb2ludCBmb250ZmlsZT0iL2V0Yy9wYXNzd2QiLz48L1N0eWxlPg==",
                        // Not base64, and base64 that is no XML
                        "Style name=s",
                        "bm90IHhtbA==",
                    }),
                    false);
}

