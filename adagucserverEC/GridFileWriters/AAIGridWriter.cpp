#include "AAIGridWriter.h"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "CDebugger.h"
#include "CTString.h"

namespace {
  bool isIntegerType(CDFType type) {
    // Same choice as GDAL's AAIGrid driver: these are written as integers, the rest (including unsigned int) as floating point.
    return type == CDF_CHAR || type == CDF_UBYTE || type == CDF_SHORT || type == CDF_USHORT || type == CDF_INT;
  }

  double getValue(const GridFileBand &band, CDFType type, size_t index) {
    const void *data = band.data.data();
    switch (type) {
    case CDF_CHAR:
    case CDF_UBYTE:
      return static_cast<const unsigned char *>(data)[index];
    case CDF_SHORT:
      return static_cast<const short *>(data)[index];
    case CDF_USHORT:
      return static_cast<const unsigned short *>(data)[index];
    case CDF_INT:
      return static_cast<const int *>(data)[index];
    case CDF_UINT:
      return static_cast<const unsigned int *>(data)[index];
    case CDF_FLOAT:
      return static_cast<const float *>(data)[index];
    case CDF_DOUBLE:
      return static_cast<const double *>(data)[index];
    }
    return NAN;
  }

  // Formats like printf %.20g, but always gives nan, inf or -inf for non finite numbers
  std::string formatFloat(double value) {
    if (std::isnan(value)) return "nan";
    if (std::isinf(value)) return value > 0 ? "inf" : "-inf";
    return CT::printf("%.20g", value);
  }

  std::string formatHeader(const GridFileData &grid) {
    const double *gt = grid.geoTransform;
    double dx = gt[1];
    double dy = gt[5];
    double yllCorner = dy < 0 ? gt[3] + grid.height * dy : gt[3];
    std::string header = CT::printf("ncols        %d\nnrows        %d\nxllcorner    %.12f\nyllcorner    %.12f\n", grid.width, grid.height, gt[0], yllCorner);
    // Same tolerance as GDAL to decide if cells are square
    if (fabs(dx + dy) < 0.0000001 || fabs(dx - dy) < 0.0000001) {
      header += CT::printf("cellsize     %.12f\n", dx);
    } else {
      header += CT::printf("dx           %.12f\ndy           %.12f\n", dx, fabs(dy));
    }
    if (isIntegerType(grid.type)) {
      // An integer grid cannot have nan as nodata value: leave it out
      if (std::isfinite(grid.noDataValue)) {
        header += CT::printf("NODATA_value %d\n", static_cast<int>(grid.noDataValue));
      }
    } else {
      header += "NODATA_value " + formatFloat(grid.noDataValue) + "\n";
    }
    return header;
  }

  void formatRows(const GridFileData &grid, std::string &output) {
    const GridFileBand &band = grid.bands[0];
    bool integerType = isIntegerType(grid.type);
    // Like GDAL, append .0 to the first integral floating point value, so readers detect the grid as floating point
    bool hasDecimalDot = integerType;
    bool bottomUp = grid.geoTransform[5] > 0;
    char buffer[64];
    for (int row = 0; row < grid.height; row++) {
      size_t rowStart = static_cast<size_t>(bottomUp ? grid.height - 1 - row : row) * grid.width;
      for (int col = 0; col < grid.width; col++) {
        double value = getValue(band, grid.type, rowStart + col);
        if (integerType) {
          snprintf(buffer, sizeof(buffer), "%d ", static_cast<int>(value));
          output += buffer;
          continue;
        }
        std::string formatted = formatFloat(value);
        if (!hasDecimalDot && std::isfinite(value)) {
          if (formatted.find_first_of(".eE") == std::string::npos) formatted += ".0";
          hasDecimalDot = true;
        }
        output += formatted;
        output += " ";
      }
      output += "\n";
    }
  }
} // namespace

int formatAAIGrid(const GridFileData &grid, std::string &output) {
  if (grid.bands.size() != 1) {
    CDBError("AAIGrid supports exactly one band, got %zu bands", grid.bands.size());
    return 1;
  }
  if (grid.geoTransform[2] != 0 || grid.geoTransform[4] != 0) {
    CDBError("AAIGrid does not support rotated grids");
    return 1;
  }
  output = formatHeader(grid);
  formatRows(grid, output);
  return 0;
}

int writeAAIGrid(const GridFileData &grid, const std::string &fileName) {
  std::string output;
  if (formatAAIGrid(grid, output) != 0) return 1;
  FILE *fp = fopen(fileName.c_str(), "wb");
  if (fp == nullptr) {
    CDBError("Unable to open %s for writing: %s", fileName.c_str(), strerror(errno));
    return 1;
  }
  size_t written = fwrite(output.data(), 1, output.size(), fp);
  int closeStatus = fclose(fp);
  if (written != output.size() || closeStatus != 0) {
    CDBError("Unable to write %s", fileName.c_str());
    return 1;
  }
  return 0;
}
