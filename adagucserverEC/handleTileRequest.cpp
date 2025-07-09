#include "CDataSource.h"
#include "CImageWarper.h"
#include "CDBFactory.h"
#include "handleTileRequest.h"

CDBStore::Store *handleTileRequest(CDataSource *dataSource) {
  auto srvParam = dataSource->srvParams;
  auto tileSettings = dataSource->cfgLayer->TileSettings[0];
  bool tileSettingsDebug = tileSettings->attr.debug.equals("true");
  int maxTilesInImage = !tileSettings->attr.maxtilesinimage.empty() ? tileSettings->attr.maxtilesinimage.toInt() : 300;

  f8box inputbox;
  inputbox = srvParam->Geo->dfBBOX;
  dataSource->nativeViewPortBBOX = reprojectExtent(tileSettings->attr.tileprojection, srvParam, inputbox);
  dataSource->queryLevel = -1;

  // Query for all possible tiles within given domain by nativeViewPortBBOX
  auto store = CDBFactory::getDBAdapter(srvParam->cfg)->getFilesAndIndicesForDimensions(dataSource, maxTilesInImage, false);

  // Put the results per tiling level into a map.
  std::map<int, int> levelMap;
  for (size_t j = 0; j < store->size(); j++) {
    auto record = store->getRecord(j);
    levelMap[record->get("adaguctilinglevel")->toInt()]++;
  }
  delete store;

  // Sort the map so that the closest targetNrOfTiles is first, this is the tiling level target
  using mypair = std::pair<int, int>;
  std::vector<mypair> v(std::begin(levelMap), std::end(levelMap));
  int targetNrOfTiles = 9;
  sort(begin(v), end(v), [targetNrOfTiles](const mypair &a, const mypair &b) { return fabs(targetNrOfTiles - a.second) < fabs(targetNrOfTiles - b.second); });
  dataSource->queryLevel = v.at(0).first;

  CDBDebug("using level %d", dataSource->queryLevel);

  // Now do the query again with the target.
  store = CDBFactory::getDBAdapter(srvParam->cfg)->getFilesAndIndicesForDimensions(dataSource, maxTilesInImage, false);
  if (store == NULL) {
    CDBError("Unable to query bbox for tiles");
    return nullptr;
  }

  if (tileSettingsDebug) {
    srvParam->mapTitle.print("level %d, tiles %d", dataSource->queryLevel, store->getSize());
  }
  return store;
}

f8box reprojectExtent(CT::string targetProjection, CServerParams *srvParam, f8box inputbox) {
  CImageWarper warper;
  if (warper.initreproj(targetProjection.c_str(), srvParam->Geo, &srvParam->cfg->Projection) != 0) {
    CDBError("Unable to init projection");
    throw __LINE__;
  }

  // A grid of numSteps * numSteps will be used to determine the new extent.
  int numSteps = 25;
  f8point step = {.x = (inputbox.right - inputbox.left) / numSteps, .y = (inputbox.top - inputbox.bottom) / numSteps};
  f8box outputbox;
  bool first = false;
  for (double y = inputbox.bottom; y < inputbox.top; y += step.y) {
    for (double x = inputbox.left; x < inputbox.right; x += step.x) {
      f8point point = {.x = x, .y = y};
      if (warper.reprojpoint(point) == 0) {
        if (first == false) {
          first = true;
          outputbox.left = point.x;
          outputbox.bottom = point.y;
          outputbox.right = point.x;
          outputbox.top = point.y;
        } else {
          if (outputbox.left > point.x) outputbox.left = point.x;
          if (outputbox.bottom > point.y) outputbox.bottom = point.y;
          if (outputbox.right < point.x) outputbox.right = point.x;
          if (outputbox.top < point.y) outputbox.top = point.y;
        }
      }
    }
  }
  return outputbox;
}
