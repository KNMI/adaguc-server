#include <CImgRenderFieldVectors.h>
#include "f8vector.h"
#include <utils.h>
#include <string>
#include <vector>
#include "CCDFObject.h"
#include "CDataSource.h"
#include "CImageWarper.h"
#include "CTString.h"
#include "Types/CPointTypes.h"
#include "Types/GeoParameters.h"

bool verboseLog = false;

/**
 * Rotates a grid-relative wind vector (u along the grid's x axis, v along its y axis) to eastward/northward components.
 *
 * The rotation angle is the angle between grid north (the grid's +y direction) and true north at gridPoint. Grid north is
 * found by reprojecting gridPoint and a second point gridYStep further along the grid's y axis. The grid is assumed to be
 * conformal (grid x perpendicular to grid y on the earth), so a single angle describes the whole transformation.
 *
 * @param speedVector The u/v components in grid space
 * @param gridPoint Location in model (grid) coordinates where the angle is evaluated, normally the grid point of the value
 * @param gridYStep Positive step along the grid's y axis in model coordinates, used to determine grid north. A small fraction of a cell.
 * @param warper Warper initialised with the model projection
 * @param gridRelative If false, the vector is already east/north and is returned unchanged
 */
f8component jacobianTransform(f8component speedVector, f8point gridPoint, double gridYStep, CImageWarper *warper, bool gridRelative) {
  if (gridRelative) {
    f8point pnt0 = gridPoint;
    warper->reprojModelToLatLon(pnt0);
    f8point pntRad0 = pnt0.rad();
    f8point pntN = {.x = gridPoint.x, .y = gridPoint.y + gridYStep};
    warper->reprojModelToLatLon(pntN);
    f8point pntRadN = pntN.rad();

    // pntN = gridPoint moved gridYStep along the grid's y axis (grid north)
    //     ^
    //     |
    //     |
    // pnt0 = gridPoint, where we have (u,v)
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
  bool verboseLog = false;
  bool gridRelative = true; // default is gridRelative=true

  if (dataSource->getNumDataObjects() >= 6) {
    // Check standard_name/var_name for first vector component
    // if x_wind/grid_east_wind of y_wind/grid_northward_wind then gridRelative=true
    // if eastward_wind/northward_wind then gridRelative=false
    // default is gridRelative=true
    std::string standard_name = getStandardName(*dataSource->getDataObject(4)->cdfVariable);

    if (standard_name == "x_wind" || standard_name == "grid_eastward_wind" || standard_name == "y_wind" || standard_name == "grid_northward_wind") {
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
  // Step along the grid's y axis used to determine grid north: a small fraction of a cell, so the angle is that at the grid point itself.
  const double gridYStep = 0.01 * fabs(dataSource->dfCellSizeY);
  for (int y = dPixelExtent[1]; y < dPixelExtent[3]; y = y + 1) {
    for (int x = dPixelExtent[0]; x < dPixelExtent[2]; x = x + 1) {
      size_t p = size_t(x - dPixelExtent[0]) + ((y - dPixelExtent[1]) * gridWidth);
      // Evaluate the rotation at the grid point (the cell centre), where the u/v value is defined and where barbs are drawn.
      f8point gridPoint = {.x = dataSource->dfCellSizeX * (x + 0.5) + dataSource->dfBBOX[0], .y = dataSource->dfCellSizeY * (y + 0.5) + dataSource->dfBBOX[3]};
      f8component newSpeed = jacobianTransform({.u = uValues[p], .v = vValues[p]}, gridPoint, gridYStep, warper, gridRelative);
      uValues[p] = newSpeed.u;
      vValues[p] = newSpeed.v;
    }
  }
  return 0;
}

