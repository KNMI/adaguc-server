#ifndef GEOTIFFWRITER_H
#define GEOTIFFWRITER_H

#include <string>
#include "GridFileData.h"

/**
 * Writes the grid as uncompressed GeoTIFF with libtiff and libgeotiff, in the same layout as GDAL's GTiff driver with default options:
 * strips of about 8 KB, one sample per band (planar configuration "separate" for more than one band), the directory at the start of the file,
 * the nodata value in the GDAL_NODATA tag and the dataset and band metadata in the GDAL_METADATA tag.
 * Returns 0 on success.
 */
int writeGeoTiff(const GridFileData &grid, const std::string &fileName);

/**
 * Formats the metadata as the XML which GDAL stores in the GDAL_METADATA TIFF tag. Returns an empty string if there is no metadata.
 */
std::string formatGDALMetadataXML(const GridFileData &grid);

/**
 * Formats the nodata value as GDAL does in the GDAL_NODATA TIFF tag
 */
std::string formatGDALNoData(double noDataValue);

#endif
