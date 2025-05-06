#include <CImgRenderFieldVectors.h>
#include "CImgWarpBilinear.h"

#define NormVector(vec0, vec1, vec2)                                                                                                                                                                   \
  {                                                                                                                                                                                                    \
    double vecLen = sqrt(vec0 * vec0 + vec1 * vec1 + vec2 * vec2);                                                                                                                                     \
    vec0 = vec0 / vecLen;                                                                                                                                                                              \
    vec1 = vec1 / vecLen;                                                                                                                                                                              \
    vec2 = vec2 / vecLen;                                                                                                                                                                              \
  }

#define CrossProd(vecx0, vecx1, vecx2, vecy0, vecy1, vecy2, vecz0, vecz1, vecz2)                                                                                                                       \
  {                                                                                                                                                                                                    \
    vecz0 = vecx1 * vecy2 - vecy1 * vecx2;                                                                                                                                                             \
    vecz1 = vecx2 * vecy0 - vecy2 * vecx0;                                                                                                                                                             \
    vecz2 = vecx0 * vecy1 - vecy0 * vecx1;                                                                                                                                                             \
  }

bool verboseLog = true;

int applyUVConversion(CImageWarper *warper, CDataSource *sourceImage, bool enableVector, bool enableBarb, int *dPixelExtent, float *uValues, float *vValues) {

  double dfSourceExtW = (sourceImage->dfBBOX[2] - sourceImage->dfBBOX[0]);
  double dfSourceExtH = (sourceImage->dfBBOX[1] - sourceImage->dfBBOX[3]);
  double dfSourceW = double(sourceImage->dWidth);
  double dfSourceH = double(sourceImage->dHeight);
  double dfSourcedExtW = dfSourceExtW / dfSourceW;
  double dfSourcedExtH = dfSourceExtH / dfSourceH;
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

    double delta = 0.01;
    double deltaLon;
    double deltaLat;

    if (verboseLog) {
      CDBDebug("Data raster: %f,%f with %f,%f (%f,%f) ll: (%d,%d) ur: (%d,%d) [%d,%d]\n", dfSourceOrigX, dfSourceOrigY, dfSourcedExtW, dfSourcedExtH, dfSourceExtW, dfSourceExtH, dPixelExtent[0],
               dPixelExtent[1], dPixelExtent[2], dPixelExtent[3], dPixelDestW, 0);
    }
    if (dfSourcedExtH < 0) {
      deltaLat = -delta;
    } else {
      deltaLat = +delta;
    }
    if (dfSourcedExtW > 0) {
      deltaLon = delta;
    } else {
      deltaLon = -delta;
    }
    deltaLat = +delta; // TODO Check this (and delete previous 2 if blocks)
    deltaLon = delta;

    for (int y = dPixelExtent[1]; y < dPixelExtent[3]; y = y + 1) {
      for (int x = dPixelExtent[0]; x < dPixelExtent[2]; x = x + 1) {
        size_t p = size_t((x - (dPixelExtent[0])) + ((y - (dPixelExtent[1])) * (dPixelDestW + 1)));
        if ((uValues[p] != fNodataValue) && (vValues[p] != fNodataValue)) {
          double modelX, modelY;
          if (x == dPixelExtent[2] - 1) {
            modelX = dfSourcedExtW * double(x - 1) + dfSourceOrigX;
          } else {
            modelX = dfSourcedExtW * double(x) + dfSourceOrigX;
          }
          if (y == dPixelExtent[3] - 1) {
            modelY = dfSourcedExtH * double(y - 1) + dfSourceOrigY;
          } else {
            modelY = dfSourcedExtH * double(y) + dfSourceOrigY;
          }

          double modelXLat, modelYLat;
          double modelXLon, modelYLon;
          double VJaa, VJab, VJba, VJbb;

          if (gridRelative) {
            double lon_pnt0, lat_pnt0;
            double lon_pntEast, lat_pntEast;
            double lon_pntNorth, lat_pntNorth;
            double dLatNorth, dLonNorth;
            double xpntEastSph, ypntEastSph;
            double xpntNorthSph, ypntNorthSph, zpntNorthSph;
            double xpntNorthSphRot, ypntNorthSphRot, zpntNorthSphRot;
            double xpnt0Sph, ypnt0Sph, zpnt0Sph;
            double xnormSph, ynormSph, znormSph;
            double xncross, yncross, zncross;
            double vecAngle;
            double VJaa, VJab, VJba, VJbb;
            double u, v;
            double magnitude, newMagnitude;
            double uu;
            double vv;
            lon_pnt0 = dfSourcedExtW * double(x) + dfSourceOrigX;
            lat_pnt0 = dfSourcedExtH * double(y) + dfSourceOrigY;

            lon_pntEast = lon_pnt0 + fabs(dfSourcedExtW);
            lat_pntEast = lat_pnt0;
            lon_pntNorth = lon_pnt0;
            lat_pntNorth = lat_pnt0 + fabs(dfSourcedExtH);

            warper->reprojModelToLatLon(lon_pnt0, lat_pnt0);
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
            dLonNorth = radians(lon_pntNorth);
            dLatNorth = radians(lat_pntNorth);
            xpntNorthSph = cos(dLatNorth) * cos(dLonNorth);
            ypntNorthSph = cos(dLatNorth) * sin(dLonNorth); // # Get [dLonNorth,dLatNorth] on the unit sphere.
            zpntNorthSph = sin(dLatNorth);                  // # Only XY plane is needed.

            lon_pnt0 = radians(lon_pnt0);
            lat_pnt0 = radians(lat_pnt0);
            xpnt0Sph = cos(lat_pnt0) * cos(lon_pnt0);
            ypnt0Sph = cos(lat_pnt0) * sin(lon_pnt0); // # Get [lon_pnt0,lat_pnt0] on the unit sphere.
            zpnt0Sph = sin(lat_pnt0);                 // # Only XY plane is needed.

            xpntNorthSph -= xpnt0Sph, ypntNorthSph -= ypnt0Sph;
            zpntNorthSph -= zpnt0Sph;

            NormVector(xpntNorthSph, ypntNorthSph, zpntNorthSph); // vecy

            xnormSph = xpnt0Sph;
            ynormSph = ypnt0Sph;
            znormSph = zpnt0Sph;
            NormVector(xnormSph, ynormSph, znormSph); // normal vector to the sphere at the point pnt0Sph
            // # vecn = (0.0,0.0,1.0)                   // up-vector in a global coordinate system
            // # Project vecn onto plane XY, where plane-normal is vecz
            // # vecnProjXY = vecn - D*vecz;   D= a*x1+b*y1+c*z1;  vecz=(a,b,c); vecn=(x1,y1,z1)=(0,0,1)
            // #                               D= vecz[2]*1;
            // # vecyRot = NormVector( (0.0 - vecz[2]*vecz[0],0.0  - vecz[2]*vecz[1], 1.0  - vecz[2]*vecz[2]) )

            // double Dist =  xnormSph * 0.0 +  ynormSph * 0.0 + znormSph * 1.0; // Left out for optimization
            xpntNorthSphRot = -znormSph * xnormSph;      // xpntNorthSphRot = 0.0 - Dist*xnormSph;
            ypntNorthSphRot = -znormSph * ynormSph;      // ypntNorthSphRot = 0.0 - Dist*ynormSph;
            zpntNorthSphRot = 1.0 - znormSph * znormSph; // zpntNorthSphRot = 1.0 - Dist*znormSph;
            NormVector(xpntNorthSphRot, ypntNorthSphRot, zpntNorthSphRot);

            // This would create in 3D the rotated Easting vector; but we don't need it in this routine.
            // Left out to optimize computation
            // CrossProd( xpntNorthSphRot, ypntNorthSphRot, zpntNorthSphRot, xnormSph, ynormSph, znormSph,
            //            xpntEastSph,  ypntEastSphRot,  zpntEastSphRot ); //vecxRot = CrossProd(vecy,vecz)

            vecAngle = acos((xpntNorthSph * xpntNorthSphRot + ypntNorthSph * ypntNorthSphRot + zpntNorthSph * zpntNorthSphRot));
            // Determine the sign of the angle
            CrossProd(xpntNorthSphRot, ypntNorthSphRot, zpntNorthSphRot, xpntNorthSph, ypntNorthSph, zpntNorthSph, xncross, yncross, zncross);
            if ((xncross * xnormSph + yncross * ynormSph + zncross * znormSph) > 0.0) // dotProduct
              vecAngle *= -1.0;

            xpntNorthSph = sin(vecAngle); // Rotate the point/vector (0,1) around Z-axis with vecAngle
            ypntNorthSph = cos(vecAngle);
            xpntEastSph = ypntNorthSph; // Rotate the same point/vector around Z-axis with 90 degrees
            ypntEastSph = -xpntNorthSph;

            // zpntNorthSph = 0; zpntEastSph = 0;  // not needed in 2D

            // 1) Build the rotation matrix and put the axes-base vectors into the matrix
            VJaa = xpntEastSph;
            VJab = xpntNorthSph;
            VJba = ypntEastSph;
            VJbb = ypntNorthSph;

            // 2) Transform the UV vector with jacobian matrix
            u = uValues[p];
            v = vValues[p];
            //              u = 0.0;  v = 6.0; // test: 6 m/s along the easting direction of the grid
            magnitude = hypot(u, v); // old vector magnitude in the model space
            //(uu) =   (VJaa VJab) * ( u )
            //(vv)     (VJba VJbb)   ( v )
            uu = VJaa * u + VJab * v;
            vv = VJba * u + VJbb * v;
            //(uu) =   (VJaa VJab VJac) * ( u )
            //(vv)     (VJba VJbb VJbc)   ( v )
            //(ww)     (VJba VJbb VJcc)   ( w )

            // 3) Apply scaling of the vector so that the vector keeps the original length (model space)
            newMagnitude = hypot(uu, vv);
            uValues[p] = uu * magnitude / newMagnitude;
            vValues[p] = vv * magnitude / newMagnitude;
          }
          warper->reprojModelToLatLon(modelX, modelY); // model to latlon proj.
          modelXLon = modelX + deltaLon;               // latlons
          modelYLon = modelY;
          modelXLat = modelX;
          modelYLat = modelY + deltaLat;
          warper->reprojfromLatLon(modelX, modelY); // latlon to vis proj.
          warper->reprojfromLatLon(modelXLon, modelYLon);
          warper->reprojfromLatLon(modelXLat, modelYLat);

          double distLon = hypot(modelXLon - modelX, modelYLon - modelY);
          double distLat = hypot(modelXLat - modelX, modelYLat - modelY);

          VJaa = (modelXLon - modelX) / distLon;
          VJab = (modelXLat - modelX) / distLat;
          VJba = (modelYLon - modelY) / distLon;
          VJbb = (modelYLat - modelY) / distLat;
          double magnitude = hypot(uValues[p], vValues[p]);
          double uu;
          double vv;
          uu = VJaa * uValues[p] + VJab * vValues[p];
          vv = VJba * uValues[p] + VJbb * vValues[p];
          double newMagnitude = hypot(uu, vv);
          uValues[p] = uu * magnitude / newMagnitude;
          vValues[p] = vv * magnitude / newMagnitude;
        }
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
