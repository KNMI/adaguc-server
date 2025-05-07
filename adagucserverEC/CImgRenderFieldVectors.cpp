#include <CImgRenderFieldVectors.h>
#include "CImgWarpBilinear.h"
#include "f8vector.h"

bool verboseLog = true;

std::tuple<double, double> doit(double uComponent, double vComponent, double modelX, double modelY, CImageWarper *warper, bool gridRelative, float fNodataValue, double lon_pntEast, double lat_pntEast,
                                double lon_pntNorth, double lat_pntNorth) {
  double delta = 0.01;
  double deltaLon = delta;
  double deltaLat = delta;

  if ((uComponent != fNodataValue) && (vComponent != fNodataValue)) {

    if (gridRelative) {

      warper->reprojModelToLatLon(lon_pntNorth, lat_pntEast);
      warper->reprojModelToLatLon(lon_pntNorth, lat_pntNorth);
      warper->reprojModelToLatLon(lon_pntEast, lat_pntEast);

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

      double dLonNorth = radians(lon_pntNorth);
      double dLatNorth = radians(lat_pntNorth);

      f8vector pntNorthSph = {.x = cos(dLatNorth) * cos(dLonNorth), .y = cos(dLatNorth) * sin(dLonNorth), .z = sin(dLatNorth)};

      double lon_pnt0 = radians(lon_pntNorth);
      double lat_pnt0 = radians(lat_pntEast);

      f8vector pnt0Sph = {.x = cos(lat_pnt0) * cos(lon_pnt0), .y = cos(lat_pnt0) * sin(lon_pnt0), .z = sin(lat_pnt0)};

      f8vector pntNorthSphNorm = (pntNorthSph - pnt0Sph).norm();

      f8vector normSph = pnt0Sph.norm();

      f8vector pntNorthSphRotNorm = f8vector({
                                                 .x = -normSph.z * normSph.x,
                                                 .y = -normSph.z * normSph.y,
                                                 .z = 1.0 - normSph.z * normSph.z,
                                             })
                                        .norm();

      double vecAngleF8 = acos(dot(pntNorthSphNorm, pntNorthSphRotNorm));

      // Determine the sign of the angle
      if (dot(cross(pntNorthSphRotNorm, pntNorthSphNorm), normSph) > 0.0) {
        vecAngleF8 *= -1.0;
      }

      double xpntNorthSph = sin(vecAngleF8); // Rotate the point/vector (0,1) around Z-axis with vecAngle
      double ypntNorthSph = cos(vecAngleF8);
      double xpntEastSph = ypntNorthSph; // Rotate the same point/vector around Z-axis with 90 degrees
      double ypntEastSph = -xpntNorthSph;

      // zpntNorthSph = 0; zpntEastSph = 0;  // not needed in 2D

      // 1) Build the rotation matrix and put the axes-base vectors into the matrix
      double VJaa = xpntEastSph;
      double VJab = xpntNorthSph;
      double VJba = ypntEastSph;
      double VJbb = ypntNorthSph;

      // 2) Transform the UV vector with jacobian matrix
      double u = uComponent;
      double v = vComponent;
      //              u = 0.0;  v = 6.0; // test: 6 m/s along the easting direction of the grid
      double magnitude = hypot(u, v); // old vector magnitude in the model space
      //(uu) =   (VJaa VJab) * ( u )
      //(vv)     (VJba VJbb)   ( v )
      double uu = VJaa * u + VJab * v;
      double vv = VJba * u + VJbb * v;
      //(uu) =   (VJaa VJab VJac) * ( u )
      //(vv)     (VJba VJbb VJbc)   ( v )
      //(ww)     (VJba VJbb VJcc)   ( w )

      // 3) Apply scaling of the vector so that the vector keeps the original length (model space)
      double newMagnitude = hypot(uu, vv);
      uComponent = uu * magnitude / newMagnitude;
      vComponent = vv * magnitude / newMagnitude;
    }
    warper->reprojModelToLatLon(modelX, modelY); // model to latlon proj.
    double modelXLon = modelX + deltaLon;        // latlons
    double modelYLon = modelY;
    double modelXLat = modelX;
    double modelYLat = modelY + deltaLat;
    warper->reprojfromLatLon(modelX, modelY); // latlon to vis proj.
    warper->reprojfromLatLon(modelXLon, modelYLon);
    warper->reprojfromLatLon(modelXLat, modelYLat);

    double distLon = hypot(modelXLon - modelX, modelYLon - modelY);
    double distLat = hypot(modelXLat - modelX, modelYLat - modelY);

    double VJaa = (modelXLon - modelX) / distLon;
    double VJab = (modelXLat - modelX) / distLat;
    double VJba = (modelYLon - modelY) / distLon;
    double VJbb = (modelYLat - modelY) / distLat;
    double magnitude = hypot(uComponent, vComponent);
    double uu;
    double vv;
    uu = VJaa * uComponent + VJab * vComponent;
    vv = VJba * uComponent + VJbb * vComponent;
    double newMagnitude = hypot(uu, vv);
    uComponent = uu * magnitude / newMagnitude;
    vComponent = vv * magnitude / newMagnitude;
  }
  return std::tuple<double, double>(uComponent, vComponent);
}

int applyUVConversion(CImageWarper *warper, CDataSource *sourceImage, bool enableVector, bool enableBarb, int *dPixelExtent, float *uValues, float *vValues) {

  double dfSourceExtW = (sourceImage->dfBBOX[2] - sourceImage->dfBBOX[0]);
  double dfSourceExtH = (sourceImage->dfBBOX[1] - sourceImage->dfBBOX[3]);
  double dfSourcedExtW = dfSourceExtW / sourceImage->dWidth;
  double dfSourcedExtH = dfSourceExtH / sourceImage->dHeight;
  double dfSourceOrigX = sourceImage->dfBBOX[0];
  double dfSourceOrigY = sourceImage->dfBBOX[3];
  int dPixelDestW = dPixelExtent[2] - dPixelExtent[0];

  float fNodataValue = sourceImage->getDataObject(0)->dfNodataValue;

  // If there are 2 components, we have wind u and v.
  // Use Jacobians for rotating u and v
  // After calculations
  bool gridRelative = true;

  // TODO this should be enabled when U and V components are available. otherwise it should do nothing.
  if (sourceImage->getNumDataObjects() > 1 && (enableVector || enableBarb)) {
    // Check standard_name/var_name for first vector component
    // if x_wind/grid_east_wind of y_wind/grid_northward_wind then gridRelative=true
    // if eastward_wind/northward_wind then gridRelative=false
    // default is gridRelative=true
    CT::string standard_name;
    standard_name = sourceImage->getDataObject(0)->variableName;
    try {
      sourceImage->getDataObject(0)->cdfVariable->getAttribute("standard_name")->getDataAsString(&standard_name);
    } catch (CDFError e) {
    }
    if (standard_name.equals("x_wind") || standard_name.equals("grid_eastward_wind") || standard_name.equals("y_wind") || standard_name.equals("grid_northward_wind")) {
      gridRelative = true;
    } else {
      gridRelative = false;
    }
    if (verboseLog) {
      CDBDebug("Grid propery gridRelative=%d", gridRelative);
    }

    if (verboseLog) {
      CDBDebug("Data raster: %f,%f with %f,%f (%f,%f) ll: (%d,%d) ur: (%d,%d) [%d,%d]\n", dfSourceOrigX, dfSourceOrigY, dfSourcedExtW, dfSourcedExtH, dfSourceExtW, dfSourceExtH, dPixelExtent[0],
               dPixelExtent[1], dPixelExtent[2], dPixelExtent[3], dPixelDestW, 0);
    }

    for (int y = dPixelExtent[1]; y < dPixelExtent[3]; y = y + 1) {
      for (int x = dPixelExtent[0]; x < dPixelExtent[2]; x = x + 1) {
        size_t p = size_t((x - (dPixelExtent[0])) + ((y - (dPixelExtent[1])) * (dPixelDestW + 1)));

        double modelX = (x == dPixelExtent[2] - 1) ? dfSourcedExtW * (x - 1) + dfSourceOrigX : dfSourcedExtW * (x) + dfSourceOrigX;
        double modelY = (y == dPixelExtent[3] - 1) ? dfSourcedExtH * double(y - 1) + dfSourceOrigY : dfSourcedExtH * double(y) + dfSourceOrigY;

        double uComponent, vComponent;
        // TODO: Avoid overwriting original data.

        double lon_pnt0 = dfSourcedExtW * double(x) + dfSourceOrigX;
        double lat_pnt0 = dfSourcedExtH * double(y) + dfSourceOrigY;

        double lon_pntEast = lon_pnt0 + fabs(dfSourcedExtW);
        double lat_pntEast = lat_pnt0;
        double lon_pntNorth = lon_pnt0;
        double lat_pntNorth = lat_pnt0 + fabs(dfSourcedExtH);

        std::tie(uComponent, vComponent) = doit(uValues[p], vValues[p], modelX, modelY, warper, gridRelative, fNodataValue, lon_pntEast, lat_pntEast, lon_pntNorth, lat_pntNorth);
        uValues[p] = uComponent;
        vValues[p] = vComponent;
      }
    }
  }
  return 0;
};

std::vector<CalculatedWindVector> calculateBarbsAndVectorsAndSpeedFromUVComponents(CImageWarper *warper, CDataSource *sourceImage, CDrawImage *drawImage, bool enableShade, bool enableContour,
                                                                                   bool enableBarb, bool drawMap, bool enableVector, bool drawGridVectors, int *dPixelExtent, float *uValueData,
                                                                                   float *vValueData, int *dpDestX, int *dpDestY) {
  float fNodataValue = sourceImage->getDataObject(0)->dfNodataValue;

  // Wind VECTOR
  int dImageWidth = drawImage->Geo->dWidth + 1;
  int dImageHeight = drawImage->Geo->dHeight + 1;
  int dPixelDestW = dPixelExtent[2] - dPixelExtent[0];
  int dPixelDestH = dPixelExtent[3] - dPixelExtent[1];
  size_t numDestPixels = (dPixelDestW + 1) * (dPixelDestH + 1);

  std::vector<CalculatedWindVector> windVectors; // holds windVectors after calculation to draw them on top
  bool convertToKnots = false;                   // default is false
                                                 // if((enableVector||enableBarb))

  if (sourceImage->getNumDataObjects() == 2) {
    int firstXPos = 0;
    int firstYPos = 0;

    double tx = ((-drawImage->Geo->dfBBOX[0]) / (drawImage->Geo->dfBBOX[2] - drawImage->Geo->dfBBOX[0])) * double(dImageWidth);
    double ty = dImageHeight - ((-drawImage->Geo->dfBBOX[1]) / (drawImage->Geo->dfBBOX[3] - drawImage->Geo->dfBBOX[1])) * double(dImageHeight);

    // Are u/v values in m/s? (Should we convert for wind barb drawing?)
    // Depends on value units
    // Derive convertToKnots from units
    CT::string units = "m/s";
    units = sourceImage->getDataObject(0)->getUnits();
    if (verboseLog) {
      CDBDebug("units = %s", units.c_str());
    }
    if (!(units.equals("kts") || units.equals("knots"))) convertToKnots = true;

    // Number of pixels between the vectors:
    int vectorDensityPy = 60; // 22;
    int vectorDensityPx = 60; // 22;
    firstXPos = int(tx) % vectorDensityPy;
    firstYPos = int(ty) % vectorDensityPx;
    double u, v;

    double direction = 0;
    double strength;
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
          u = uValueData[p];
          v = vValueData[p];
          if ((u != fNodataValue) && (v != fNodataValue)) {
            direction = atan2(v, u);
            strength = sqrt(u * u + v * v);
            uValueData[p] = strength;

            if (drawMap == true) {
              CImgWarpBilinear::setValuePixel(sourceImage, drawImage, x, y, strength);
            }
            if (!drawGridVectors) {
              if ((int(x - firstXPos) % vectorDensityPy == 0 && (y - firstYPos) % vectorDensityPx == 0) || (enableContour == false && enableShade == false)) {
                strength = (strength) * 1.0;

                // Calculate coordinates from requested coordinate system
                double projectedCoordX = ((double(x) / double(dImageWidth)) * (drawImage->Geo->dfBBOX[2] - drawImage->Geo->dfBBOX[0])) + drawImage->Geo->dfBBOX[0];
                double projectedCoordY = ((double(dImageHeight - y) / double(dImageHeight)) * (drawImage->Geo->dfBBOX[3] - drawImage->Geo->dfBBOX[1])) + drawImage->Geo->dfBBOX[1];

                warper->reprojToLatLon(projectedCoordX, projectedCoordY);

                bool flip = projectedCoordY < 0; // Remember if we have to flip barb dir for southern hemisphere
                flip = false;
                windVectors.push_back({.x = x, .y = y, .dir = direction, .strength = strength, .convertToKnots = convertToKnots, .flip = flip});
              }
            }
          } else
            uValueData[p] = sourceImage->getDataObject(0)->dfNodataValue; // Set speeed to nodata if u OR v == no data
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
    if (sourceImage->getNumDataObjects() == 2) {
      for (int y = dPixelExtent[1]; y < dPixelExtent[3]; y = y + stepy) { // TODO Refactor to GridExtent
        for (int x = dPixelExtent[0]; x < dPixelExtent[2]; x = x + stepx) {
          size_t p = size_t((x - (dPixelExtent[0])) + ((y - (dPixelExtent[1])) * dPixelDestW));
          // Skip rest if x,y outside drawArea
          if ((dpDestX[p] >= 0) && (dpDestX[p] < dImageWidth) && (dpDestY[p] >= 0) && (dpDestY[p] < dImageHeight)) {
            double direction;
            double strength;
            double u = uValueData[p];
            double v = vValueData[p];
            if ((u != fNodataValue) && (v != fNodataValue)) {
              direction = atan2(v, u);
              strength = sqrt(u * u + v * v);
              windVectors.push_back({.x = dpDestX[p], .y = dpDestY[p], .dir = direction, .strength = strength, .convertToKnots = convertToKnots, .flip = false});
            }
          }
        }
      }
    }
  }

  return windVectors;
}
