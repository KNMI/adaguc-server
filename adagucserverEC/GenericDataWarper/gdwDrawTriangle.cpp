#include "CGenericDataWarper.h"

template <typename T> int gdwDrawTriangle(double *_xP, double *_yP, T value, bool aOrB, GDWState &warperState, const std::function<void(int, int, T, GDWState &warperState)> &drawFunction) {
  int W = warperState.destDataWidth;
  int H = warperState.destDataHeight;
  if (_xP[0] < 0 && _xP[1] < 0 && _xP[2] < 0) return 0;
  if (_xP[0] >= W && _xP[1] >= W && _xP[2] >= W) return 0;
  if (_yP[0] < 0 && _yP[1] < 0 && _yP[2] < 0) return 0;
  if (_yP[0] >= H && _yP[1] >= H && _yP[2] >= H) return 0;

  int lower = -1;
  int middle = -1;
  int upper = -1;

  double xP[3] = {round(_xP[0]), round(_xP[1]), round(_xP[2])};
  double yP[3] = {round(_yP[0]), round(_yP[1]), round(_yP[2])};

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

  bool bOrA = !aOrB;

  double screenW = W;
  double screenH = H;

  double xlength = X3 - X1;
  double ylength_A = Y2 - Y1;
  double ylength_B = Y3 - Y2;
  double xlength_A = X2 - X1;
  double xlength_B = X3 - X2;
  double r_longside = xlength / ylength;

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

  double r_lower = ylength_B > 0 ? xlength_B / ylength_B : 0;
  double dn = ((yv2 - yv3) * (xv1 - xv3) + (xv3 - xv2) * (yv1 - yv3));
  for (double y = sy; y < ey; y++) {
    double x_longside = r_longside * (y - Y1) + X1;
    double sx = x_longside;
    double ex = y < Y2 ? (r_upper * (y - Y1) + X1) : (r_lower * (y - Y2) + X2);
    double minx = sx > ex ? ex : sx;
    double maxx = sx > ex ? sx : ex;
    for (double x = minx < 0 ? 0 : minx; x < maxx && x < screenW; x++) {
      double WV1 = ((yv2 - yv3) * (x - xv3) + (xv3 - xv2) * (y - yv3)) / dn;
      double WV2 = ((yv3 - yv1) * (x - xv3) + (xv1 - xv3) * (y - yv3)) / dn;
      double WV3 = 1 - WV1 - WV2;
      warperState.tileDx = WV1 * aOrB + WV2 * bOrA + WV3 * bOrA;
      warperState.tileDy = WV1 * bOrA + WV2 * 1 + WV3 * 0;
      warperState.destX = x;
      warperState.destY = y;
      drawFunction(x, y, value, warperState);
    }
  }
  return 0;
}

template int gdwDrawTriangle<char>(double *_xP, double *_yP, char value, bool aOrB, GDWState &warperState, const std::function<void(int, int, char, GDWState &warperState)> &drawFunction);
template int gdwDrawTriangle<uchar>(double *_xP, double *_yP, uchar value, bool aOrB, GDWState &warperState, const std::function<void(int, int, uchar, GDWState &warperState)> &drawFunction);
template int gdwDrawTriangle<short>(double *_xP, double *_yP, short value, bool aOrB, GDWState &warperState, const std::function<void(int, int, short, GDWState &warperState)> &drawFunction);
template int gdwDrawTriangle<ushort>(double *_xP, double *_yP, ushort value, bool aOrB, GDWState &warperState, const std::function<void(int, int, ushort, GDWState &warperState)> &drawFunction);
template int gdwDrawTriangle<int>(double *_xP, double *_yP, int value, bool aOrB, GDWState &warperState, const std::function<void(int, int, int, GDWState &warperState)> &drawFunction);
template int gdwDrawTriangle<uint>(double *_xP, double *_yP, uint value, bool aOrB, GDWState &warperState, const std::function<void(int, int, uint, GDWState &warperState)> &drawFunction);
template int gdwDrawTriangle<long>(double *_xP, double *_yP, long value, bool aOrB, GDWState &warperState, const std::function<void(int, int, long, GDWState &warperState)> &drawFunction);
template int gdwDrawTriangle<ulong>(double *_xP, double *_yP, ulong value, bool aOrB, GDWState &warperState, const std::function<void(int, int, ulong, GDWState &warperState)> &drawFunction);
template int gdwDrawTriangle<float>(double *_xP, double *_yP, float value, bool aOrB, GDWState &warperState, const std::function<void(int, int, float, GDWState &warperState)> &drawFunction);
template int gdwDrawTriangle<double>(double *_xP, double *_yP, double value, bool aOrB, GDWState &warperState, const std::function<void(int, int, double, GDWState &warperState)> &drawFunction);
