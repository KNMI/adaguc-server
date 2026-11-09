#include "smoothRasterField.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <vector>
#include <GenericDataWarper/GDWDrawFunctionSettings.h>
#include "CCDFTypes.h"
#include "Types/CPointTypes.h"

// The smoothed grid is computed lazily in square tiles, so the work and memory scale with the part of the grid that is actually read.
static constexpr int SMOOTHING_TILE_SIZE = 64;

template <typename T> static void computeSmoothedTile(int tileX, int tileY, T *sourceGrid, GDWState &warperState, GDWDrawFunctionSettings &settings, std::vector<double> &tile) {
  int width = warperState.sourceGridWidth;
  int height = warperState.sourceGridHeight;
  int r = (int)settings.smoothingFiter;
  int x0 = tileX * SMOOTHING_TILE_SIZE;
  int y0 = tileY * SMOOTHING_TILE_SIZE;
  int x1 = std::min(width, x0 + SMOOTHING_TILE_SIZE);
  int y1 = std::min(height, y0 + SMOOTHING_TILE_SIZE);
  int tileWidth = x1 - x0;
  int rowStart = std::max(0, y0 - r);
  int rowEnd = std::min(height, y1 + r);

  double nodataAsDouble = settings.hasNodataValue ? (double)(T)settings.dfNodataValue : 0.0;
  auto isValid = [&](double value) { return std::isfinite(value) && !(settings.hasNodataValue && value == nodataAsDouble); };

  // Horizontal pass: sum and count of valid values within [x-r, x+r], for the tile's columns and r extra rows above and below.
  size_t numRowElements = (size_t)(rowEnd - rowStart) * tileWidth;
  std::vector<double> rowSum(numRowElements);
  std::vector<int32_t> rowCount(numRowElements);
  for (int y = rowStart; y < rowEnd; y++) {
    const T *sourceRow = sourceGrid + (size_t)y * width;
    size_t rowBase = (size_t)(y - rowStart) * tileWidth;
    for (int x = x0; x < x1; x++) {
      int a = std::max(0, x - r);
      int b = std::min(width - 1, x + r);
      double sum = 0;
      int32_t count = 0;
      for (int sx = a; sx <= b; sx++) {
        double value = (double)sourceRow[sx];
        if (isValid(value)) {
          sum += value;
          count++;
        }
      }
      rowSum[rowBase + (x - x0)] = sum;
      rowCount[rowBase + (x - x0)] = count;
    }
  }

  // Vertical pass: sum and count of the horizontal results within [y-r, y+r], row by row so memory is read in order.
  tile.resize((size_t)(y1 - y0) * tileWidth);
  std::vector<double> columnSum(tileWidth);
  std::vector<int32_t> columnCount(tileWidth);
  for (int y = y0; y < y1; y++) {
    std::ranges::fill(columnSum, 0.0);
    std::ranges::fill(columnCount, 0);
    int a = std::max(0, y - r);
    int b = std::min(height - 1, y + r);
    for (int sy = a; sy <= b; sy++) {
      size_t rowBase = (size_t)(sy - rowStart) * tileWidth;
      for (int i = 0; i < tileWidth; i++) {
        columnSum[i] += rowSum[rowBase + i];
        columnCount[i] += rowCount[rowBase + i];
      }
    }
    size_t tileBase = (size_t)(y - y0) * tileWidth;
    for (int i = 0; i < tileWidth; i++) {
      tile[tileBase + i] = columnCount[i] > 0 ? columnSum[i] / columnCount[i] : settings.dfNodataValue;
    }
  }
}

// Callers only invoke this when settings.smoothingFiter > 0.
template <typename T> T smoothingAtLocation(int sourceX, int sourceY, T *sourceGrid, GDWState &warperState, GDWDrawFunctionSettings &settings) {
  int width = warperState.sourceGridWidth;
  int height = warperState.sourceGridHeight;
  if (sourceX < 0 || sourceY < 0 || sourceX >= width || sourceY >= height) return (T)settings.dfNodataValue;

  int tilesX = (width + SMOOTHING_TILE_SIZE - 1) / SMOOTHING_TILE_SIZE;
  int tilesY = (height + SMOOTHING_TILE_SIZE - 1) / SMOOTHING_TILE_SIZE;
  if (settings.smoothingMemo.empty()) {
    settings.smoothingMemo.resize((size_t)tilesX * tilesY);
  }

  int tileX = sourceX / SMOOTHING_TILE_SIZE;
  int tileY = sourceY / SMOOTHING_TILE_SIZE;
  auto &tile = settings.smoothingMemo[(size_t)tileY * tilesX + tileX];
  if (tile.empty()) {
    computeSmoothedTile(tileX, tileY, sourceGrid, warperState, settings, tile);
  }

  int tileWidth = std::min(width, (tileX + 1) * SMOOTHING_TILE_SIZE) - tileX * SMOOTHING_TILE_SIZE;
  return (T)tile[(size_t)(sourceY - tileY * SMOOTHING_TILE_SIZE) * tileWidth + (sourceX - tileX * SMOOTHING_TILE_SIZE)];
}

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE) template CPPTYPE smoothingAtLocation<CPPTYPE>(int sourceX, int sourceY, CPPTYPE *inputGrid, GDWState &warperState, GDWDrawFunctionSettings &settings);
ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)

#undef SPECIALIZE_TEMPLATE
