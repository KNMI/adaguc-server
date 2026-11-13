#ifndef GRIDFILEDATA_H
#define GRIDFILEDATA_H

#include <string>
#include <utility>
#include <vector>
#include "CCDFTypes.h"

typedef std::vector<std::pair<std::string, std::string>> GridFileMetadata;

struct GridFileBand {
  std::vector<unsigned char> data; // width * height values of GridFileData::type, top row first
  GridFileMetadata metadata;
};

// An in-memory raster, georeferenced with a GDAL style geotransform, which can be written as GeoTIFF or AAIGrid
struct GridFileData {
  int width = 0;
  int height = 0;
  CDFType type = CDF_NONE;
  // x = gt[0] + col * gt[1] + row * gt[2], y = gt[3] + col * gt[4] + row * gt[5]
  double geoTransform[6] = {0, 1, 0, 0, 0, -1};
  std::string crs;        // EPSG code like EPSG:4326, or a proj string
  double noDataValue = 0; // NaN when the source data has no nodata value
  std::vector<GridFileBand> bands;
  GridFileMetadata metadata;
};

#endif
