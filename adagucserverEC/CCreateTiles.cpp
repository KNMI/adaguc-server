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
  double bbox[4];
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

    for (double x = xminB; x < xmaxB; x += newSpanX) {
      for (double y = yminB; y < ymaxB; y += newSpanY) {
        int dx = round((x - xminB) / newSpanX);
        int dy = round((y - yminB) / newSpanY);
        destinationGrids.push_back({.level = level, .x = dx, .y = dy, .bbox = {x, y, x + newSpanX, y + newSpanY}});
      }
    }
  }
  return destinationGrids;
}

int CCreateTiles::createTilesForFile(CDataSource *dataSource, int, CT::string fileToTile) {
  if (fileToTile.endsWith("tile.nc")) {
    CDBDebug("Ends with a tile %s", fileToTile.c_str());
    return 0;
  }
  auto dataSourceToTile = dataSource;
  if (dataSourceToTile->cfgLayer->TileSettings.size() != 1) {
    CDBDebug("No tile settings");
    return 1;
  }
  auto srvParam = dataSourceToTile->srvParams;
  auto tileSettings = dataSourceToTile->cfgLayer->TileSettings[0];
  CT::string dataSourceFileName = dataSourceToTile->getFileName();
  if (dataSourceFileName.endsWith("tile.nc")) {
    CDBDebug("Seems a tile %s", dataSourceFileName.c_str());
    return 0;
  }
  // CDBDebug("%f %f %f %f %f %f %s", dataSourceToTile->dfBBOX[0], dataSourceToTile->dfBBOX[1], dataSourceToTile->dfBBOX[2], dataSourceToTile->dfBBOX[3], dataSourceToTile->dfCellSizeX,
  //          dataSourceToTile->dfCellSizeY, dataSourceToTile->nativeProj4.c_str());

  CRequest::fillDimValuesForDataSource(dataSourceToTile, srvParam);

  dataSource->queryLevel = 0;
  CRequest::queryDimValuesForDataSource(dataSourceToTile, srvParam, false);
  dataSourceFileName = dataSourceToTile->getFileName();
  if (dataSourceFileName.endsWith("tile.nc")) {
    CDBDebug("Seems a tile %s", dataSourceFileName.c_str());
    return 0;
  }
  std::vector<DestinationGrids> tileSet = makeTileSet(*dataSourceToTile);

  // Write tiles

  CT::string basename = dataSourceFileName.basename();
  basename.substring(0, basename.lastIndexOf("."));
  CT::string directory = dataSourceToTile->cfgLayer->FilePath[0]->value;

  srvParam->Geo->BBOX_CRS = tileSettings->attr.tileprojection;
  srvParam->Geo->CRS = srvParam->Geo->BBOX_CRS;
  srvParam->WCS_GoNative = false;
  srvParam->Geo->dWidth = tileSettings->attr.tilewidthpx.toInt();
  srvParam->Geo->dHeight = tileSettings->attr.tileheightpx.toInt();

  CDBDebug("Start making tiles %d", tileSet.size());
  int index = 0;
  for (auto destGrid : tileSet) {

    CT::string destFileName;
    destFileName.print("%s/%s-%0.3d_%0.3d_%0.3dtile.nc", directory.c_str(), basename.c_str(), destGrid.level, destGrid.y, destGrid.x);

    index++;
    if (CDirReader::isFile(destFileName.c_str())) {
      CDBDebug("Already there %s", destFileName.c_str());
      continue;
    }
    CDBDebug("Doing tile %s %0.1f procent done", destFileName.basename().c_str(), (index / double(tileSet.size())) * 100.);
    for (size_t b = 0; b < 4; b++) {
      srvParam->Geo->dfBBOX[b] = destGrid.bbox[b];
    }
    CNetCDFDataWriter wcsWriter;
    wcsWriter.init(srvParam, dataSourceToTile, dataSourceToTile->getNumTimeSteps());
    std::vector<CDataSource *> dataSourcesToTile = {dataSourceToTile};
    wcsWriter.addData(dataSourcesToTile);
    wcsWriter.writeFile(destFileName.c_str(), destGrid.level, true);
    CDBDebug("Starting updatedb from createTiles for "
             "file %s",
             destFileName.c_str());

    std::vector<std::string> fileList = {destFileName.c_str()};
    auto dataSourceToScan = dataSourceToTile->clone();
    int status = CDBFileScanner::DBLoopFiles(dataSourceToScan, 0, &fileList, CDBFILESCANNER_DONTREMOVEDATAFROMDB | CDBFILESCANNER_UPDATEDB | CDBFILESCANNER_IGNOREFILTER | CDBFILESCANNER_DONOTTILE);
    delete dataSourceToScan;
    if (status != 0) {
      CDBDebug("Something went wrong.");
      throw(__LINE__);
    }
  }
  // delete dataSourceToTile;

  return 0;
};
