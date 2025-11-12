#include "CCreateTiles.h"
#include "CDBAdapterPostgreSQL.h"
#include "CDBFactory.h"
#include "CReporter.h"
#include "CRequest.h"
#include "CNetCDFDataWriter.h"

const char *CCreateTiles::className = "CCreateTiles";

int CCreateTiles::createTiles(CDataSource *dataSource, int scanFlags) {
  if (dataSource->isConfigured == false) {
    CDBError("Error! dataSource->isConfigured == false");
    return 1;
  }

  if (dataSource->srvParams->configObj->Configuration.size() == 0) {
    CDBError("Error! dataSource->srvParams->configObj->Configuration.size() == 0");
    return 1;
  }

  if (dataSource->cfgLayer->TileSettings.size() != 1) {
    CDBDebug("TileSettings is not set for this layer");
    return 0;
  }
  if (dataSource->cfgLayer->Dimension.size() == 0) {
    if (CAutoConfigure::autoConfigureDimensions(dataSource) != 0) {
      CREPORT_ERROR_NODOC("Unable to configure dimensions automatically", CReportMessage::Categories::GENERAL);
      return 1;
    }
  }
  /* Find all files on disk */
  std::vector<std::string> fileList;
  CT::string filter = dataSource->cfgLayer->FilePath[0]->attr.filter.c_str();
  CT::string tailPath;
  if (scanFlags & CDBFILESCANNER_IGNOREFILTER) {
    filter = "^.*$";
  }
  try {
    fileList = CDBFileScanner::searchFileNames(dataSource->cfgLayer->FilePath[0]->value.c_str(), filter.c_str(), tailPath.c_str());
  } catch (int linenr) {
    CDBError("Exception in searchFileNames [%s] [%s]", dataSource->cfgLayer->FilePath[0]->value.c_str(), filter.c_str(), tailPath.c_str());
    return 1;
  }
  if (fileList.size() == 0) {
    CDBError("No files found");
    return 1;
  }

  CDBDebug("Found %d files", fileList.size());
  for (size_t j = 0; j < fileList.size(); j++) {
    CCreateTiles::createTilesForFile(dataSource, scanFlags, fileList[j].c_str());
  }

  return 0;
}
struct DestinationGrids {
  int level, x, y;
  f8box bbox;
};

std::vector<DestinationGrids> makeTileSet(CDataSource &dataSource) {
  std::vector<DestinationGrids> destinationGrids;
  auto tileSettings = dataSource.cfgLayer->TileSettings[0];
  int desiredWidth = tileSettings->attr.tilewidthpx.toInt();
  int desiredHeight = tileSettings->attr.tileheightpx.toInt();
  // Level 0 means no tiles
  // Level 1 is same resolution as source data
  // Level 2 is half the resolution, etc...
  int minLevel = tileSettings->attr.minlevel.empty() ? 1 : tileSettings->attr.minlevel.toInt();
  int maxLevel = tileSettings->attr.maxlevel.empty() ? 3 : tileSettings->attr.maxlevel.toInt();
  double cellSizeX = fabs(dataSource.dfCellSizeX);
  double cellSizeY = fabs(dataSource.dfCellSizeY);
  double xminB = std::min(dataSource.dfBBOX[0], dataSource.dfBBOX[2]);
  double xmaxB = std::max(dataSource.dfBBOX[0], dataSource.dfBBOX[2]);
  double yminB = std::min(dataSource.dfBBOX[1], dataSource.dfBBOX[3]);
  double ymaxB = std::max(dataSource.dfBBOX[1], dataSource.dfBBOX[3]);
  for (int level = minLevel; level < (maxLevel + 1); level++) {
    int inc = pow(2, level - 1);
    double newSpanX = desiredWidth * cellSizeX * inc;
    double newSpanY = desiredHeight * cellSizeY * inc;
    for (double y = yminB; y < ymaxB; y += newSpanY) {
      for (double x = xminB; x < xmaxB; x += newSpanX) {
        int dx = round((x - xminB) / newSpanX);
        int dy = round((y - yminB) / newSpanY);
        destinationGrids.push_back({.level = level, .x = dx, .y = dy, .bbox = {.left = x, .bottom = y, .right = x + newSpanX, .top = y + newSpanY}});
      }
    }
  }
  return destinationGrids;
}

int CCreateTiles::createTilesForFile(CDataSource *baseDataSource, int, CT::string fileToTile) {
  if (fileToTile.endsWith("tile.nc")) {
    return 0;
  }
  if (baseDataSource->cfgLayer->TileSettings.size() != 1) {
    CDBDebug("No tile settings");
    return 1;
  }
  auto srvParam = baseDataSource->srvParams;
  auto tileSettings = baseDataSource->cfgLayer->TileSettings[0];
  auto db = CDBFactory::getDBAdapter(srvParam->cfg);
  auto tableName = db->getTableNameForPathFilterAndDimension(baseDataSource);

  CDataSource *dataSourceToTile = new CDataSource();
  dataSourceToTile->setCFGLayer(baseDataSource->srvParams, baseDataSource->cfg, baseDataSource->cfgLayer, baseDataSource->getLayerName(), 0);

  CDataReader reader;
  reader.silent = true;

  if (dataSourceToTile->getNumTimeSteps() != 0) {
    CDBError("dataSourceToTile->getNumTimeSteps() should be 0");
    return 1;
  }

  dataSourceToTile->addStep(fileToTile.c_str());
  dataSourceToTile->setTimeStep(0);

  CDBDebug("Opening input file for tiles: %s", dataSourceToTile->getFileName());
  int status = reader.open(dataSourceToTile, CNETCDFREADER_MODE_OPEN_HEADER);
  if (status != 0) {
    CDBError("Unable to open input file for tiles: %s", dataSourceToTile->getFileName());
    return 1;
  }

  // Extract time and set it.
  try {
    auto var = dataSourceToTile->getFirstAvailableDataObject()->cdfObject->getVariable("time");
    double timeValue = var->getDataAt<double>(0);
    auto adagucTime = CTime::GetCTimeInstance(var);
    auto timeString = adagucTime->dateToISOString(adagucTime->getDate(timeValue));
    dataSourceToTile->requiredDims.push_back(new COGCDims("time", timeString));
    dataSourceToTile->getCDFDims()->addDimension("time", timeString, 0);
  } catch (int e) {
    if (dataSourceToTile->requiredDims.size() == 0) {
      COGCDims *ogcDim = new COGCDims();
      dataSourceToTile->requiredDims.push_back(ogcDim);
      ogcDim->name.copy("none");
      ogcDim->value.copy("0");
      ogcDim->netCDFDimName.copy("none");
      ogcDim->hidden = true;
      dataSourceToTile->getCDFDims()->addDimension("none", "0", 0);
    }
    // Source data has no time dimension.
  }

  // Make the tileset
  std::vector<DestinationGrids> tileSet = makeTileSet(*dataSourceToTile);

  // Write tiles
  CT::string basename = fileToTile.basename();
  basename = basename.substring(0, basename.lastIndexOf("."));
  CT::string tileBasePath = fileToTile.substring(0, fileToTile.lastIndexOf("/"));
  if (tileSettings->attr.tilepath.empty() == false) {
    tileBasePath = tileSettings->attr.tilepath;
    tileBasePath = CDirReader::makeCleanPath(tileBasePath.c_str());
    if (!CDirReader::isDir(tileBasePath)) {

      CDirReader::makePublicDirectory(tileBasePath);
    }
  }

  srvParam->geoParams.BBOX_CRS = baseDataSource->nativeProj4.c_str();
  srvParam->geoParams.CRS = srvParam->geoParams.BBOX_CRS;
  srvParam->WCS_GoNative = false;
  srvParam->geoParams.dWidth = tileSettings->attr.tilewidthpx.toInt();
  srvParam->geoParams.dHeight = tileSettings->attr.tileheightpx.toInt();

  int index = 0;
  CT::string suffix;
  suffix.print("_tmp%d", getpid());
  for (auto destGrid : tileSet) {
    index++;
    CT::string destFileName;
    destFileName.print("%s/%s-%0.3d_%0.3d_%0.3dtile.nc", tileBasePath.c_str(), basename.c_str(), destGrid.level, destGrid.y, destGrid.x);
    CT::string tmpFile = destFileName;
    tmpFile.concat(suffix);
    // Only write if the file is not there already
    if (CDirReader::isFile(destFileName.c_str())) {
      continue;
    }
    CDBDebug("Generating  %s %0.1f done", destFileName.basename().c_str(), (index / double(tileSet.size())) * 100.);
    srvParam->geoParams.bbox = destGrid.bbox;
    CNetCDFDataWriter wcsWriter;
    wcsWriter.silent = true;
    wcsWriter.init(srvParam, dataSourceToTile, dataSourceToTile->getNumTimeSteps());
    std::vector<CDataSource *> dataSourcesToTile = {dataSourceToTile};
    wcsWriter.addData(dataSourcesToTile);
    wcsWriter.writeFile(tmpFile.c_str(), destGrid.level, true);

    rename(tmpFile.c_str(), destFileName.c_str());

    // Scan the file, add it to the db
    CDBFileScanner::scanFile(destFileName, dataSourceToTile, CDBFILESCANNER_DONTREMOVEDATAFROMDB | CDBFILESCANNER_UPDATEDB | CDBFILESCANNER_IGNOREFILTER | CDBFILESCANNER_DONOTTILE);
    // Close the created tile cdfObject
    CDFObjectStore::getCDFObjectStore()->deleteCDFObject(destFileName.c_str());
    CDFObjectStore::getCDFObjectStore()->deleteCDFObject(tmpFile.c_str());
  }
  // Close the source data
  reader.close();
  delete dataSourceToTile;

  // Should we really close the source data? For now we do.
  CDFObjectStore::getCDFObjectStore()->deleteCDFObject(fileToTile.c_str());

  return 0;
};
