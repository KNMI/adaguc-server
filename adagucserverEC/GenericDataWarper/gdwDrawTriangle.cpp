#include "GenericDataWarper/CGenericDataWarper.h"

template <typename T>
int gdwDrawTriangle(int *xP, int *yP, T value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, T, void *settings, void *genericDataWarper), void *genericDataWarper,
                    bool aOrB) {
  CGenericDataWarper *g = (CGenericDataWarper *)genericDataWarper;
  int W = destWidth;
  int H = destHeight;
  if (xP[0] < 0 && xP[1] < 0 && xP[2] < 0) return 0;
  if (xP[0] >= W && xP[1] >= W && xP[2] >= W) return 0;
  if (yP[0] < 0 && yP[1] < 0 && yP[2] < 0) return 0;
  if (yP[0] >= H && yP[1] >= H && yP[2] >= H) return 0;

  unsigned int lower;
  unsigned int middle;
  unsigned int upper;

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

  int X1 = xP[lower];
  int X2 = xP[middle];
  int X3 = xP[upper];
  int Y1 = yP[lower];
  int Y2 = yP[middle];
  int Y3 = yP[upper];

  double vX1 = aOrB ? 1 : 0;
  double vY1 = aOrB ? 0 : 1;
  double vX2 = aOrB ? 0 : 1;
  double vY2 = 1;
  double vX3 = aOrB ? 0 : 1;
  double vY3 = 0;

  // return 1;
  /*
  //  1
  //   \
  //    2
  //   /
  //  3
  */
  // If top point is equal to bottom point of triangle, skip
  // If middle is equal to top and middle is equal to bottom, skip
  if ((Y1 == Y3) || (Y2 == Y1 && Y3 == Y2)) {
    int minx = X1;
    if (minx > X2) minx = X2;
    if (minx > X3) minx = X3;
    int maxx = X1;
    if (maxx < X2) maxx = X2;
    if (maxx < X3) maxx = X3;
    g->warperState.tileDy = 0;
    for (int x = minx; x < maxx + 1; x++) {
      g->warperState.tileDx = 0; //(x - minx) / double(maxx-minx);
      drawFunction(x, yP[2], value, settings, genericDataWarper);
    }
    return 1;
  }

  /* https://codeplea.com/triangular-interpolation */
  double dn = ((yP[1] - yP[2]) * (xP[0] - xP[2]) + (xP[2] - xP[1]) * (yP[0] - yP[2]));

  double rcl = double(X3 - X1) / double(Y3 - Y1);
  if (Y2 != Y1 && Y1 < H && Y2 > 0) {
    double rca = double(X2 - X1) / double(Y2 - Y1);
    int sy = (Y1 < 0) ? 0 : Y1;
    int ey = (Y2 > H) ? H : Y2;
    for (int y = sy; y <= ey - 1; y++) {
      int xL = floor(rcl * double(y - Y1) + X1);
      int xA = floor(rca * double(y - Y1) + X1);
      int x1, x2;
      if (xL < xA) {
        x1 = xL;
        x2 = xA;
      } else {
        x2 = xL;
        x1 = xA;
      }
      if (x1 < W && x2 > 0) {
        int sx = (x1 < 0) ? 0 : x1;
        int ex = (x2 > W) ? W : x2;
        for (int x = sx; x <= ex - 1; x++) {
          double WV1 = ((yP[1] - yP[2]) * (x - xP[2]) + (xP[2] - xP[1]) * (y - yP[2])) / dn;
          double WV2 = ((yP[2] - yP[0]) * (x - xP[2]) + (xP[0] - xP[2]) * (y - yP[2])) / dn;
          double WV3 = 1 - WV1 - WV2;

          g->warperState.tileDx = WV1 * vX1 + WV2 * vX2 + WV3 * vX3;
          g->warperState.tileDy = WV1 * vY1 + WV2 * vY2 + WV3 * vY3;
          drawFunction(x, y, value, settings, genericDataWarper);
        }
      }
    }
  }
  // return 0;
  if (Y3 != Y2 && Y2 < H && Y3 > 0) {
    double rcb = double(X3 - X2) / double(Y3 - Y2);
    int sy = (Y2 < 0) ? 0 : Y2;
    int ey = (Y3 > H) ? H : Y3;
    for (int y = sy; y <= ey - 1; y++) {
      int xL = (rcl * double(y - Y1) + X1); // floor
      int xB = (rcb * double(y - Y2) + X2); // floor
      int x1, x2;
      if (xL <= xB) {
        x1 = xL;
        x2 = xB;
      } else {
        x2 = xL;
        x1 = xB;
      }
      if (x1 < W && x2 > 0) {
        int sx = (x1 < 0) ? 0 : x1;
        int ex = (x2 > W) ? W : x2;
        for (int x = sx; x < ex; x++) {
          double WV1 = ((yP[1] - yP[2]) * (x - xP[2]) + (xP[2] - xP[1]) * (y - yP[2])) / dn;
          double WV2 = ((yP[2] - yP[0]) * (x - xP[2]) + (xP[0] - xP[2]) * (y - yP[2])) / dn;
          double WV3 = 1 - WV1 - WV2;

          g->warperState.tileDx = WV1 * vX1 + WV2 * vX2 + WV3 * vX3;
          g->warperState.tileDy = WV1 * vY1 + WV2 * vY2 + WV3 * vY3;
          // http://localhost:8080/adaguc-server?DATASET=noaaglm&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=LIGHTNING_COUNTS&WIDTH=1321&HEIGHT=959&CRS=EPSG%3A32661&BBOX=-8314748.562870221,-1301096.785244672,-8149138.113029413,-1180869.3655646304&STYLES=counts_knmi%2Fgeneric&FORMAT=image/png&TRANSPARENT=TRUE&&time=2024-06-28T09%3A00%3A00Z&0.013241703752992606
          drawFunction(x, y, value, settings, genericDataWarper);
        }
      }
    }
  }
  return 0;
}

template int gdwDrawTriangle<char>(int *xP, int *yP, char value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, char, void *settings, void *genericDataWarper),
                                   void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<unsigned char>(int *xP, int *yP, unsigned char value, int destWidth, int destHeight, void *settings,
                                            void (*drawFunction)(int, int, unsigned char, void *settings, void *genericDataWarper), void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<short>(int *xP, int *yP, short value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, short, void *settings, void *genericDataWarper),
                                    void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<ushort>(int *xP, int *yP, ushort value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, ushort, void *settings, void *genericDataWarper),
                                     void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<int>(int *xP, int *yP, int value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, int, void *settings, void *genericDataWarper),
                                  void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<uint>(int *xP, int *yP, uint value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, uint, void *settings, void *genericDataWarper),
                                   void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<long>(int *xP, int *yP, long value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, long, void *settings, void *genericDataWarper),
                                   void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<unsigned long>(int *xP, int *yP, unsigned long value, int destWidth, int destHeight, void *settings,
                                            void (*drawFunction)(int, int, unsigned long, void *settings, void *genericDataWarper), void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<float>(int *xP, int *yP, float value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, float, void *settings, void *genericDataWarper),
                                    void *genericDataWarper, bool aOrB);
template int gdwDrawTriangle<double>(int *xP, int *yP, double value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, double, void *settings, void *genericDataWarper),
                                     void *genericDataWarper, bool aOrB);
