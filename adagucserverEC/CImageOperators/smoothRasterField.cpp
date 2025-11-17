#include "smoothRasterField.h"
#include <cstddef>
#include <math.h>
#include <CGenericDataWarper.h>
#include <GenericDataWarper/GDWDrawFunctionSettings.h>

#define MEMO_NODATAVALUE -99999999999999.f

void smoothingMakeDistanceMatrix(GDWDrawFunctionSettings &settings) {

  int smoothWindowSize = settings.smoothingFiter;
  if (smoothWindowSize == 0) {
    return;
  }
  settings.smoothingDistanceMatrix = new double[(smoothWindowSize + 1) * 2 * (smoothWindowSize + 1) * 2];

  double distanceAmmount = 0;
  int dWinP = 0;
  for (int y1 = -smoothWindowSize; y1 < smoothWindowSize + 1; y1++) {
    for (int x1 = -smoothWindowSize; x1 < smoothWindowSize + 1; x1++) {
      double d = sqrt(x1 * x1 + y1 * y1);
      d = 1 / (d + 1);
      settings.smoothingDistanceMatrix[dWinP++] = d;
      distanceAmmount += d;
    }
  }
}

template <typename T> T smoothingAtLocation(int sourceX, int sourceY, T *sourceGrid, GDWState &warperState, GDWDrawFunctionSettings &settings) {
  if (settings.smoothingFiter == 0 || settings.smoothingDistanceMatrix == nullptr) {
    return (T)settings.dfNodataValue;
  }
  if (sourceX < 0 || sourceY < 0 || sourceX >= warperState.sourceGridWidth || sourceY >= warperState.sourceGridHeight) return (T)settings.dfNodataValue;

  size_t p = sourceX + sourceY * warperState.sourceGridWidth;

  if (settings.smoothingMemo != nullptr && settings.smoothingMemo[p] != MEMO_NODATAVALUE) {
    return (T)settings.smoothingMemo[p];
  }

  if (settings.smoothingMemo == nullptr) {
    settings.smoothingMemo = new double[warperState.sourceGridWidth * warperState.sourceGridHeight];
    CDF::fill(settings.smoothingMemo, CDF_DOUBLE, MEMO_NODATAVALUE, warperState.sourceGridWidth * warperState.sourceGridHeight);
  }

  double distanceAmmount = 0;
  double resultValue = 0;
  int smoothWindowSize = settings.smoothingFiter;
  size_t dWinP = -1;

  for (int y1 = -smoothWindowSize; y1 < smoothWindowSize + 1; y1++) {
    for (int x1 = -smoothWindowSize; x1 < smoothWindowSize + 1; x1++) {
      dWinP++;
      auto sx = x1 + sourceX;
      auto sy = y1 + sourceY;
      if (sx < warperState.sourceGridWidth && sy < warperState.sourceGridHeight && sx >= 0 && sy >= 0) {
        double value = (double)(sourceGrid[(sx) + (sy)*warperState.sourceGridWidth]);
        if ((settings.hasNodataValue && ((value) == (T)settings.dfNodataValue)) || !(value == value)) continue;
        double d = 1; // settings.smoothingDistanceMatrix[dWinP];
        distanceAmmount += d;
        resultValue += value * d;
      }
    }
  }
  double value = (T)settings.dfNodataValue;
  if (distanceAmmount > 0) {
    value = (resultValue /= distanceAmmount);
  }
  settings.smoothingMemo[p] = value;
  return (T)value;
}

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE) template CPPTYPE smoothingAtLocation<CPPTYPE>(int sourceX, int sourceY, CPPTYPE *inputGrid, GDWState &warperState, GDWDrawFunctionSettings &settings);
ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)

#undef SPECIALIZE_TEMPLATE
