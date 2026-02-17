#include "smoothRasterField.h"
#include <cstddef>
#include <cmath>
#include <CGenericDataWarper.h>
#include <GenericDataWarper/GDWDrawFunctionSettings.h>

#define MEMO_NODATAVALUE -99999999999999.f

template <typename T> T smoothingAtLocation(int sourceX, int sourceY, T *sourceGrid, GDWState &warperState, GDWDrawFunctionSettings &settings) {
  if (settings.smoothingFiter == 0) {
    return (T)settings.dfNodataValue;
  }
  if (sourceX < 0 || sourceY < 0 || sourceX >= warperState.sourceGridWidth || sourceY >= warperState.sourceGridHeight) return (T)settings.dfNodataValue;

  size_t p = sourceX + sourceY * warperState.sourceGridWidth;

  if (settings.smoothingMemo.empty() == false && settings.smoothingMemo[p] != MEMO_NODATAVALUE) {
    return (T)settings.smoothingMemo[p];
  }

  if (settings.smoothingMemo.empty()) {
    settings.smoothingMemo.resize(warperState.sourceGridWidth * warperState.sourceGridHeight);
    CDF::fill(settings.smoothingMemo.data(), CDF_DOUBLE, MEMO_NODATAVALUE, warperState.sourceGridWidth * warperState.sourceGridHeight);
  }

  double distanceAmmount = 0;
  double resultValue = 0;
  int smoothWindowSize = settings.smoothingFiter;

  for (int y1 = -smoothWindowSize; y1 < smoothWindowSize + 1; y1++) {
    for (int x1 = -smoothWindowSize; x1 < smoothWindowSize + 1; x1++) {
      auto sx = x1 + sourceX;
      auto sy = y1 + sourceY;
      if (sx < warperState.sourceGridWidth && sy < warperState.sourceGridHeight && sx >= 0 && sy >= 0) {
        double value = (double)(sourceGrid[(sx) + (sy)*warperState.sourceGridWidth]);
        if ((settings.hasNodataValue && ((value) == (T)settings.dfNodataValue)) || !(value == value)) continue;
        double d = 1;
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
