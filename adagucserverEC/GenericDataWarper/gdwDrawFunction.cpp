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
    int sourceDataPX = genericDataWarper->warperState.sourceDataPX;
    int sourceDataPY = genericDataWarper->warperState.sourceDataPY;
    int sourceDataWidth = genericDataWarper->warperState.sourceDataWidth;
    int sourceDataHeight = genericDataWarper->warperState.sourceDataHeight;

    if (sourceDataPX < 0 || sourceDataPY < 0 || sourceDataPY > sourceDataHeight - 1 || sourceDataPX > sourceDataWidth - 1) return;

    bool bilinear = drawSettings->drawInImage == DrawInImageBilinear || drawSettings->drawInDataGrid == DrawInDataGridBilinear;
    bool nearest = drawSettings->drawInImage == DrawInImageNearest || drawSettings->drawInDataGrid == DrawInDataGridNearest;

    if (x >= 0 && y >= 0 && x < drawSettings->drawImage->Geo->dWidth && y < drawSettings->drawImage->Geo->dHeight) {

      if (drawSettings->smoothingFiter == 0) {

        // Bilinear
        if (bilinear && sourceDataPY <= sourceDataHeight - 2 && sourceDataPX <= sourceDataWidth - 2) {
          int xL = (sourceDataPX + 0);
          int yT = (sourceDataPY + 0);
          int xR = (sourceDataPX + 1);
          int yB = (sourceDataPY + 1);
          double values[2][2];
          values[0][0] = (double)((T *)sourceData)[xL + yT * sourceDataWidth];
          values[1][0] = (double)((T *)sourceData)[xR + yT * sourceDataWidth];
          values[0][1] = (double)((T *)sourceData)[xL + yB * sourceDataWidth];
          values[1][1] = (double)((T *)sourceData)[xR + yB * sourceDataWidth];

          double dx = genericDataWarper->warperState.tileDx;
          double dy = genericDataWarper->warperState.tileDy;
          double gx1 = (1 - dx) * values[0][0] + dx * values[1][0];
          double gx2 = (1 - dx) * values[0][1] + dx * values[1][1];
          double billValue = (1 - dy) * gx1 + dy * gx2;
          // Draw in drawImage pixels, interpolated with bilinear method
          if (drawSettings->drawInImage == DrawInImageBilinear) {
            setPixelInDrawImage(x, y, billValue, drawSettings);
          }

          if (drawSettings->drawInDataGrid == DrawInDataGridBilinear && genericDataWarper->warperState.destinationGrid != nullptr) {
            genericDataWarper->warperState.destinationGrid[x + y * drawSettings->drawImage->Geo->dWidth] = billValue;
          }
        }

        // Nearest
        if (nearest) {
          int xL = (sourceDataPX + 0);
          int yT = (sourceDataPY + 0);
          double value = ((T *)sourceData)[xL + yT * sourceDataWidth];

          // Draw in drawImage pixels, interpolated with nearest method
          if (drawSettings->drawInImage == DrawInImageNearest) {
            setPixelInDrawImage(x, y, value, drawSettings);
          }

          if (drawSettings->drawInDataGrid == DrawInDataGridNearest && genericDataWarper->warperState.destinationGrid != nullptr) {
            genericDataWarper->warperState.destinationGrid[x + y * drawSettings->drawImage->Geo->dWidth] = value;
          }
        }

      } else {
        // values[0][0] = smoothingAtLocation((float *)sourceData, drawSettings->smoothingDistanceMatrix, drawSettings->smoothingFiter, (float)drawSettings->dfNodataValue, sourceDataPX, sourceDataPY,
        //                                    sourceDataWidth, sourceDataHeight);
        // values[1][0] = smoothingAtLocation((float *)sourceData, drawSettings->smoothingDistanceMatrix, drawSettings->smoothingFiter, (float)drawSettings->dfNodataValue, sourceDataPX + 1,
        // sourceDataPY,
        //                                    sourceDataWidth, sourceDataHeight);
        // values[0][1] = smoothingAtLocation((float *)sourceData, drawSettings->smoothingDistanceMatrix, drawSettings->smoothingFiter, (float)drawSettings->dfNodataValue, sourceDataPX, sourceDataPY +
        // 1,
        //                                    sourceDataWidth, sourceDataHeight);
        // values[1][1] = smoothingAtLocation((float *)sourceData, drawSettings->smoothingDistanceMatrix, drawSettings->smoothingFiter, (float)drawSettings->dfNodataValue, sourceDataPX + 1,
        //                                    sourceDataPY + 1, sourceDataWidth, sourceDataHeight);
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
