#include "smoothRasterField.h"
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <vector>
#include <GenericDataWarper/GDWDrawFunctionSettings.h>
#include "CCDFTypes.h"
#include "Types/CPointTypes.h"

// Fills settings.smoothingMemo with the average of the valid values within +/- smoothingFiter cells of each cell.
// Done as a horizontal and a vertical pass with prefix sums, so the cost does not depend on the radius.
template <typename T> void computeSmoothedGrid(T *sourceGrid, GDWState &warperState, GDWDrawFunctionSettings &settings) {
  int width = warperState.sourceGridWidth;
  int height = warperState.sourceGridHeight;
  int r = (int)settings.smoothingFiter;
  size_t numGridElements = (size_t)width * (size_t)height;

  double nodataAsDouble = settings.hasNodataValue ? (double)(T)settings.dfNodataValue : 0.0;
  auto isValid = [&](double value) { return !((settings.hasNodataValue && value == nodataAsDouble) || std::isnan(value)); };

  // Horizontal pass: sum and count of valid values within [x-r, x+r]
  std::vector<double> rowSum(numGridElements);
  std::vector<int32_t> rowCount(numGridElements);
  std::vector<double> prefixVal(width + 1);
  std::vector<int32_t> prefixCnt(width + 1);
  for (int y = 0; y < height; y++) {
    size_t rowBase = (size_t)y * width;
    prefixVal[0] = 0;
    prefixCnt[0] = 0;
    for (int x = 0; x < width; x++) {
      double value = (double)sourceGrid[rowBase + x];
      bool valid = isValid(value);
      prefixVal[x + 1] = prefixVal[x] + (valid ? value : 0.0);
      prefixCnt[x + 1] = prefixCnt[x] + (valid ? 1 : 0);
    }
    for (int x = 0; x < width; x++) {
      int a = x - r < 0 ? 0 : x - r;
      int b = x + r >= width ? width - 1 : x + r;
      rowSum[rowBase + x] = prefixVal[b + 1] - prefixVal[a];
      rowCount[rowBase + x] = prefixCnt[b + 1] - prefixCnt[a];
    }
  }

  // Vertical pass: sum and count of the horizontal results within [y-r, y+r]
  settings.smoothingMemo.resize(numGridElements);
  std::vector<double> prefixVal2(height + 1);
  std::vector<int32_t> prefixCnt2(height + 1);
  for (int x = 0; x < width; x++) {
    prefixVal2[0] = 0;
    prefixCnt2[0] = 0;
    for (int y = 0; y < height; y++) {
      prefixVal2[y + 1] = prefixVal2[y] + rowSum[(size_t)y * width + x];
      prefixCnt2[y + 1] = prefixCnt2[y] + rowCount[(size_t)y * width + x];
    }
    for (int y = 0; y < height; y++) {
      int a = y - r < 0 ? 0 : y - r;
      int b = y + r >= height ? height - 1 : y + r;
      double sum = prefixVal2[b + 1] - prefixVal2[a];
      int32_t count = prefixCnt2[b + 1] - prefixCnt2[a];
      settings.smoothingMemo[(size_t)y * width + x] = count > 0 ? sum / count : settings.dfNodataValue;
    }
  }
}

// Callers only invoke this when settings.smoothingFiter > 0.
template <typename T> T smoothingAtLocation(int sourceX, int sourceY, T *sourceGrid, GDWState &warperState, GDWDrawFunctionSettings &settings) {
  if (sourceX < 0 || sourceY < 0 || sourceX >= warperState.sourceGridWidth || sourceY >= warperState.sourceGridHeight) return (T)settings.dfNodataValue;

  if (settings.smoothingMemo.empty()) {
    computeSmoothedGrid(sourceGrid, warperState, settings);
  }

  size_t p = sourceX + sourceY * warperState.sourceGridWidth;
  return (T)settings.smoothingMemo[p];
}

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE) template CPPTYPE smoothingAtLocation<CPPTYPE>(int sourceX, int sourceY, CPPTYPE *inputGrid, GDWState &warperState, GDWDrawFunctionSettings &settings);
ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)

#undef SPECIALIZE_TEMPLATE
