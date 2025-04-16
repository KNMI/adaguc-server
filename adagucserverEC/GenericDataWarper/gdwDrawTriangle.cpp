#include "GenericDataWarper/CGenericDataWarper.h"

typedef double gdwfloat;
typedef int gdwint;
typedef short gdwshort;

template <typename T>
int gdwDrawTriangle(double *_xP, double *_yP, T value, int W, int H, void *settings, void (*drawFunction)(int, int, T, void *settings, void *genericDataWarper), void *genericDataWarper, bool aOrB) {
  CGenericDataWarper *g = (CGenericDataWarper *)genericDataWarper;

  if (_xP[0] < 0 && _xP[1] < 0 && _xP[2] < 0) return 0;
  if (_xP[0] >= W && _xP[1] >= W && _xP[2] >= W) return 0;
  if (_yP[0] < 0 && _yP[1] < 0 && _yP[2] < 0) return 0;
  if (_yP[0] >= H && _yP[1] >= H && _yP[2] >= H) return 0;

  gdwshort lower = -1;
  gdwshort middle = -1;
  gdwshort upper = -1;

  gdwfloat xP[3] = {round(_xP[0]), round(_xP[1]), round(_xP[2])};
  gdwfloat yP[3] = {round(_yP[0]), round(_yP[1]), round(_yP[2])};

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

  gdwfloat Y1 = yP[lower];
  gdwfloat Y3 = yP[upper];
  gdwfloat ylength = Y3 - Y1;
  // The triangle is less then a pixel heigh, it has no area so ignore.
  if (Y1 >= H || ylength < 1) {
    return 0;
  }
  gdwfloat Y2 = yP[middle];
  gdwfloat X1 = xP[lower];
  gdwfloat X2 = xP[middle];
  gdwfloat X3 = xP[upper];

  gdwfloat xv1 = xP[0];
  gdwfloat xv2 = xP[1];
  gdwfloat xv3 = xP[2];
  gdwfloat yv1 = yP[0];
  gdwfloat yv2 = yP[1];
  gdwfloat yv3 = yP[2];

  bool bOrA = !aOrB;

  gdwfloat screenW = W;
  gdwfloat screenH = H;

  gdwfloat xlength = X3 - X1;
  gdwfloat ylength_A = Y2 - Y1;
  gdwfloat ylength_B = Y3 - Y2;
  gdwfloat xlength_A = X2 - X1;
  gdwfloat xlength_B = X3 - X2;
  gdwfloat r_longside = xlength / ylength;

  // clip startY and endY to the screen
  gdwfloat sy = Y1 < 0 ? 0 : Y1;
  gdwfloat ey = Y3 > screenH ? screenH : Y3;

  /* https://codeplea.com/triangular-interpolation */

  // If triangle partA has no length, directly go to second part.
  gdwfloat r_upper = 0;
  if (ylength_A == 0) {
    sy = Y2 < 0 ? 0 : Y2;
  } else {
    r_upper = xlength_A / ylength_A;
  }

  gdwfloat r_lower = ylength_B > 0 ? xlength_B / ylength_B : 0;
  gdwfloat dn = ((yv2 - yv3) * (xv1 - xv3) + (xv3 - xv2) * (yv1 - yv3));
  for (gdwfloat y = sy; y < ey; y++) {
    gdwfloat x_longside = r_longside * (y - Y1) + X1;
    gdwfloat sx = x_longside;
    gdwfloat ex = y < Y2 ? (r_upper * (y - Y1) + X1) : (r_lower * (y - Y2) + X2);
    gdwfloat minx = sx > ex ? ex : sx;
    gdwfloat maxx = sx > ex ? sx : ex;
    for (gdwfloat x = minx < 0 ? 0 : minx; x < maxx && x < screenW; x++) {
      gdwfloat WV1 = ((yv2 - yv3) * (x - xv3) + (xv3 - xv2) * (y - yv3)) / dn;
      gdwfloat WV2 = ((yv3 - yv1) * (x - xv3) + (xv1 - xv3) * (y - yv3)) / dn;
      gdwfloat WV3 = 1 - WV1 - WV2;
      g->warperState.tileDx = WV1 * aOrB + WV2 * bOrA + WV3 * bOrA;
      g->warperState.tileDy = WV1 * bOrA + WV2 * 1 + WV3 * 0;
      drawFunction(x, y, value, settings, genericDataWarper);
    }
  }
  return 0;
}

template int gdwDrawTriangle<char>(double *xP, double *yP, char value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, char, void *settings, void *genericDataWarper),
                                   void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<unsigned char>(double *xP, double *yP, unsigned char value, int destWidth, int destHeight, void *settings,
                                            void (*drawFunction)(int, int, unsigned char, void *settings, void *genericDataWarper), void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<short>(double *xP, double *yP, short value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, short, void *settings, void *genericDataWarper),
                                    void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<unsigned short>(double *xP, double *yP, unsigned short value, int destWidth, int destHeight, void *settings,
                                             void (*drawFunction)(int, int, unsigned short, void *settings, void *genericDataWarper), void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<int>(double *xP, double *yP, int value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, int, void *settings, void *genericDataWarper),
                                  void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<uint>(double *xP, double *yP, uint value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, uint, void *settings, void *genericDataWarper),
                                   void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<long>(double *xP, double *yP, long value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, long, void *settings, void *genericDataWarper),
                                   void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<unsigned long>(double *xP, double *yP, unsigned long value, int destWidth, int destHeight, void *settings,
                                            void (*drawFunction)(int, int, unsigned long, void *settings, void *genericDataWarper), void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<float>(double *xP, double *yP, float value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, float, void *settings, void *genericDataWarper),
                                    void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<double>(double *xP, double *yP, double value, int destWidth, int destHeight, void *settings,
                                     void (*drawFunction)(int, int, double, void *settings, void *genericDataWarper), void *genericDataWarper, bool aOrB);
