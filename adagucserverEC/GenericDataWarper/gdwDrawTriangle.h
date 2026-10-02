#ifndef GDW_DRAWTRIANGLE_UTILS_H
#define GDW_DRAWTRIANGLE_UTILS_H

#include <climits>
#include <cmath>
#include "GenericDataWarper/GDWState.h"

// Header only, so that drawFunction (any callable with signature void(int x, int y, T value, GDWState &warperState)) can be inlined in the pixel loop.
// Only destination rows in [bandTop, bandBottom) are drawn, this is used to draw bands of rows in separate threads.
template <typename T, typename DrawFn>
int gdwDrawTriangle(const double triangleXCoords[3], const double triangleYCoords[3], const T &value, bool tUp, GDWState &warperState, const DrawFn &drawFunction, int bandTop = 0,
                    int bandBottom = INT_MAX) {
  int W = warperState.destGridWidth;
  int H = warperState.destGridHeight;
  if (triangleXCoords[0] < 0 && triangleXCoords[1] < 0 && triangleXCoords[2] < 0) return 0;
  if (triangleXCoords[0] >= W && triangleXCoords[1] >= W && triangleXCoords[2] >= W) return 0;
  if (triangleYCoords[0] < 0 && triangleYCoords[1] < 0 && triangleYCoords[2] < 0) return 0;
  if (triangleYCoords[0] >= H && triangleYCoords[1] >= H && triangleYCoords[2] >= H) return 0;

  int lower = -1;
  int middle = -1;
  int upper = -1;

  const double xP[3] = {std::round(triangleXCoords[0]), std::round(triangleXCoords[1]), std::round(triangleXCoords[2])};
  const double yP[3] = {std::round(triangleYCoords[0]), std::round(triangleYCoords[1]), std::round(triangleYCoords[2])};

  /*Sort the vertices in Y direction*/
  if (yP[0] < yP[1]) {
    if (yP[0] < yP[2]) {
      lower = 0;
      if (yP[1] < yP[2]) {
        middle = 1;
        upper = 2;
      } else {
        middle = 2;
        upper = 1;
      }
    } else {
      middle = 0;
      lower = 2;
      upper = 1;
    }
  } else {
    if (yP[1] < yP[2]) {
      lower = 1;
      if (yP[0] < yP[2]) {
        middle = 0;
        upper = 2;
      } else {
        middle = 2;
        upper = 0;
      }
    } else {
      middle = 1;
      lower = 2;
      upper = 0;
    }
  }

  double Y1 = yP[lower];
  double Y3 = yP[upper];
  double ylength = Y3 - Y1;
  // The triangle is less than a pixel height, it has no area so ignore.
  if (Y1 >= H || ylength < 1) {
    return 0;
  }
  double Y2 = yP[middle];
  double X1 = xP[lower];
  double X2 = xP[middle];
  double X3 = xP[upper];

  double xv1 = xP[0];
  double xv2 = xP[1];
  double xv3 = xP[2];
  double yv1 = yP[0];
  double yv2 = yP[1];
  double yv3 = yP[2];

  double screenW = W;
  double screenH = H;

  double xlength = X3 - X1;
  double ylength_A = Y2 - Y1;
  double ylength_B = Y3 - Y2;
  double xlength_A = X2 - X1;
  double xlength_B = X3 - X2;
  double r_longside = xlength / ylength;

  // The triangle is completely outside the band of rows to draw
  if (Y3 <= bandTop || Y1 >= bandBottom) {
    return 0;
  }

  // clip startY and endY to the screen
  double sy = Y1 < 0 ? 0 : Y1;
  double ey = Y3 > screenH ? screenH : Y3;

  /* https://codeplea.com/triangular-interpolation */

  // If triangle partA has no length, directly go to second part.
  double r_upper = 0;
  if (ylength_A == 0) {
    sy = Y2 < 0 ? 0 : Y2;
  } else {
    r_upper = xlength_A / ylength_A;
  }

  // clip startY and endY to the band of rows to draw
  if (sy < bandTop) sy = bandTop;
  if (ey > bandBottom) ey = bandBottom;

  double r_lower = ylength_B > 0 ? xlength_B / ylength_B : 0;
  double dn = ((yv2 - yv3) * (xv1 - xv3) + (xv3 - xv2) * (yv1 - yv3));
  // All corners on one line, the triangle has no area. This would otherwise give a division by zero for the barycentric weights.
  if (dn == 0) {
    return 0;
  }
  double invDn = 1.0 / dn;
  // The barycentric weights WV1 and WV2 are linear in x, so along a row they change with a constant step per pixel.
  double dWV1dx = (yv2 - yv3) * invDn;
  double dWV2dx = (yv3 - yv1) * invDn;
  for (double y = sy; y < ey; y++) {
    double x_longside = r_longside * (y - Y1) + X1;
    double sx = x_longside;
    double ex = y < Y2 ? (r_upper * (y - Y1) + X1) : (r_lower * (y - Y2) + X2);
    double minx = sx > ex ? ex : sx;
    double maxx = sx > ex ? sx : ex;
    double startX = minx < 0 ? 0 : minx;
    double endX = maxx > screenW ? screenW : maxx;
    double WV1 = ((yv2 - yv3) * (startX - xv3) + (xv3 - xv2) * (y - yv3)) * invDn;
    double WV2 = ((yv3 - yv1) * (startX - xv3) + (xv1 - xv3) * (y - yv3)) * invDn;
    for (double x = startX; x < endX; x++, WV1 += dWV1dx, WV2 += dWV2dx) {
      // WV3 = 1 - WV1 - WV2. For the upper triangle, dx is WV1 and dy is WV2. For the lower triangle, dx is WV2 + WV3 and dy is WV1 + WV2.
      warperState.sourceTileDx = tUp ? WV1 : 1 - WV1;
      warperState.sourceTileDy = tUp ? WV2 : WV1 + WV2;
      warperState.destIndexX = x;
      warperState.destIndexY = y;
      drawFunction(x, y, value, warperState);
    }
  }
  return 0;
}

#endif
