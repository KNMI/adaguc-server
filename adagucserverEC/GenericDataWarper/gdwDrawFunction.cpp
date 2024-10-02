#include "GenericDataWarper/gdwDrawFunction.h"
#include "CImageOperators/smoothRasterField.h"
#include "CDrawFunction.h"
#include <sys/types.h>

static inline int nfast_mod(const int input, const int ceil) { return input >= ceil ? input % ceil : input; }

template <typename T> void gdwDrawFunction(int x, int y, T val, void *_settings, void *g) {
  CDrawFunctionSettings *drawSettings = static_cast<CDrawFunctionSettings *>(_settings);
  if (x < 0 || y < 0 || x > drawSettings->drawImage->Geo->dWidth || y > drawSettings->drawImage->Geo->dHeight) return;
  CGenericDataWarper *genericDataWarper = static_cast<CGenericDataWarper *>(g);
  bool isNodata = false;
  if (drawSettings->hasNodataValue) {
    if ((val) == (T)drawSettings->dfNodataValue) isNodata = true;
  }
  if (!(val == val)) isNodata = true;
  if (!isNodata) {
    T *sourceData = (T *)genericDataWarper->warperState.sourceData;
    size_t sourceDataPX = genericDataWarper->warperState.sourceDataPX;
    size_t sourceDataPY = genericDataWarper->warperState.sourceDataPY;
    size_t sourceDataWidth = genericDataWarper->warperState.sourceDataWidth;
    size_t sourceDataHeight = genericDataWarper->warperState.sourceDataHeight;

    if (sourceDataPY > sourceDataHeight - 1) return;
    if (sourceDataPX > sourceDataWidth - 1) return;
    if (x >= 0 && y >= 0 && x < (int)drawSettings->drawImage->Geo->dWidth && y < (int)drawSettings->drawImage->Geo->dHeight) {
      double values[2][2] = {{0, 0}, {0, 0}};
      if (drawSettings->smoothingFiter == 0) {

        values[0][0] += ((T *)sourceData)[nfast_mod(sourceDataPX + 0, sourceDataWidth) + nfast_mod(sourceDataPY + 0, sourceDataHeight) * sourceDataWidth];

        // setPixelInDrawImage(x, y, val, drawSettings);
        values[1][0] += ((T *)sourceData)[nfast_mod(sourceDataPX + 1, sourceDataWidth) + nfast_mod(sourceDataPY + 0, sourceDataHeight) * sourceDataWidth];
        values[0][1] += ((T *)sourceData)[nfast_mod(sourceDataPX + 0, sourceDataWidth) + nfast_mod(sourceDataPY + 1, sourceDataHeight) * sourceDataWidth];
        values[1][1] += ((T *)sourceData)[nfast_mod(sourceDataPX + 1, sourceDataWidth) + nfast_mod(sourceDataPY + 1, sourceDataHeight) * sourceDataWidth];
      } else {
        values[0][0] = smoothingAtLocation((float *)sourceData, drawSettings->smoothingDistanceMatrix, drawSettings->smoothingFiter, (float)drawSettings->dfNodataValue, sourceDataPX, sourceDataPY,
                                           sourceDataWidth, sourceDataHeight);
        values[1][0] = smoothingAtLocation((float *)sourceData, drawSettings->smoothingDistanceMatrix, drawSettings->smoothingFiter, (float)drawSettings->dfNodataValue, sourceDataPX + 1, sourceDataPY,
                                           sourceDataWidth, sourceDataHeight);
        values[0][1] = smoothingAtLocation((float *)sourceData, drawSettings->smoothingDistanceMatrix, drawSettings->smoothingFiter, (float)drawSettings->dfNodataValue, sourceDataPX, sourceDataPY + 1,
                                           sourceDataWidth, sourceDataHeight);
        values[1][1] = smoothingAtLocation((float *)sourceData, drawSettings->smoothingDistanceMatrix, drawSettings->smoothingFiter, (float)drawSettings->dfNodataValue, sourceDataPX + 1,
                                           sourceDataPY + 1, sourceDataWidth, sourceDataHeight);
      }

      double dx = genericDataWarper->warperState.tileDx;
      double dy = genericDataWarper->warperState.tileDy;

      double gx1 = (1 - dx) * values[0][0] + dx * values[1][0];
      double gx2 = (1 - dx) * values[0][1] + dx * values[1][1];
      double bilValue = (1 - dy) * gx1 + dy * gx2;
      if (dy >= 0.0 && dy < 1.0) {
        if (dx >= 0.0 && dx < 1.0) {
          setPixelInDrawImage(x, y, bilValue, drawSettings);
        }
      }
    }
  }
};

template void gdwDrawFunction<char>(int x, int y, char val, void *_settings, void *g);
template void gdwDrawFunction<unsigned char>(int x, int y, unsigned char val, void *_settings, void *g);
template void gdwDrawFunction<short>(int x, int y, short val, void *_settings, void *g);
template void gdwDrawFunction<ushort>(int x, int y, ushort val, void *_settings, void *g);
template void gdwDrawFunction<int>(int x, int y, int val, void *_settings, void *g);
template void gdwDrawFunction<uint>(int x, int y, uint val, void *_settings, void *g);
template void gdwDrawFunction<long>(int x, int y, long val, void *_settings, void *g);
template void gdwDrawFunction<unsigned long>(int x, int y, unsigned long val, void *_settings, void *g);
template void gdwDrawFunction<float>(int x, int y, float val, void *_settings, void *g);
template void gdwDrawFunction<double>(int x, int y, double val, void *_settings, void *g);
