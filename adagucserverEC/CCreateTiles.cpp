#include "CCreateTiles.h"
#include "CDBAdapter.h"
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
  double cellSizeX = fabs(dataSource.dfCellSizeX);
  double cellSizeY = fabs(dataSource.dfCellSizeY);
  double xminB = std::min(dataSource.dfBBOX[0], dataSource.dfBBOX[2]);
  double xmaxB = std::max(dataSource.dfBBOX[0], dataSource.dfBBOX[2]);
  double yminB = std::min(dataSource.dfBBOX[1], dataSource.dfBBOX[3]);
  double ymaxB = std::max(dataSource.dfBBOX[1], dataSource.dfBBOX[3]);
  for (int level = 1; level < 3; level++) {
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

  dataSourceToTile->addStep(fileToTile.c_str(), NULL);
  dataSourceToTile->setTimeStep(0);

  CDBDebug("Opening input file for tiles: %s", dataSourceToTile->getFileName());
  reader.open(dataSourceToTile, CNETCDFREADER_MODE_OPEN_HEADER);

  // Extract time and set it.
  auto var = dataSourceToTile->getFirstAvailableDataObject()->cdfObject->getVariable("time");
  double timeValue = var->getDataAt<double>(0);
  auto adagucTime = CTime::GetCTimeInstance(var);
  auto timeString = adagucTime->dateToISOString(adagucTime->getDate(timeValue));
  dataSourceToTile->requiredDims.push_back(new COGCDims("time", timeString));
  dataSourceToTile->getCDFDims()->addDimension("time", timeString, 0);

  // Make the tileset
  std::vector<DestinationGrids> tileSet = makeTileSet(*dataSourceToTile);

  // Write tiles
  CT::string basename = fileToTile.basename();
  basename.substring(0, basename.lastIndexOf("."));
  CT::string directory = dataSourceToTile->cfgLayer->FilePath[0]->value;

  srvParam->Geo->BBOX_CRS = tileSettings->attr.tileprojection;
  srvParam->Geo->CRS = srvParam->Geo->BBOX_CRS;
  srvParam->WCS_GoNative = false;
  srvParam->Geo->dWidth = tileSettings->attr.tilewidthpx.toInt();
  srvParam->Geo->dHeight = tileSettings->attr.tileheightpx.toInt();

  int index = 0;
  for (auto destGrid : tileSet) {
    index++;
    CT::string destFileName;
    destFileName.print("%s/%s-%0.3d_%0.3d_%0.3dtile.nc", directory.c_str(), basename.c_str(), destGrid.level, destGrid.y, destGrid.x);

    // Already done?
    if (db->checkIfFileIsInTable(tableName, destFileName) == 0) {
      continue;
    }
    // Only write if the file is not there already
    if (!CDirReader::isFile(destFileName.c_str())) {
      CDBDebug("Make  %s %0.1f done", destFileName.basename().c_str(), (index / double(tileSet.size())) * 100.);
      destGrid.bbox.toArray(srvParam->Geo->dfBBOX);
      CNetCDFDataWriter wcsWriter;
      wcsWriter.silent = true;
      wcsWriter.init(srvParam, dataSourceToTile, dataSourceToTile->getNumTimeSteps());
      std::vector<CDataSource *> dataSourcesToTile = {dataSourceToTile};
      wcsWriter.addData(dataSourcesToTile);
      wcsWriter.writeFile(destFileName.c_str(), destGrid.level, true);
    }

    // Scan the file, add it to the db
    CDBFileScanner::scanFile(destFileName, dataSourceToTile, CDBFILESCANNER_DONTREMOVEDATAFROMDB | CDBFILESCANNER_UPDATEDB | CDBFILESCANNER_IGNOREFILTER | CDBFILESCANNER_DONOTTILE);

    // Close the created tile cdfObject
    CDFObjectStore::getCDFObjectStore()->deleteCDFObject(destFileName.c_str());
  }
  // Close the source data
  reader.close();
  delete dataSourceToTile;
  CDFObjectStore::getCDFObjectStore()->deleteCDFObject(fileToTile.c_str());

  return 0;
};
