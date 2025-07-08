#include "CDataSource.h"
#include "CImageWarper.h"
#include "CDBFactory.h"
#include "handleTileRequest.h"

CDBStore::Store *handleTileRequest(CDataSource *dataSource) {
  auto srvParam = dataSource->srvParams;
  CT::string nativeProj4 = dataSource->cfgLayer->TileSettings[0]->attr.tileprojection.c_str();
  CImageWarper warper;
  int status = warper.initreproj(nativeProj4.c_str(), srvParam->Geo, &srvParam->cfg->Projection);
  if (status != 0) {
    CDBError("Unable to initialize projection ");
    return nullptr;
  }
  bool tileSettingsDebug = dataSource->cfgLayer->TileSettings[0]->attr.debug.equals("true");

  int maxTilesInImage = !dataSource->cfgLayer->TileSettings[0]->attr.maxtilesinimage.empty() ? dataSource->cfgLayer->TileSettings[0]->attr.maxtilesinimage.toInt() : 300;
  if (!dataSource->cfgLayer->TileSettings[0]->attr.maxtilesinimage.empty()) {
    maxTilesInImage = dataSource->cfgLayer->TileSettings[0]->attr.maxtilesinimage.toInt();
  }

  dataSource->nativeViewPortBBOX[0] = srvParam->Geo->dfBBOX[0];
  dataSource->nativeViewPortBBOX[1] = srvParam->Geo->dfBBOX[1];
  dataSource->nativeViewPortBBOX[2] = srvParam->Geo->dfBBOX[2];
  dataSource->nativeViewPortBBOX[3] = srvParam->Geo->dfBBOX[3];

  findExtentForTiles(nativeProj4.c_str(), srvParam, dataSource->nativeViewPortBBOX);

  // CDBDebug("New extent %f %f %f %f", dataSource->nativeViewPortBBOX[0], dataSource->nativeViewPortBBOX[1], dataSource->nativeViewPortBBOX[2], dataSource->nativeViewPortBBOX[3]);

  dataSource->queryLevel = -1;
  auto store = CDBFactory::getDBAdapter(srvParam->cfg)->getFilesAndIndicesForDimensions(dataSource, maxTilesInImage, false);
  std::map<int, int> levelMap;
  for (size_t j = 0; j < store->size(); j++) {
    auto record = store->getRecord(j);
    levelMap[record->get("adaguctilinglevel")->toInt()]++;
  }
  delete store;

  using mypair = std::pair<int, int>;
  std::vector<mypair> v(std::begin(levelMap), std::end(levelMap));
  sort(begin(v), end(v), [](const mypair &a, const mypair &b) { return fabs(9 - a.second) < fabs(9 - b.second); });

  dataSource->queryLevel = v.at(0).first;

  CDBDebug("using level %d", dataSource->queryLevel);

  store = CDBFactory::getDBAdapter(srvParam->cfg)->getFilesAndIndicesForDimensions(dataSource, maxTilesInImage, false);
  if (store == NULL) {
    CDBError("Unable to query bbox for tiles");
    return nullptr;
  }
  if (store->getSize() == 0) {
    CDBDebug("Found no tiles");
    delete store;
  }
  if (tileSettingsDebug) {
    srvParam->mapTitle.print("level %d, tiles %d", dataSource->queryLevel, store->getSize());
  }
  return store;
}

int findExtentForTiles(const char *srcProj4Str, CServerParams *srvParam, double nativeViewPortBBOX[4]) {
  CImageWarper warper;
  int status = warper.initreproj(srcProj4Str, srvParam->Geo, &srvParam->cfg->Projection);
  if (status != 0) {
    warper.closereproj();
    if (status != 0) {
      warper.closereproj();
      return 1;
    }
  }

  double bbStepX = (nativeViewPortBBOX[2] - nativeViewPortBBOX[0]) / 100.;
  double bbStepY = (nativeViewPortBBOX[3] - nativeViewPortBBOX[1]) / 100.;

  double xLow, yLow;
  double xHigh, yHigh;
  xLow = nativeViewPortBBOX[0];
  yLow = nativeViewPortBBOX[1];
  xHigh = nativeViewPortBBOX[2];
  yHigh = nativeViewPortBBOX[3];

  bool first = false;
  for (double y = yLow; y < yHigh; y += bbStepY) {
    for (double x = xLow; x < xHigh; x += bbStepX) {

      double x1 = x, y1 = y;
      status = warper.reprojpoint(x1, y1);
      if (status == 0) {
        // CDBDebug("Testing %f %f" ,x,y);
        if (first == false) {
          nativeViewPortBBOX[0] = x1;
          nativeViewPortBBOX[1] = y1;
          nativeViewPortBBOX[2] = x1;
          nativeViewPortBBOX[3] = y1;
        } else {
          if (nativeViewPortBBOX[0] > x1) nativeViewPortBBOX[0] = x1;
          if (nativeViewPortBBOX[1] > y1) nativeViewPortBBOX[1] = y1;
          if (nativeViewPortBBOX[2] < x1) nativeViewPortBBOX[2] = x1;
          if (nativeViewPortBBOX[3] < y1) nativeViewPortBBOX[3] = y1;
        }
        first = true;
      }
    }
  }
  warper.closereproj();
  return 0;
}
