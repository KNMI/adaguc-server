#include <CImgRenderFieldVectors.h>
#include "CImgWarpBilinear.h"
#include "f8vector.h"

bool verboseLog = true;

f8component jacobianTransform(f8component speedVector, f8point gridCoordLL, f8point gridCoordUR, CImageWarper *warper, bool gridRelative) {
  if (gridRelative) {
    f8point pnt0 = {.x = gridCoordLL.x, .y = gridCoordLL.y};
    warper->reprojModelToLatLon(pnt0);
    f8point pntRad0 = pnt0.rad();
    f8point pntN = {.x = gridCoordLL.x, .y = gridCoordUR.y};
    warper->reprojModelToLatLon(pntN);
    f8point pntRadN = pntN.rad();

    // f8point pntE = {.x = _lon_pntEast, .y = _lat_pntEast};
    // warper->reprojModelToLatLon(pntE);
    // f8point pntRadE = pntE.rad();

    // (lon_pntNorth, lat_pntNorth)
    //     ^
    //     |       (lon_pntCenter, lat_pntCenter)   center of the cell-diagonal
    //     |
    // (lon_pnt0,lat_pnt0) ----> (lon_pntEast,lat_pntEast)
    // This is the local coordinate system of a grid cell where we have (u,v) at location (xpnt0,ypnt0).
    // The local coordinate system is now centered around (lon_pnt0,lat_pnt0)
    // The vector towards north pole at this location will be (0,1,0)
    // The tangent plane at this location is XY wil a normal (0, 0, 1)
    // Nummerical approach using projection onto a unit sphere

    f8vector pntNSph = {.x = cos(pntRadN.y) * cos(pntRadN.x), .y = cos(pntRadN.y) * sin(pntRadN.x), .z = sin(pntRadN.y)};
    f8vector pnt0Sph = {.x = cos(pntRad0.y) * cos(pntRad0.x), .y = cos(pntRad0.y) * sin(pntRad0.x), .z = sin(pntRad0.y)};
    f8vector pntNSphNorm = (pntNSph - pnt0Sph).norm();
    f8vector normSph = pnt0Sph.norm();
    f8vector pntNSphRot = {
        .x = -normSph.z * normSph.x,
        .y = -normSph.z * normSph.y,
        .z = 1.0 - normSph.z * normSph.z,
    };
    f8vector pntNSphRotNorm = pntNSphRot.norm();

    double vectorAngle = acos(dot(pntNSphNorm, pntNSphRotNorm));

    // Determine the sign of the angle
    if (dot(cross(pntNSphRotNorm, pntNSphNorm), normSph) > 0.0) {
      vectorAngle *= -1.0;
    }

    f8point pN = {.x = sin(vectorAngle), // Rotate the point/vector (0,1) around Z-axis with vecAngle
                  .y = cos(vectorAngle)};

    f8point pE = {.x = pN.y, // Rotate the same point/vector around Z-axis with 90 degrees
                  .y = -pN.x};

    // 1) Build the rotation matrix and put the axes-base vectors into the matrix
    double VJaa = pE.x;
    double VJab = pN.x;
    double VJba = pE.y;
    double VJbb = pN.y;

    // 2) Transform the UV vector with jacobian matrix
    double magnitude = speedVector.magnitude(); // old vector magnitude in the model space
    double uu = VJaa * speedVector.u + VJab * speedVector.v;
    double vv = VJba * speedVector.u + VJbb * speedVector.v;

    // 3) Apply scaling of the vector so that the vector keeps the original length (model space)
    double newMagnitude = hypot(uu, vv);
    speedVector.u = uu * magnitude / newMagnitude;
    speedVector.v = vv * magnitude / newMagnitude;
  }
  return speedVector;
}

bool isGridRelative(CDataSource *dataSource) {
  bool verboseLog = true;
  bool gridRelative = true; // default is gridRelative=true

  if (dataSource->getNumDataObjects() >= 6) {
    // Check standard_name/var_name for first vector component
    // if x_wind/grid_east_wind of y_wind/grid_northward_wind then gridRelative=true
    // if eastward_wind/northward_wind then gridRelative=false
    // default is gridRelative=true
    CT::string standard_name = dataSource->getDataObject(4)->getStandardName();

    if (standard_name.equals("x_wind") || standard_name.equals("grid_eastward_wind") || standard_name.equals("y_wind") || standard_name.equals("grid_northward_wind")) {
      gridRelative = true;
    } else {
      gridRelative = false;
    }
    if (verboseLog) {
      CDBDebug("Grid property gridRelative=%d", gridRelative);
    }
  }
  return gridRelative;
}

int applyUVConversion(CImageWarper *warper, CDataSource *dataSource, int *dPixelExtent, float *uValues, float *vValues) {

  int gridWidth = dPixelExtent[2] - dPixelExtent[0];
  // If there are 2 components, we have wind u and v.
  // Use Jacobians for rotating u and v
  // After calculations
  bool gridRelative = isGridRelative(dataSource);
  for (int y = dPixelExtent[1]; y < dPixelExtent[3]; y = y + 1) {
    for (int x = dPixelExtent[0]; x < dPixelExtent[2]; x = x + 1) {
      size_t p = size_t(x - dPixelExtent[0]) + ((y - dPixelExtent[1]) * gridWidth);
      f8point gridCoordLL = {.x = dataSource->dfCellSizeX * x + dataSource->dfBBOX[0], .y = dataSource->dfCellSizeY * y + dataSource->dfBBOX[3]};
      f8point gridCoordUR = {.x = gridCoordLL.x + fabs(dataSource->dfCellSizeX), .y = gridCoordLL.y + fabs(dataSource->dfCellSizeY)};
      f8component newSpeed = jacobianTransform({.u = uValues[p], .v = vValues[p]}, gridCoordLL, gridCoordUR, warper, gridRelative);
      uValues[p] = newSpeed.u;
      vValues[p] = newSpeed.v;
    }
  }
  return 0;
}

std::vector<CalculatedWindVector> calculateBarbsAndVectorsAndSpeedFromUVComponents(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage, bool enableShade, bool enableContour,
                                                                                   bool enableBarb, bool drawMap, bool enableVector, bool drawGridVectors, int *dPixelExtent, float *uValueData,
                                                                                   float *vValueData, int *dpDestX, int *dpDestY) {
  float fNodataValue = dataSource->getDataObject(0)->dfNodataValue;
  CT::string units = "m/s";
  units = dataSource->getDataObject(0)->getUnits();

  // Wind VECTOR
  int dImageWidth = drawImage->Geo->dWidth + 1;
  int dImageHeight = drawImage->Geo->dHeight + 1;
  int dPixelDestW = dPixelExtent[2] - dPixelExtent[0];
  int dPixelDestH = dPixelExtent[3] - dPixelExtent[1];
  size_t numDestPixels = (dPixelDestW + 1) * (dPixelDestH + 1);

  std::vector<CalculatedWindVector> windVectors; // holds windVectors after calculation to draw them on top
  bool convertToKnots = false;                   // default is false
                                                 // if((enableVector||enableBarb))

  int firstXPos = 0;
  int firstYPos = 0;

  double tx = ((-drawImage->Geo->dfBBOX[0]) / (drawImage->Geo->dfBBOX[2] - drawImage->Geo->dfBBOX[0])) * double(dImageWidth);
  double ty = dImageHeight - ((-drawImage->Geo->dfBBOX[1]) / (drawImage->Geo->dfBBOX[3] - drawImage->Geo->dfBBOX[1])) * double(dImageHeight);

  // Are u/v values in m/s? (Should we convert for wind barb drawing?)
  // Depends on value units
  // Derive convertToKnots from units

  if (verboseLog) {
    CDBDebug("units = %s", units.c_str());
  }
  if (!(units.equals("kts") || units.equals("knots"))) convertToKnots = true;

  // Number of pixels between the vectors:
  int vectorDensityPy = 60; // 22;
  int vectorDensityPx = 60; // 22;
  firstXPos = int(tx) % vectorDensityPy;
  firstYPos = int(ty) % vectorDensityPx;

  int stepx = vectorDensityPx; // Raster stride at barb distances
  int stepy = vectorDensityPy;
  // If contouring, drawMap or shading is wanted, step through all destination raster points
  if (enableContour || enableShade || drawMap) {
    stepy = 1;
    stepx = 1;
  }
  // Loops through complete destination image in screen coord.
  if (!drawGridVectors) {
    for (int y = firstYPos - vectorDensityPx; y < dImageHeight; y = y + stepy) {
      for (int x = firstXPos - vectorDensityPy; x < dImageWidth; x = x + stepx) {
        if (x >= 0 && y >= 0) {
          size_t p = size_t(x + y * dImageWidth); // pointer in dest. image
          f8component comp = {.u = uValueData[p], .v = vValueData[p]};
          if (comp.u == comp.u && comp.v == comp.v && comp.u != fNodataValue && comp.v != fNodataValue) {

            if ((int(x - firstXPos) % vectorDensityPy == 0 && (y - firstYPos) % vectorDensityPx == 0) || (enableContour == false && enableShade == false)) {
              // Calculate coordinates from requested coordinate system
              double projectedCoordX = ((double(x) / double(dImageWidth)) * (drawImage->Geo->dfBBOX[2] - drawImage->Geo->dfBBOX[0])) + drawImage->Geo->dfBBOX[0];
              double projectedCoordY = ((double(dImageHeight - y) / double(dImageHeight)) * (drawImage->Geo->dfBBOX[3] - drawImage->Geo->dfBBOX[1])) + drawImage->Geo->dfBBOX[1];

              // Transform view point on screen to lat/lon
              double coordLon = projectedCoordX;
              double coordLat = projectedCoordY;
              warper->reprojToLatLon(coordLon, coordLat);

              //  Create offset coordinate, 0.001 degree higher
              double projectedCoordXOffset = coordLon;
              double projectedCoordYOffset = coordLat - 0.001;
              // Convert back to view coordinate
              warper->reprojfromLatLon(projectedCoordXOffset, projectedCoordYOffset);

              // Calculate angle correction
              float viewDirCorrection = ((atan2(projectedCoordYOffset - projectedCoordY, projectedCoordXOffset - projectedCoordX))) + M_PI_2;
              bool flip = coordLat < 0; // Remember if we have to flip barb dir for southern hemisphere
              flip = false;

              windVectors.push_back({.x = x, .y = y, .dir = comp.direction(), .viewDirCorrection = viewDirCorrection, .strength = comp.magnitude(), .convertToKnots = convertToKnots, .flip = flip});
            }
          }
        }
      }
    }
  }

  // TODO: Follow up
  if (((enableVector || enableBarb) && drawGridVectors)) {
    int wantedSpacing = 40;
    float distPoint = hypot(dpDestX[numDestPixels / 2 + 1] - dpDestX[numDestPixels / 2], dpDestY[numDestPixels / 2 + 1] - dpDestY[numDestPixels / 2]);

    int stepx = int(float(wantedSpacing) / distPoint + 0.5);
    if (stepx < 1) stepx = 1;
    int stepy = stepx;
    for (int y = dPixelExtent[1]; y < dPixelExtent[3]; y = y + stepy) { // TODO Refactor to GridExtent
      for (int x = dPixelExtent[0]; x < dPixelExtent[2]; x = x + stepx) {
        size_t p = size_t((x - (dPixelExtent[0])) + ((y - (dPixelExtent[1])) * dPixelDestW));
        // Skip rest if x,y outside drawArea
        if ((dpDestX[p] >= 0) && (dpDestX[p] < dImageWidth) && (dpDestY[p] >= 0) && (dpDestY[p] < dImageHeight)) {
          f8component comp = {.u = uValueData[p], .v = vValueData[p]};
          if (comp.u != fNodataValue && comp.v != fNodataValue) {
            // TODO IN FOLLOW UP
            // windVectors.push_back({.x = dpDestX[p], .y = dpDestY[p], .dir = comp.direction(), .strength = comp.magnitude(), .convertToKnots = convertToKnots, .flip = false});
          }
        }
      }
    }
  }

  return windVectors;
}
