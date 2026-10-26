#ifndef GDW_DRAWTRIANGLE_UTILS_H
#define GDW_DRAWTRIANGLE_UTILS_H

#include <algorithm>
#include <cmath>
#include "GenericDataWarper/GDWState.h"

// Round half away from zero without a libm call (no SSE4.1 in this build).
static inline double gdwRoundHalfAwayFromZero(double v) {
  constexpr double limit = 1e15;
  v = std::min(limit, std::max(-limit, v));
  return (double)(long long)(v + std::copysign(0.5, v));
}

// Floor for already non-negative values.
static inline int gdwFloorNonNegative(double v) { return (int)v; }

// Header only, so drawFunction (void(int x, int y, T value, GDWState&)) can be inlined in the pixel loop.
// Pixel ownership: a pixel belongs to a row span iff its center is in [minx, maxx), so shared edges don't double-draw (avoids stippling on translucent colors).
// gdwDrawQuad takes unrounded corners and rounds each one once; gdwDrawTriangleRounded takes pre-rounded corners.

// tUp: which half of a quad this triangle is.
template <typename T, typename DrawFn>
int gdwDrawTriangleRounded(const double xP[3], const double yP[3], const T &value, bool tUp, GDWState &warperState, const DrawFn &drawFunction) {
  const int W = warperState.destGridWidth;
  const int H = warperState.destGridHeight;

  // Sub-pixel height: no area, skip (most triangles of an oversized grid end up here).
  if (std::max(yP[0], std::max(yP[1], yP[2])) - std::min(yP[0], std::min(yP[1], yP[2])) < 1) return 0;

  const double xv1 = xP[0], xv2 = xP[1], xv3 = xP[2];
  const double yv1 = yP[0], yv2 = yP[1], yv3 = yP[2];
  const double dn = (yv2 - yv3) * (xv1 - xv3) + (xv3 - xv2) * (yv1 - yv3);
  if (dn == 0) return 0; // Degenerate (collinear) triangle.

  // Sort corners by Y: lower, middle, upper.
  int lower, middle, upper;
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

  const double Y1 = yP[lower], Y2 = yP[middle], Y3 = yP[upper];
  if (Y1 >= H) return 0;
  const double X1 = xP[lower], X2 = xP[middle], X3 = xP[upper];

  const double ylength_A = Y2 - Y1;
  const double ylength_B = Y3 - Y2;
  const double r_longside = (X3 - X1) / (Y3 - Y1);
  const double r_upper = ylength_A == 0 ? 0 : (X2 - X1) / ylength_A;
  const double r_lower = ylength_B > 0 ? (X3 - X2) / ylength_B : 0;

  // clip startY and endY to the screen
  const int sy = (int)(ylength_A == 0 ? std::max(Y2, 0.0) : std::max(Y1, 0.0));
  const int ey = (int)std::min(Y3, (double)H);

  /* https://codeplea.com/triangular-interpolation */
  const double invDn = 1.0 / dn;
  // WV1/WV2 are linear in x: constant per-pixel step along a row.
  const double dWV1dx = (yv2 - yv3) * invDn;
  const double dWV2dx = (yv3 - yv1) * invDn;
  const double dDxPerPixel = tUp ? dWV1dx : -dWV1dx;
  const double dDyPerPixel = tUp ? dWV2dx : dWV1dx + dWV2dx;

  for (int y = sy; y < ey; y++) {
    const double x_longside = r_longside * (y - Y1) + X1;
    const double x_other = y < Y2 ? (r_upper * (y - Y1) + X1) : (r_lower * (y - Y2) + X2);
    const double minx = std::min(x_longside, x_other);
    const double maxx = std::max(x_longside, x_other);
    // Bias keeps both triangles on the same side when an edge runs exactly through a pixel center.
    constexpr double centerBias = 0.5 + 1e-7;
    const double startD = minx + centerBias;
    const double endD = maxx + centerBias;
    if (endD < 1) continue;
    const int startPixel = startD < 0 ? 0 : (startD >= W ? W : gdwFloorNonNegative(startD));
    const int endPixel = endD >= W ? W : gdwFloorNonNegative(endD);
    if (startPixel >= endPixel) continue;
    // Sampled at the fractional span-start position; clamped below since this can land up to half a pixel outside the triangle.
    const double startX = startPixel + (minx < 0 ? 0 : minx - (double)gdwFloorNonNegative(minx));
    const double WV1 = ((yv2 - yv3) * (startX - xv3) + (xv3 - xv2) * (y - yv3)) * invDn;
    const double WV2 = ((yv3 - yv1) * (startX - xv3) + (xv1 - xv3) * (y - yv3)) * invDn;
    double tileDx = tUp ? WV1 : 1 - WV1;
    double tileDy = tUp ? WV2 : WV1 + WV2;
    for (int x = startPixel; x < endPixel; x++, tileDx += dDxPerPixel, tileDy += dDyPerPixel) {
      warperState.sourceTileDx = tileDx < 0 ? 0 : (tileDx > 1 ? 1 : tileDx);
      warperState.sourceTileDy = tileDy < 0 ? 0 : (tileDy > 1 ? 1 : tileDy);
      drawFunction(x, y, value, warperState);
    }
  }
  return 0;
}

// Triangle with unrounded destination pixel coordinates.
template <typename T, typename DrawFn>
int gdwDrawTriangle(const double triangleXCoords[3], const double triangleYCoords[3], const T &value, bool tUp, GDWState &warperState, const DrawFn &drawFunction) {
  const int W = warperState.destGridWidth;
  const int H = warperState.destGridHeight;
  if (triangleXCoords[0] < 0 && triangleXCoords[1] < 0 && triangleXCoords[2] < 0) return 0;
  if (triangleXCoords[0] >= W && triangleXCoords[1] >= W && triangleXCoords[2] >= W) return 0;
  if (triangleYCoords[0] < 0 && triangleYCoords[1] < 0 && triangleYCoords[2] < 0) return 0;
  if (triangleYCoords[0] >= H && triangleYCoords[1] >= H && triangleYCoords[2] >= H) return 0;
  const double xP[3] = {gdwRoundHalfAwayFromZero(triangleXCoords[0]), gdwRoundHalfAwayFromZero(triangleXCoords[1]), gdwRoundHalfAwayFromZero(triangleXCoords[2])};
  const double yP[3] = {gdwRoundHalfAwayFromZero(triangleYCoords[0]), gdwRoundHalfAwayFromZero(triangleYCoords[1]), gdwRoundHalfAwayFromZero(triangleYCoords[2])};
  return gdwDrawTriangleRounded(xP, yP, value, tUp, warperState, drawFunction);
}

// Quad as two triangles, corners ordered q[0] q[1] / q[3] q[2]. Rejects early and rounds each corner once (not once per triangle).
template <typename T, typename DrawFn> void gdwDrawQuad(const double quadX[4], const double quadY[4], const T &value, GDWState &warperState, const DrawFn &drawFunction) {
  const int W = warperState.destGridWidth;
  const int H = warperState.destGridHeight;
  if (quadX[0] < 0 && quadX[1] < 0 && quadX[2] < 0 && quadX[3] < 0) return;
  if (quadX[0] >= W && quadX[1] >= W && quadX[2] >= W && quadX[3] >= W) return;
  if (quadY[0] < 0 && quadY[1] < 0 && quadY[2] < 0 && quadY[3] < 0) return;
  if (quadY[0] >= H && quadY[1] >= H && quadY[2] >= H && quadY[3] >= H) return;

  const double yP[4] = {gdwRoundHalfAwayFromZero(quadY[0]), gdwRoundHalfAwayFromZero(quadY[1]), gdwRoundHalfAwayFromZero(quadY[2]), gdwRoundHalfAwayFromZero(quadY[3])};
  // Sub-pixel height quad: both triangles would be too, skip.
  if (std::max(std::max(yP[0], yP[1]), std::max(yP[2], yP[3])) - std::min(std::min(yP[0], yP[1]), std::min(yP[2], yP[3])) < 1) return;
  const double xP[4] = {gdwRoundHalfAwayFromZero(quadX[0]), gdwRoundHalfAwayFromZero(quadX[1]), gdwRoundHalfAwayFromZero(quadX[2]), gdwRoundHalfAwayFromZero(quadX[3])};

  const double xA[3] = {xP[0], xP[1], xP[2]}, yA[3] = {yP[0], yP[1], yP[2]};
  const double xB[3] = {xP[2], xP[0], xP[3]}, yB[3] = {yP[2], yP[0], yP[3]};
  gdwDrawTriangleRounded(xA, yA, value, false, warperState, drawFunction);
  gdwDrawTriangleRounded(xB, yB, value, true, warperState, drawFunction);
}

#endif
