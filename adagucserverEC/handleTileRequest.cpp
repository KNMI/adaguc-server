#include "CDataSource.h"
#include "CImageWarper.h"
#include "CDBFactory.h"
#include "handleTileRequest.h"

CDBStore::Store *handleTileRequest(CDataSource *dataSource) {
  auto srvParam = dataSource->srvParams;
  auto tileSettings = dataSource->cfgLayer->TileSettings[0];
  bool tileSettingsDebug = tileSettings->attr.debug.equals("true");
  int initialRequestLimit = 500;
  int targetNrOfTiles = 9;
  size_t maxTilesInImage = !tileSettings->attr.maxtilesinimage.empty() ? tileSettings->attr.maxtilesinimage.toInt() : 16;

  if (!srvParam->dFound_BBOX) {
    try {
      f8box box = CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->getExtent(dataSource);
      srvParam->Geo->dfBBOX[0] = box.left;
      srvParam->Geo->dfBBOX[1] = box.bottom;
      srvParam->Geo->dfBBOX[2] = box.right;
      srvParam->Geo->dfBBOX[3] = box.top;
      srvParam->Geo->CRS = tileSettings->attr.tileprojection;
    } catch (int e) {
    }
  }
  f8box inputbox;
  inputbox = srvParam->Geo->dfBBOX;
  dataSource->nativeViewPortBBOX = reprojectExtent(tileSettings->attr.tileprojection, srvParam->Geo->CRS, srvParam, inputbox);
  dataSource->queryLevel = -1; // All tiles
  dataSource->queryBBOX = true;

  // Query for all possible tiles within given domain by nativeViewPortBBOX
  auto store = CDBFactory::getDBAdapter(srvParam->cfg)->getFilesAndIndicesForDimensions(dataSource, initialRequestLimit, false);

  if (store->size() == 0) {
    CDBError("No tiles found");
    delete store;
    return nullptr;
  }
  // Put the results per tiling level into a map.
  std::map<int, int> levelMap;
  for (auto record : store->records) {
    levelMap[record.get("adaguctilinglevel")->toInt()]++;
  }

  // Sort the map so that the closest targetNrOfTiles is first, this is the tiling level target
  using mypair = std::pair<int, int>;
  std::vector<mypair> v(std::begin(levelMap), std::end(levelMap));

  sort(begin(v), end(v), [targetNrOfTiles](const mypair &a, const mypair &b) { return fabs(targetNrOfTiles - a.second) < fabs(targetNrOfTiles - b.second); });
  dataSource->queryLevel = v.at(0).first;

  CDBDebug("using level %d", dataSource->queryLevel);

  // Now filter out all the tiling levels
  std::vector<CDBStore::Record> records;
  for (auto record : store->records) {
    // Make sure that there are not too many tiles
    if (records.size() < maxTilesInImage) {
      if (record.get("adaguctilinglevel")->toInt() == dataSource->queryLevel) {
        records.push_back(record);
      }
    } else
      break;
  }

  // Assign the filtered records
  store->records = records;

  if (tileSettingsDebug) {
    srvParam->mapTitle.print("level %d, tiles %d", dataSource->queryLevel, store->getSize());
  }
  return store;
}

f8box reprojectExtent(CT::string targetProjection, CT::string sourceProjection, CServerParams *srvParam, f8box inputbox) {
  CImageWarper warper;
  if (warper.init(targetProjection.c_str(), sourceProjection.c_str(), &srvParam->cfg->Projection) != 0) {
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
