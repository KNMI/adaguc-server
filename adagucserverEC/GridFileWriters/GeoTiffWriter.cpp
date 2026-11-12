#include "GeoTiffWriter.h"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <geotiffio.h>
#include <xtiffio.h>
#include "CDebugger.h"
#include "CTString.h"
#include "GeoTiffGeoKeys.h"

// Private TIFF tags of GDAL, see https://gdal.org/en/stable/drivers/raster/gtiff.html. Recent libtiff versions define them in tiff.h.
#ifndef TIFFTAG_GDAL_METADATA
#define TIFFTAG_GDAL_METADATA 42112
#endif
#ifndef TIFFTAG_GDAL_NODATA
#define TIFFTAG_GDAL_NODATA 42113
#endif

namespace {
  TIFFExtendProc parentTagExtender = nullptr;

  void extendWithGDALTags(TIFF *tif) {
    static char metadataTagName[] = "GDALMetadata";
    static char noDataTagName[] = "GDALNoDataValue";
    static const TIFFFieldInfo gdalFieldInfo[] = {
        {TIFFTAG_GDAL_METADATA, -1, -1, TIFF_ASCII, FIELD_CUSTOM, 1, 0, metadataTagName},
        {TIFFTAG_GDAL_NODATA, -1, -1, TIFF_ASCII, FIELD_CUSTOM, 1, 0, noDataTagName},
    };
    TIFFMergeFieldInfo(tif, gdalFieldInfo, sizeof(gdalFieldInfo) / sizeof(gdalFieldInfo[0]));
    if (parentTagExtender != nullptr) parentTagExtender(tif);
  }

  void logTIFFError(const char *module, const char *fmt, va_list args) {
    char message[1024];
    vsnprintf(message, sizeof(message), fmt, args);
    CDBError("libtiff %s: %s", module == nullptr ? "" : module, message);
  }

  void logTIFFWarning(const char *module, const char *fmt, va_list args) {
    char message[1024];
    vsnprintf(message, sizeof(message), fmt, args);
    CDBWarning("libtiff %s: %s", module == nullptr ? "" : module, message);
  }

  void initLibTIFF() {
    static bool initialized = false;
    if (initialized) return;
    parentTagExtender = TIFFSetTagExtender(extendWithGDALTags);
    TIFFSetErrorHandler(logTIFFError);
    TIFFSetWarningHandler(logTIFFWarning);
    initialized = true;
  }

  struct SampleType {
    int bitsPerSample;
    int sampleFormat;
  };

  bool getSampleType(CDFType type, SampleType &sampleType) {
    switch (type) {
    case CDF_CHAR:
    case CDF_UBYTE:
      sampleType = {8, SAMPLEFORMAT_UINT};
      return true;
    case CDF_SHORT:
      sampleType = {16, SAMPLEFORMAT_INT};
      return true;
    case CDF_USHORT:
      sampleType = {16, SAMPLEFORMAT_UINT};
      return true;
    case CDF_INT:
      sampleType = {32, SAMPLEFORMAT_INT};
      return true;
    case CDF_UINT:
      sampleType = {32, SAMPLEFORMAT_UINT};
      return true;
    case CDF_FLOAT:
      sampleType = {32, SAMPLEFORMAT_IEEEFP};
      return true;
    case CDF_DOUBLE:
      sampleType = {64, SAMPLEFORMAT_IEEEFP};
      return true;
    }
    return false;
  }

  std::string escapeXML(const std::string &input) {
    std::string output;
    for (char c: input) {
      switch (c) {
      case '&':
        output += "&amp;";
        break;
      case '<':
        output += "&lt;";
        break;
      case '>':
        output += "&gt;";
        break;
      case '"':
        output += "&quot;";
        break;
      default:
        output += c;
      }
    }
    return output;
  }

  // Like GDAL: keys are unique ignoring case (the last value wins) and sorted ignoring case
  GridFileMetadata sortedUniqueItems(const GridFileMetadata &items) {
    GridFileMetadata result;
    for (const auto &item: items) {
      auto existing = std::find_if(result.begin(), result.end(), [&](const auto &other) { return CT::equalsIgnoreCase(other.first, item.first); });
      if (existing != result.end()) {
        existing->second = item.second;
      } else {
        result.push_back(item);
      }
    }
    std::stable_sort(result.begin(), result.end(), [](const auto &a, const auto &b) { return CT::toUpperCase(a.first) < CT::toUpperCase(b.first); });
    return result;
  }

  void setGeoreferencing(TIFF *tif, const double *gt) {
    if (gt[2] == 0 && gt[4] == 0 && gt[5] < 0) {
      double pixelScale[3] = {gt[1], -gt[5], 0};
      double tiePoint[6] = {0, 0, 0, gt[0], gt[3], 0};
      TIFFSetField(tif, TIFFTAG_GEOPIXELSCALE, 3, pixelScale);
      TIFFSetField(tif, TIFFTAG_GEOTIEPOINTS, 6, tiePoint);
    } else {
      // South up or rotated grids need the full transformation matrix
      double transformation[16] = {gt[1], gt[2], 0, gt[0], gt[4], gt[5], 0, gt[3], 0, 0, 0, 0, 0, 0, 0, 1};
      TIFFSetField(tif, TIFFTAG_GEOTRANSMATRIX, 16, transformation);
    }
  }

  int setGeoKeys(TIFF *tif, const std::string &crs) {
    GTIF *gtif = GTIFNew(tif);
    if (gtif == nullptr) {
      CDBError("Unable to initialize GeoTIFF keys");
      return 1;
    }
    int status = setGeoKeysFromCRS(gtif, crs);
    if (status == 0) GTIFWriteKeys(gtif);
    GTIFFree(gtif);
    return status;
  }

  int setTags(TIFF *tif, const GridFileData &grid, const SampleType &sampleType) {
    uint16_t samplesPerPixel = static_cast<uint16_t>(grid.bands.size());
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, grid.width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, grid.height);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, sampleType.bitsPerSample);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, sampleType.sampleFormat);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, samplesPerPixel);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, samplesPerPixel > 1 ? PLANARCONFIG_SEPARATE : PLANARCONFIG_CONTIG);
    if (samplesPerPixel > 1) {
      std::vector<uint16_t> extraSamples(samplesPerPixel - 1, EXTRASAMPLE_UNSPECIFIED);
      TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, static_cast<uint16_t>(extraSamples.size()), extraSamples.data());
    }
    uint32_t rowsPerStrip = std::min(TIFFDefaultStripSize(tif, 0), static_cast<uint32_t>(grid.height));
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rowsPerStrip);
    setGeoreferencing(tif, grid.geoTransform);
    if (setGeoKeys(tif, grid.crs) != 0) return 1;
    std::string metadata = formatGDALMetadataXML(grid);
    if (!metadata.empty()) TIFFSetField(tif, TIFFTAG_GDAL_METADATA, metadata.c_str());
    TIFFSetField(tif, TIFFTAG_GDAL_NODATA, formatGDALNoData(grid.noDataValue).c_str());
    return 0;
  }

  // Writes the directory before the image data, like GDAL, so readers find it at the start of the file
  int writeDirectoryFirst(TIFF *tif) {
    if (!TIFFDeferStrileArrayWriting(tif) || !TIFFWriteCheck(tif, 0, "writeGeoTiff") || !TIFFWriteDirectory(tif) || !TIFFSetDirectory(tif, 0)) {
      CDBError("Unable to write the TIFF directory");
      return 1;
    }
    return 0;
  }

  int writeStrips(TIFF *tif, const GridFileData &grid, const SampleType &sampleType) {
    uint32_t rowsPerStrip = 0;
    TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);
    size_t bytesPerRow = static_cast<size_t>(grid.width) * sampleType.bitsPerSample / 8;
    uint32_t height = static_cast<uint32_t>(grid.height);
    uint32_t stripsPerBand = (height + rowsPerStrip - 1) / rowsPerStrip;
    for (size_t b = 0; b < grid.bands.size(); b++) {
      unsigned char *bandData = const_cast<unsigned char *>(grid.bands[b].data.data());
      for (uint32_t strip = 0; strip < stripsPerBand; strip++) {
        uint32_t firstRow = strip * rowsPerStrip;
        size_t rows = std::min(rowsPerStrip, height - firstRow);
        tstrip_t stripIndex = static_cast<tstrip_t>(b * stripsPerBand + strip);
        if (TIFFWriteEncodedStrip(tif, stripIndex, bandData + firstRow * bytesPerRow, static_cast<tmsize_t>(rows * bytesPerRow)) < 0) {
          CDBError("Unable to write TIFF strip %u", stripIndex);
          return 1;
        }
      }
    }
    if (!TIFFForceStrileArrayWriting(tif)) {
      CDBError("Unable to write the TIFF strip offsets");
      return 1;
    }
    return 0;
  }

  int writeTIFF(TIFF *tif, const GridFileData &grid, const SampleType &sampleType) {
    if (setTags(tif, grid, sampleType) != 0) return 1;
    if (writeDirectoryFirst(tif) != 0) return 1;
    return writeStrips(tif, grid, sampleType);
  }
} // namespace

std::string formatGDALNoData(double noDataValue) {
  if (std::isnan(noDataValue)) return "nan";
  if (std::isinf(noDataValue)) return noDataValue > 0 ? "inf" : "-inf";
  return CT::printf("%.17g", noDataValue);
}

std::string formatGDALMetadataXML(const GridFileData &grid) {
  std::string items;
  for (const auto &item: sortedUniqueItems(grid.metadata)) {
    items += CT::printf("  <Item name=\"%s\">%s</Item>\n", escapeXML(item.first).c_str(), escapeXML(item.second).c_str());
  }
  for (size_t b = 0; b < grid.bands.size(); b++) {
    for (const auto &item: sortedUniqueItems(grid.bands[b].metadata)) {
      items += CT::printf("  <Item name=\"%s\" sample=\"%zu\">%s</Item>\n", escapeXML(item.first).c_str(), b, escapeXML(item.second).c_str());
    }
  }
  if (items.empty()) return "";
  return "<GDALMetadata>\n" + items + "</GDALMetadata>\n";
}

int writeGeoTiff(const GridFileData &grid, const std::string &fileName) {
  SampleType sampleType;
  if (!getSampleType(grid.type, sampleType)) {
    CDBError("GeoTIFF: unsupported data type %s", CDF::getCDFDataTypeName(grid.type).c_str());
    return 1;
  }
  if (grid.bands.empty() || grid.width <= 0 || grid.height <= 0) {
    CDBError("GeoTIFF: nothing to write, %zu bands of %dx%d", grid.bands.size(), grid.width, grid.height);
    return 1;
  }
  initLibTIFF();
  TIFF *tif = XTIFFOpen(fileName.c_str(), "w");
  if (tif == nullptr) {
    CDBError("Unable to open %s for writing", fileName.c_str());
    return 1;
  }
  int status = writeTIFF(tif, grid, sampleType);
  XTIFFClose(tif);
  return status;
}
