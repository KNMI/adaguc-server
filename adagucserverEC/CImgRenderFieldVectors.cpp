#include <CImgRenderFieldVectors.h>
#include "f8vector.h"
#include <algorithm>
#include <utils.h>
#include <string>
#include "CDataSource.h"
#include "CImageWarper.h"
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
    //
    // Both points are placed on the unit sphere. Grid north at pnt0 is the direction from pnt0 to pntN, projected onto the
    // tangent plane at pnt0. Without the projection the chord dips below the tangent plane by half the arc it spans, and
    // that dip would be read as a rotation (for example on a plain lat/lon grid, where the correct angle is 0).
    // True north at pnt0 is the earth's axis (0, 0, 1) projected onto the same tangent plane.

    f8vector pntNSph = {.x = cos(pntRadN.y) * cos(pntRadN.x), .y = cos(pntRadN.y) * sin(pntRadN.x), .z = sin(pntRadN.y)};
    f8vector pnt0Sph = {.x = cos(pntRad0.y) * cos(pntRad0.x), .y = cos(pntRad0.y) * sin(pntRad0.x), .z = sin(pntRad0.y)};
    f8vector normSph = pnt0Sph.norm();
    f8vector chord = pntNSph - pnt0Sph;
    double chordAlongNormal = dot(chord, normSph);
    f8vector chordInTangentPlane = {
        .x = chord.x - chordAlongNormal * normSph.x,
        .y = chord.y - chordAlongNormal * normSph.y,
        .z = chord.z - chordAlongNormal * normSph.z,
    };
    f8vector pntNSphNorm = chordInTangentPlane.norm();
    f8vector pntNSphRot = {
        .x = -normSph.z * normSph.x,
        .y = -normSph.z * normSph.y,
        .z = 1.0 - normSph.z * normSph.z,
    };
    f8vector pntNSphRotNorm = pntNSphRot.norm();

    // Both vectors have unit length, but rounding can push their dot product just outside [-1, 1], where acos returns NaN.
    double vectorAngle = acos(std::clamp(dot(pntNSphNorm, pntNSphRotNorm), -1.0, 1.0));

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
      // This assumes u and v share the grid points given by the coordinate variables (Arakawa A, not staggered). That holds
      // for HARMONIE output as encoded in its GRIB files: u, v and the mass fields have one grid definition without offsets.
      // Staggered (e.g. Arakawa C) u/v fields are not supported.
      f8point gridPoint = {.x = dataSource->dfCellSizeX * (x + 0.5) + dataSource->dfBBOX[0], .y = dataSource->dfCellSizeY * (y + 0.5) + dataSource->dfBBOX[3]};
      f8component newSpeed = jacobianTransform({.u = uValues[p], .v = vValues[p]}, gridPoint, gridYStep, warper, gridRelative);
      uValues[p] = newSpeed.u;
      vValues[p] = newSpeed.v;
    }
  }
  return 0;
}

