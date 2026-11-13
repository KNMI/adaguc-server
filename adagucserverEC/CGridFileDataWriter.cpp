#include "CGridFileDataWriter.h"
#include <cstdio>
#include <set>
#include "CCDFObject.h"
#include "CTime.h"
#include "CCDFNetCDFIO.h"
#include "CServerParams.h"
#include "CDataSource.h"
#include "utils/serverutils.h"
#include "CDebugger.h"
#include "CTString.h"
#include "CImageWarper.h"
#include "GridFileWriters/AAIGridWriter.h"
#include "GridFileWriters/GeoTiffGeoKeys.h"
#include "GridFileWriters/GeoTiffWriter.h"

static const bool CGRIDFILEDATAWRITER_DEBUG = false;

WCSDriver getWCSDriver(const std::string &driverName, const std::string &formatName) {
  if (driverName.empty()) {
    if (CT::equalsIgnoreCase(formatName, "GEOTIFF")) return WCSDriver::GeoTiff;
    if (CT::equalsIgnoreCase(formatName, "AAIGRID")) return WCSDriver::AAIGrid;
    return WCSDriver::Unsupported;
  }
  if (CT::equalsIgnoreCase(driverName, "ADAGUCNetCDF") || CT::equalsIgnoreCase(driverName, "NetCDF")) return WCSDriver::NetCDF;
  if (CT::equalsIgnoreCase(driverName, "GTiff")) return WCSDriver::GeoTiff;
  if (CT::equalsIgnoreCase(driverName, "AAIGrid")) return WCSDriver::AAIGrid;
  return WCSDriver::Unsupported;
}

std::string getUnsupportedWCSDriverMessage(const std::string &driverName, const std::string &formatName) {
  if (driverName.empty()) {
    return CT::printf("WCSFormat \"%s\" has no driver. Supported drivers are ADAGUCNetCDF, GTiff and AAIGrid", formatName.c_str());
  }
  return CT::printf("WCSFormat \"%s\": driver \"%s\" is not supported. Supported drivers are ADAGUCNetCDF, GTiff and AAIGrid (other GDAL drivers are no longer available)", formatName.c_str(),
                    driverName.c_str());
}

int CGridFileDataWriter::init(CServerParams *_srvParam, CDataSource *dataSource, int _NrOfBands) {
  if (CGRIDFILEDATAWRITER_DEBUG) {
    CDBDebug("INIT");
  }
  srvParam = _srvParam;
  if (_NrOfBands < 0) {
    CDBError("Invalid number of bands: %d", _NrOfBands);
    return 1;
  }
  NrOfBands = _NrOfBands;
  int status;
  _dataSource = dataSource;
  // Init projections
  if (CT::startsWith(srvParam->geoParams.crs, "PROJ4:")) {
    std::string temp(srvParam->geoParams.crs.c_str() + 6);
    srvParam->geoParams.crs = temp;
  }
  // Load metadata from the dataSource
  status = reader.open(dataSource, CNETCDFREADER_MODE_GET_METADATA);
  if (status != 0) {
    CDBError("Could not open file: %s", dataSource->getFileName().c_str());
    return 1;
  }

  try {
    TimeUnit = reader.getTimeUnit(dataSource);
  } catch (int e) {
    TimeUnit = "";
  }

  if (CGRIDFILEDATAWRITER_DEBUG) {
    CDBDebug("Time unit: %s", TimeUnit.c_str());
  }

  reader.close();

  // Set up geo parameters
  if (srvParam->WCS_GoNative == 1) {
    // Native!
    for (int j = 0; j < 4; j++) dfSrcBBOX[j] = dataSource->dfBBOX[j];
    dfDstBBOX[0] = dfSrcBBOX[0];
    dfDstBBOX[1] = dfSrcBBOX[1];
    dfDstBBOX[2] = dfSrcBBOX[2];
    dfDstBBOX[3] = dfSrcBBOX[3];
    srvParam->geoParams.bbox = dfSrcBBOX;
    srvParam->geoParams.width = dataSource->dWidth;
    srvParam->geoParams.height = dataSource->dHeight;
    srvParam->geoParams.crs = dataSource->nativeProj4;
  } else {
    // Non native projection units
    for (int j = 0; j < 4; j++) dfSrcBBOX[j] = dataSource->dfBBOX[j];
    srvParam->geoParams.bbox.toArray(dfDstBBOX);
    /* CRS is provided, but not RESX+RESY or HEIGHT+WIDTH, calculate here */
    if (srvParam->dfResX == 0 && srvParam->dfResY == 0 && srvParam->geoParams.width == 1 && srvParam->geoParams.height == 1) {
      CImageWarper warper;
      dataSource->srvParams = this->srvParam;
      status = warper.initreproj(dataSource, this->srvParam->geoParams, &srvParam->cfg->Projection);
      if (status != 0) {
        CDBError("Unable to initialize projection");
        return 1;
      }
      double dfBBOX[4];
      for (int j = 0; j < 4; j++) dfBBOX[j] = dataSource->dfBBOX[j];
      warper.findExtent(dataSource, dfBBOX);
      double resXInRequestedCRS = fabs((dfBBOX[2] - dfBBOX[0]) / dataSource->dWidth);
      double resYInRequestedCRS = fabs((dfBBOX[3] - dfBBOX[1]) / dataSource->dHeight);
      srvParam->dfResX = resXInRequestedCRS;
      srvParam->dfResY = resYInRequestedCRS;
    }

    // dfResX and dfResY are in the CRS ore ResponseCRS
    if (srvParam->dfResX != 0 && srvParam->dfResY != 0) {
      srvParam->geoParams.width = int(((dfDstBBOX[2] - dfDstBBOX[0]) / srvParam->dfResX));
      srvParam->geoParams.height = int(((dfDstBBOX[1] - dfDstBBOX[3]) / srvParam->dfResY));
      srvParam->geoParams.height = abs(srvParam->geoParams.height);
    }
    if (srvParam->geoParams.width > 20000 || srvParam->geoParams.height > 20000) {
      CDBError("Requested Width or Height is larger than 20000 pixels. Aborting request.");
      return 1;
    }
  }

  // Setup Destination geotransform
  grid.geoTransform[0] = dfDstBBOX[0];
  grid.geoTransform[1] = (dfDstBBOX[2] - dfDstBBOX[0]) / double(srvParam->geoParams.width);
  grid.geoTransform[2] = 0;
  grid.geoTransform[3] = dfDstBBOX[3];
  grid.geoTransform[4] = 0;
  grid.geoTransform[5] = (dfDstBBOX[1] - dfDstBBOX[3]) / double(srvParam->geoParams.height);
  grid.width = srvParam->geoParams.width;
  grid.height = srvParam->geoParams.height;

  if (driver == WCSDriver::AAIGrid && NrOfBands > 1) {
    CDBError("This WCS format ('%s') does not support multiple bands. Select a single image, or choose an other format.", srvParam->Format.c_str());
    return 1;
  }

  // Setup data types
  CDFType dataType = dataSource->getDataObject(0)->cdfVariable->getType();
  std::set<CDFType> supportedTypes = {CDF_CHAR, CDF_UBYTE, CDF_SHORT, CDF_USHORT, CDF_INT, CDF_UINT, CDF_FLOAT, CDF_DOUBLE};
  if (supportedTypes.count(dataType) == 0) {
    CDBError("Invalid datatype: dataSource->getDataObject(0)->cdfVariable->getType()=%s", CDF::getCDFDataTypeName(dataType).c_str());
    return 1;
  }
  grid.type = dataType;

  if (CGRIDFILEDATAWRITER_DEBUG) {
    CDBDebug("Dataset datatype = %s WH = [%d,%d], NrOfBands = [%d]", CDF::getCDFDataTypeName(dataType).c_str(), dataSource->dWidth, dataSource->dHeight, NrOfBands);
  }

  if (initCRS() != 0) return 1;
  grid.bands.clear();
  grid.metadata.clear();
  return 0;
}

int CGridFileDataWriter::initCRS() {
  grid.crs = srvParam->geoParams.crs;
  // AAIGrid has no CRS in the file
  if (driver != WCSDriver::GeoTiff || isCRSKnownToPROJ(grid.crs)) return 0;
  // Not an EPSG code known to PROJ: use the proj string configured for it in <Projection>
  CImageWarper warper;
  std::string decodedCRS;
  if (warper.decodeCRS(decodedCRS, grid.crs, &srvParam->cfg->Projection) == 0 && isCRSKnownToPROJ(decodedCRS)) {
    grid.crs = decodedCRS;
    return 0;
  }
  CDBError("WCS: Invalid source projection: [%s]", srvParam->geoParams.crs.c_str());
  return 1;
}

int CGridFileDataWriter::addData(std::vector<CDataSource *> &dataSources) {
  if (CGRIDFILEDATAWRITER_DEBUG) {
    CDBDebug("addData");
  }
  int status;
  CDataSource *dataSource = dataSources[0];
  status = reader.open(dataSource, CNETCDFREADER_MODE_OPEN_ALL);

  if (status != 0) {
    CDBError("Could not open file: %s", dataSource->getFileName().c_str());
    return 1;
  }
  if (CGRIDFILEDATAWRITER_DEBUG) {
    CDBDebug("Reading %s for bandnr %zu", dataSource->getFileName().c_str(), grid.bands.size());
  }
  grid.noDataValue = NAN;
  if (dataSource->getDataObject(0)->hasNodataValue == 1) {
    grid.noDataValue = dataSource->getDataObject(0)->dfNodataValue;
  }

  CDFType dataType = dataSource->getDataObject(0)->cdfVariable->getType();
  if (dataType != grid.type) {
    // All bands are written with the data type of the first one
    CDBError("The data type of %s (%s) differs from the first band (%s)", dataSource->getFileName().c_str(), CDF::getCDFDataTypeName(dataType).c_str(), CDF::getCDFDataTypeName(grid.type).c_str());
    reader.close();
    return 1;
  }
  size_t numValues = static_cast<size_t>(srvParam->geoParams.width) * srvParam->geoParams.height;
  GridFileBand band;
  band.data.resize(numValues * CDF::getTypeSize(dataType));
  void *warpedData = band.data.data();

  if (CDF::fill(warpedData, dataType, grid.noDataValue, numValues) != 0) {
    CDBError("Unable to initialize data field to nodata value");
    return 1;
  }

  // Warp
  CImageWarper warper;

  status = warper.initreproj(dataSource, srvParam->geoParams, &srvParam->cfg->Projection);
  if (status != 0) {
    CDBError("Unable to initialize projection");
    return 1;
  }
  GeoParameters sourceGeo;

  sourceGeo.width = dataSource->dWidth;
  sourceGeo.height = dataSource->dHeight;
  sourceGeo.bbox = dataSource->dfBBOX;

  sourceGeo.cellsizeX = dataSource->dfCellSizeX;
  sourceGeo.cellsizeY = dataSource->dfCellSizeY;
  sourceGeo.crs = dataSource->nativeProj4;

  void *sourceData = dataSource->getDataObject(0)->cdfVariable->data;

  DrawFunctionState drawFunctionState;
  drawFunctionState.width = srvParam->geoParams.width;
  drawFunctionState.height = srvParam->geoParams.height;
  drawFunctionState.data = warpedData;

  GenericDataWarper genericDataWarper;
  GDWArgs args = {.warper = &warper, .sourceData = sourceData, .sourceGeoParams = sourceGeo, .destGeoParams = srvParam->geoParams};

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return drawFunction(x, y, val, warperState, drawFunctionState); });
  ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER

  /* Band metadata */
  CCDFDims *dims = _dataSource->getCDFDims();
  for (size_t d = 0; d < dims->size(); d++) {
    std::string dimName = dims->at(d).name;
    if (dimName != "forecast_reference_time") {
      band.metadata.push_back({"NETCDF_DIM_" + dimName, getDimensionValue(d, dims)});
    }
  }
  band.metadata.push_back({"NETCDF_VARNAME", _dataSource->getDataObject(0)->cdfVariable->name});
  grid.bands.push_back(std::move(band));

  reader.close();
  if (CGRIDFILEDATAWRITER_DEBUG) {
    CDBDebug("/addData");
  }
  return 0;
}

// The dataset metadata, with the same names as the GDAL netCDF driver uses
void CGridFileDataWriter::addDatasetMetadata() {
  std::string extraDimNames = "{";

  bool first = true;

  CCDFDims *dims = _dataSource->getCDFDims();
  for (size_t d = 0; d < _dataSource->requiredDims.size(); d++) {
    std::string dimName = dims->at(d).name;
    if (dimName == "forecast_reference_time") continue;

    CDFType cdf_type = -1;
    try {
      cdf_type = _dataSource->getDataObject(0)->cdfObject->getVariableThrows(dimName.c_str())->getType();
    } catch (int e) {
      CDBDebug("Exception code %d for dimension name %s", e, dimName.c_str());
    }
    if (cdf_type == -1) continue;
    if (!first) extraDimNames += ",";
    first = false;
    extraDimNames += dimName;
    std::string dimDef = CT::printf("{%zu,%d}", _dataSource->requiredDims[d].uniqueValues.size(), CDFNetCDFWriter::NCtypeConversion(cdf_type));
    grid.metadata.push_back({CT::printf("NETCDF_DIM_%s_DEF", dimName.c_str()), dimDef});

    std::set<std::string> uniqueValues;
    for (size_t t = 0; t < _dataSource->timeSteps.size(); t++) {
      uniqueValues.insert(getDimensionValue(d, &_dataSource->timeSteps[t].dims));
    }
    std::vector<std::string> values(uniqueValues.begin(), uniqueValues.end());
    grid.metadata.push_back({CT::printf("NETCDF_DIM_%s_VALUES", dimName.c_str()), "{" + CT::join(values, ",") + "}"});
  }
  extraDimNames += "}";

  if (extraDimNames.length() > 2) {
    grid.metadata.push_back({"NETCDF_DIM_EXTRA", extraDimNames});
  }

  for (const auto &kvp: _dataSource->metaDataItems) {
    grid.metadata.push_back({kvp.key + "#" + kvp.value, kvp.abstract});
  }
}

int CGridFileDataWriter::writeFile(const std::string &fileName) {
  if (driver == WCSDriver::AAIGrid) {
    return writeAAIGrid(grid, fileName);
  }
  addDatasetMetadata();
  return writeGeoTiff(grid, fileName);
}

int CGridFileDataWriter::streamFile(const std::string &fileName) {
  if (mimeType.length() < 2) mimeType = ("Content-Type:text/plain");
  FILE *fp = fopen(fileName.c_str(), "r");
  if (fp == NULL) {
    CDBError("Invalid File: %s<br>\n", fileName.c_str());
    return 1;
  }
  // Send the binary data
  fseek(fp, 0L, SEEK_END);
  size_t endPos = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  CDBDebug("Now start streaming %lu bytes to the client with mimetype %s", endPos, mimeType.c_str());
  printf("Content-Disposition: attachment; filename=%s\r\n", generateGetCoverageFileName().c_str());
  printf("Content-Description: File Transfer\r\n");
  printf("Content-Transfer-Encoding: binary\r\n");
  printf("Content-Length: %zu\r\n", endPos);
  printf("%s%s\r\n\n", mimeType.c_str(), srvParam->getResponseHeaders(srvParam->getCacheControlOption()).c_str());
  copyFileToStream(fp, stdout);
  fclose(fp);
  fclose(stdout);
  return 0;
}

int CGridFileDataWriter::end() {
  if (CGRIDFILEDATAWRITER_DEBUG) {
    CDBDebug("END");
  }
  const char *pszADAGUCWriteToFile = getenv("ADAGUC_WRITETOFILE");
  if (pszADAGUCWriteToFile != NULL) {
    CDBDebug("Write to ADAGUC_WRITETOFILE %s", pszADAGUCWriteToFile);
    return writeFile(pszADAGUCWriteToFile);
  }

  // Generate a temporary filename for storage
  std::string tmpFileName = srvParam->cfg->TempDir[0].attr.value + "/" + generateUniqueGetCoverageFileName(this);
  if (writeFile(tmpFileName) != 0) {
    CDBError("Failed to create output file: \"%s\"", tmpFileName.c_str());
    remove(tmpFileName.c_str());
    return 1;
  }
  int returnCode = streamFile(tmpFileName);
  remove(tmpFileName.c_str());
  return returnCode;
}

std::string CGridFileDataWriter::generateGetCoverageFileName() {
  std::string humanReadableString = srvParam->Format;
  humanReadableString += "_";
  humanReadableString += dObjgetVariableName(*_dataSource->getDataObject(0));

  for (size_t i = 0; i < _dataSource->requiredDims.size(); i++) {
    CT::printfconcat(humanReadableString, "_%s", _dataSource->requiredDims[i].value.c_str());
  }

  CT::replaceSelf(humanReadableString, ":", "_");
  CT::replaceSelf(humanReadableString, ".", "_");

  humanReadableString += driver == WCSDriver::AAIGrid ? ".asc" : ".tif";

  return humanReadableString;
}
std::string generateUniqueGetCoverageFileName(CGridFileDataWriter *dataWriter) {
  auto s = dataWriter->srvParam;
  auto d = dataWriter->_dataSource;
  auto variableName = dObjgetVariableName(*d->getDataObject(0));
  std::string dimSettings;
  for (size_t i = 0; i < d->cfgLayer->Dimension.size() && i < 4; i++) {
    CT::printfconcat(dimSettings, "%d_", int(d->getDimensionIndex(i)));
  }

  std::string fileName =
      CT::printf("%s_%s_%f_%f_%f_%f_%d_%d_%f_%f_%s_%s_%s", s->Format.c_str(), variableName.c_str(), s->geoParams.bbox.left, s->geoParams.bbox.bottom, s->geoParams.bbox.right, s->geoParams.bbox.top,
                 s->geoParams.width, s->geoParams.height, s->dfResX, s->dfResY, CT::basename(s->configFileName).c_str(), dimSettings.c_str(), CT::randomString(12).c_str());
  for (auto c: {".", "=", "+", "/", ":", ";", "?", "&", "|", "<", ">", "\"", "'", "\\", "*", "^", "%", "$", "#", "@", "!", "~"}) {
    fileName = CT::replace(fileName, c, "_");
  }
  fileName += ".tmp";

  CDBDebug("Generated unique filename: %s", fileName.c_str());

  return fileName;
}

std::string CGridFileDataWriter::getDimensionValue(int d, CCDFDims *dims) {
  std::string value;
  if (isOGCTimeDim(dims->at(d))) {
    CTime adagucTime;
    try {
      value = "0";
      adagucTime.init(TimeUnit.c_str(), ""); // TODO replace with var
      double offset = adagucTime.dateToOffset(adagucTime.ISOStringToDate(dims->at(d).value));
      value = CT::printf("%f", offset);
    } catch (int e) {
      CDBDebug("Warning in getDimensionValue: Unable to get string value from time dimension");
    }

  } else {
    value = CT::printf("%s", dims->at(d).value.c_str());
  }
  return value;
}
