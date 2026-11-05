#ifndef GDW_BULK_COLOR_H
#define GDW_BULK_COLOR_H

#include <array>
#include <cstring>
#include <vector>
#include "CDrawFunction.h"

inline constexpr int GDW_BATCH_SIZE = 256;

// Color classification is independent of pixel ownership. Batch pixels across
// triangle boundaries; replay the resulting paints in their original order.
class GDWColorMapper {
  GDWDrawFunctionSettings &settings;
  std::vector<double> minima, maxima;
  std::vector<uint32_t> intervalColors;
  std::array<uint32_t, 240> palette{};
  uint32_t background = 0;
  std::array<int, 1024> searchHints{};
  double hintLower = 0, hintUpper = 0, hintScale = 0;
  bool hasHints = false;

public:
  static uint32_t pack(const CColor &color) {
    static_assert(sizeof(CColor) == sizeof(uint32_t));
    uint32_t packed;
    std::memcpy(&packed, &color, sizeof(packed));
    return packed;
  }

  explicit GDWColorMapper(GDWDrawFunctionSettings &settings) : settings(settings) {
    if (settings.isUsingShadeIntervals && !(settings.shadeInterval > 0)) {
      for (const auto &interval : settings.intervals) {
        minima.push_back(interval.min);
        maxima.push_back(interval.max);
        intervalColors.push_back(pack(interval.color));
      }
      if (!minima.empty() && std::all_of(minima.begin(), minima.end(), [](double v) { return std::isfinite(v); })) {
        hintLower = minima.front();
        hintUpper = minima.back();
        double span = hintUpper - hintLower;
        if (std::isnormal(span) && span > 0) {
          hintScale = double(searchHints.size()) / span;
          hasHints = std::isfinite(hintScale);
          if (hasHints) {
            for (size_t i = 0; i < searchHints.size(); ++i) {
              double value = hintLower + (double(i) + 0.5) / double(searchHints.size()) * span;
              searchHints[i] = int(std::upper_bound(minima.begin(), minima.end(), value) - minima.begin());
            }
          }
        }
      }
      if (settings.bgColorDefined) background = pack(settings.bgColor);
    } else {
      for (int i = 0; i < int(palette.size()); ++i) palette[i] = pack(settings.getCColorFromLegendPaletteIndex(i));
    }
  }

  template <typename T> void map(const T *values, uint32_t *output, int count) const {
    if (settings.isUsingShadeIntervals && !(settings.shadeInterval > 0)) {
      // Hints only predict an upper_bound position. Validate both neighboring
      // boundaries exactly, then fall back to upper_bound when a bucket straddles
      // a boundary. This supports arbitrary widths, gaps and duplicate minima.
      alignas(64) int first[GDW_BATCH_SIZE];
      alignas(64) unsigned char exact[GDW_BATCH_SIZE];
      const int n = int(minima.size());
#ifdef ADAGUC_OPENMP_SIMD
#pragma omp simd
#endif
      for (int i = 0; i < count; ++i) {
        int candidate = 0;
        if (hasHints) {
          double value = double(values[i]);
          if (!(value > hintLower && value < hintUpper)) {
            candidate = value < hintLower ? 0 : n;
          } else {
            int bucket = std::clamp(int((value - hintLower) * hintScale), 0, int(searchHints.size()) - 1);
            candidate = searchHints[bucket];
          }
        }
        first[i] = candidate;
        exact[i] = (candidate == 0 || !(values[i] < minima[candidate - 1])) && (candidate == n || values[i] < minima[candidate]);
      }
      for (int i = 0; i < count; ++i) {
        if (!exact[i]) first[i] = int(std::upper_bound(minima.begin(), minima.end(), values[i]) - minima.begin());
        uint32_t color = background;
        if (first[i] > 0 && values[i] < maxima[first[i] - 1]) color = intervalColors[first[i] - 1];
        output[i] = isNodataValue(values[i], settings) ? 0 : color;
      }
    } else if (settings.legendLog == 0) {
#ifdef ADAGUC_OPENMP_SIMD
#pragma omp simd
#endif
      for (int i = 0; i < count; ++i) {
        if (isNodataValue(values[i], settings)) {
          output[i] = 0;
          continue;
        }
        if (settings.isUsingShadeIntervals) {
          float value = std::floor(values[i] / settings.shadeInterval) * settings.shadeInterval;
          output[i] = palette[determinePixelIndexFromValue(value, settings)];
        } else {
          output[i] = palette[determinePixelIndexFromValue(values[i], settings)];
        }
      }
    } else {
      // Retain the existing log10 and source-type rounding rules without fast-math.
      for (int i = 0; i < count; ++i) output[i] = pack(determinePixelColorFromValue(values[i], &settings));
    }
  }
};

class GDWBatchPainter {
  GDWDrawFunctionSettings &settings;
  GDWColorMapper mapper;

public:
  explicit GDWBatchPainter(GDWDrawFunctionSettings &settings) : settings(settings), mapper(settings) {}

  template <typename T> void write(int count, const int *x, const int *y, const T *values) {
    alignas(64) uint32_t colors[GDW_BATCH_SIZE];
    if (settings.drawgrid) mapper.map(values, colors, count);
    CColor color;
    auto *grid = (float *)settings.destinationGrid;
    const int width = settings.drawImage->geoParams.width;
    // Writes may overlap, and alpha blending depends on traversal order.
    for (int i = 0; i < count; ++i) {
      grid[size_t(y[i]) * width + x[i]] = values[i];
      if (!settings.drawgrid) continue;
      std::memcpy(&color, &colors[i], sizeof(color));
      if (color.a > 0) settings.drawImage->setPixel(x[i], y[i], color);
      else if (color.r > 0 || color.g > 0 || color.b > 0) settings.drawImage->setPixelTrueColorOverWrite(x[i], y[i], color.r, color.g, color.b, color.a);
    }
  }
};

template <typename T> class GDWPixelBatch {
  GDWBatchPainter &painter;
  int count = 0;
  int x[GDW_BATCH_SIZE], y[GDW_BATCH_SIZE];
  T values[GDW_BATCH_SIZE];

public:
  explicit GDWPixelBatch(GDWBatchPainter &painter) : painter(painter) {}
  void append(int px, int py, T value) {
    x[count] = px;
    y[count] = py;
    values[count++] = value;
    if (count == GDW_BATCH_SIZE) flush();
  }
  void flush() {
    if (count) painter.write(count, x, y, values);
    count = 0;
  }
};

#endif
