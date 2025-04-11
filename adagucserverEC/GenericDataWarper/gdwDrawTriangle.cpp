#include "GenericDataWarper/CGenericDataWarper.h"

typedef double gdwfloat;
typedef int gdwint;
typedef short gdwshort;

template <typename T>
int gdwDrawTriangle(double *xP, double *yP, T value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, T, void *settings, void *genericDataWarper), void *genericDataWarper,
                    bool aOrB) {
  CGenericDataWarper *g = (CGenericDataWarper *)genericDataWarper;
  gdwint W = destWidth;
  gdwint H = destHeight;
  if (xP[0] < 0 && xP[1] < 0 && xP[2] < 0) return 0;
  if (xP[0] >= W && xP[1] >= W && xP[2] >= W) return 0;
  if (yP[0] < 0 && yP[1] < 0 && yP[2] < 0) return 0;
  if (yP[0] >= H && yP[1] >= H && yP[2] >= H) return 0;

  gdwshort lower = -1;
  gdwshort middle = -1;
  gdwshort upper = -1;

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

  // CDBDebug("lmu: %d %d %d", lower, middle, upper);

  // CDBDebug("lmu: %f %f %f", yP[0], yP[1], yP[2]);

  // CDBDebug("lmu: %f %f %f", xP[0], xP[1], xP[2]);

  gdwfloat X1 = round(xP[lower]);
  gdwfloat X2 = round(xP[middle]);
  gdwfloat X3 = round(xP[upper]);
  gdwfloat Y1 = round(yP[lower]);
  gdwfloat Y2 = round(yP[middle]);
  gdwfloat Y3 = round(yP[upper]);

  gdwfloat xv1 = round(xP[0]);
  gdwfloat xv2 = round(xP[1]);
  gdwfloat xv3 = round(xP[2]);
  gdwfloat yv1 = round(yP[0]);
  gdwfloat yv2 = round(yP[1]);
  gdwfloat yv3 = round(yP[2]);

  gdwfloat vX1 = aOrB ? 1 : 0;
  gdwfloat vY1 = aOrB ? 0 : 1;
  gdwfloat vX2 = aOrB ? 0 : 1;
  gdwfloat vY2 = 1;
  gdwfloat vX3 = aOrB ? 0 : 1;
  gdwfloat vY3 = 0;

  // return 1;
  /*
  //  1
  //   \
  //    2
  //   /
  //  3
  */

  gdwfloat ylength = Y3 - Y1;
  gdwfloat screen0 = 0;
  gdwfloat screenW = W;
  gdwfloat screenH = H;

  // The triangle is less then a pixel heigh, just draw a line.
  if (ylength < 1) {
    gdwfloat minx = std::max(std::min({X1, X2, X3}), screen0);
    gdwfloat maxx = std::min(std::max({X1, X2, X3}), screenW);
    g->warperState.tileDy = 0;
    for (gdwfloat x = minx; x < maxx + 1; x++) {
      g->warperState.tileDx = 0;
      drawFunction(round(x), (Y1), value, settings, genericDataWarper);
    }
    return 0;
  }

  gdwfloat xlength = X3 - X1;
  gdwfloat ylenght2_1 = Y2 - Y1;
  gdwfloat ylenght3_2 = Y3 - Y2;
  gdwfloat xlenght2_1 = X2 - X1;
  gdwfloat xlenght3_2 = X3 - X2;
  gdwfloat rcl = xlength / ylength;
  gdwfloat rca = xlenght2_1 / ylenght2_1;
  gdwfloat rcb = xlenght3_2 / ylenght3_2;
  gdwfloat y;
  gdwfloat sy = std::max(screen0, (Y1));
  gdwfloat ey = std::min(screenH, (Y3));

  /* https://codeplea.com/triangular-interpolation */
  gdwfloat dn = ((yv2 - yv3) * (xv1 - xv3) + (xv3 - xv2) * (yv1 - yv3));
  // Draw triangle part Y1 to Y2. Clip on drawing canvas area
  y = sy;
  while (y < ey + 1) {
    gdwfloat roundY = (y);
    if (roundY >= screenH) {
      break;
    }

    gdwfloat xL = rcl * (roundY - (Y1)) + X1;
    gdwfloat xA = rca * (roundY - (Y1)) + X1;
    gdwfloat xB = rcb * (roundY - (Y2)) + X2;
    // xA = xL;
    // xB = xL;
    gdwfloat sx = (y <= Y2) ? std::max(std::min(xL, xA), screen0) : std::max(std::min(xL, xB), screen0);
    gdwfloat ex = (y <= Y2) ? std::min(std::max(xL, xA), screenW) : std::min(std::max(xL, xB), screenW);
    for (gdwfloat x = (sx); x < ex + 1; x++) {
      gdwfloat roundX = round(x);
      if (roundX >= screenW) {
        break;
      }
      gdwfloat WV1 = ((yv2 - yv3) * (roundX - xv3) + (xv3 - xv2) * (roundY - yv3)) / dn;
      gdwfloat WV2 = ((yv3 - yv1) * (roundX - xv3) + (xv1 - xv3) * (roundY - yv3)) / dn;
      gdwfloat WV3 = 1 - WV1 - WV2;
      g->warperState.tileDx = WV1 * vX1 + WV2 * vX2 + WV3 * vX3;
      g->warperState.tileDy = WV1 * vY1 + WV2 * vY2 + WV3 * vY3;
      if (g->warperState.tileDy >= 0.0 && g->warperState.tileDy <= 1.0 && g->warperState.tileDx >= 0.0 && g->warperState.tileDx <= 1.0) {
        drawFunction(roundX, roundY, value, settings, genericDataWarper);
      }
    }
    y++;
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
