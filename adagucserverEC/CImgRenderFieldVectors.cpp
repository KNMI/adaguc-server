#include <CImgRenderFieldVectors.h>
#include "CImgWarpBilinear.h"
#include "f8vector.h"

bool verboseLog = true;

f8component jacobianTransform(f8component speedVector, f8point gridCoordUL, f8point gridCoordLR, CImageWarper *warper, bool gridRelative) {
  // Degrees offset in latitude/longutide space to determine rotation of projected model grid per grid cell
  f8point deltaLatLon = {.x = 0.01, .y = 0.01};

  if (gridRelative) {
    f8point pnt0 = {.x = gridCoordUL.x, .y = gridCoordUL.y};
    warper->reprojModelToLatLon(pnt0);
    f8point pntRad0 = pnt0.rad();
    f8point pntN = {.x = gridCoordUL.x, .y = gridCoordLR.y};
    warper->reprojModelToLatLon(pntN);
    f8point pntRadN = pntN.rad();

    // TODO CHECK WHY NOT USED with Ernst:
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

  double modelX = gridCoordUL.x;
  double modelY = gridCoordUL.y;
  warper->reprojModelToLatLon(modelX, modelY); // model to latlon proj.
  double modelXLon = modelX + deltaLatLon.x;   // latlons
  double modelYLon = modelY;
  double modelXLat = modelX;
  double modelYLat = modelY + deltaLatLon.y;
  warper->reprojfromLatLon(modelX, modelY); // latlon to vis proj.
  warper->reprojfromLatLon(modelXLon, modelYLon);
  warper->reprojfromLatLon(modelXLat, modelYLat);

  double distLon = hypot(modelXLon - modelX, modelYLon - modelY);
  double distLat = hypot(modelXLat - modelX, modelYLat - modelY);

  double VJaa = (modelXLon - modelX) / distLon;
  double VJab = (modelXLat - modelX) / distLat;
  double VJba = (modelYLon - modelY) / distLon;
  double VJbb = (modelYLat - modelY) / distLat;
  double magnitude = speedVector.magnitude();
  double uu = VJaa * speedVector.u + VJab * speedVector.v;
  double vv = VJba * speedVector.u + VJbb * speedVector.v;
  double newMagnitude = hypot(uu, vv);
  speedVector.u = uu * magnitude / newMagnitude;
  speedVector.v = vv * magnitude / newMagnitude;

  return speedVector;
}

int applyUVConversion(CImageWarper *warper, CDataSource *dataSource, int *dPixelExtent, float *uValues, float *vValues) {
  int gridWidth = dPixelExtent[2] - dPixelExtent[0];
  // If there are 2 components, we have wind u and v.
  // Use Jacobians for rotating u and v
  // After calculations
  bool gridRelative = dataSource->isGridRelative();
  for (int y = dPixelExtent[1]; y < dPixelExtent[3]; y = y + 1) {
    for (int x = dPixelExtent[0]; x < dPixelExtent[2]; x = x + 1) {
      size_t p = size_t((x - (dPixelExtent[0])) + ((y - (dPixelExtent[1])) * (gridWidth)));
      f8point gridCoordUL = {.x = dataSource->dfCellSizeX * double(x) + dataSource->dfBBOX[0], .y = dataSource->dfCellSizeY * double(y) + dataSource->dfBBOX[3]};
      f8point gridCoordLR = {.x = dataSource->dfCellSizeX * x + fabs(dataSource->dfCellSizeX) + dataSource->dfBBOX[0],
                             .y = dataSource->dfCellSizeY * y + fabs(dataSource->dfCellSizeY) + dataSource->dfBBOX[3]};
      f8component newSpeed = jacobianTransform({.u = uValues[p], .v = vValues[p]}, gridCoordUL, gridCoordLR, warper, gridRelative);
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
  for (int y = firstYPos - vectorDensityPx; y < dImageHeight; y = y + stepy) {
    for (int x = firstXPos - vectorDensityPy; x < dImageWidth; x = x + stepx) {
      if (x >= 0 && y >= 0) {
        size_t p = size_t(x + y * dImageWidth); // pointer in dest. image
        f8component comp = {.u = uValueData[p], .v = vValueData[p]};
        if (comp.u != fNodataValue && comp.v != fNodataValue) {
          if (!drawGridVectors) {
            if ((int(x - firstXPos) % vectorDensityPy == 0 && (y - firstYPos) % vectorDensityPx == 0) || (enableContour == false && enableShade == false)) {
              // Calculate coordinates from requested coordinate system
              double projectedCoordX = ((double(x) / double(dImageWidth)) * (drawImage->Geo->dfBBOX[2] - drawImage->Geo->dfBBOX[0])) + drawImage->Geo->dfBBOX[0];
              double projectedCoordY = ((double(dImageHeight - y) / double(dImageHeight)) * (drawImage->Geo->dfBBOX[3] - drawImage->Geo->dfBBOX[1])) + drawImage->Geo->dfBBOX[1];
              warper->reprojToLatLon(projectedCoordX, projectedCoordY);
              bool flip = projectedCoordY < 0; // Remember if we have to flip barb dir for southern hemisphere
              flip = false;
              windVectors.push_back({.x = x, .y = y, .dir = comp.direction(), .strength = comp.magnitude(), .convertToKnots = convertToKnots, .flip = flip});
            }
          }
        }
      }
    }
  }

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
            windVectors.push_back({.x = dpDestX[p], .y = dpDestY[p], .dir = comp.direction(), .strength = comp.magnitude(), .convertToKnots = convertToKnots, .flip = false});
          }
        }
      }
    }
  }

  return windVectors;
}
