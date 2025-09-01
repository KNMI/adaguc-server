#include "CDataSource.h"
#include "CImageWarper.h"
#include "CDBFactory.h"
#include "handleTileRequest.h"
#include "Types/LayerMetadataType.h"
#include "utils/LayerMetadataStore.h"

#define DEFAULT_MAX_TILES_IN_IMAGE 16
#define DEFAULT_REQUEST_LIMIT 1000

CT::string getProjStringFromDb(CDataSource *dataSource) {
  // Obtain same proj string as base datasource
  MetadataLayer metadataLayer;
  metadataLayer.layer = dataSource->cfgLayer;
  metadataLayer.dataSource = dataSource;
  metadataLayer.srvParams = dataSource->srvParams;
  metadataLayer.readFromDb = true;
  loadLayerMetadataStructFromMetadataDb(&metadataLayer);
  return metadataLayer.layerMetadata.projstring;
}

int estimateNrOfTargetTiles(CDataSource *dataSource) {
  // Estimate number of needed target tiles to match number of grid cells in source and destination
  auto srvParam = dataSource->srvParams;
  auto tileSettings = dataSource->cfgLayer->TileSettings[0];
  int targetNrOfTilesX = floor((srvParam->Geo->dWidth / tileSettings->attr.tilewidthpx.toInt()) + 0.0) + 2;
  int targetNrOfTilesY = floor((srvParam->Geo->dHeight / tileSettings->attr.tileheightpx.toInt()) + 0.0) + 2;
  return targetNrOfTilesX * targetNrOfTilesY;
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

CDBStore::Store *handleTileRequest(CDataSource *dataSource) {
  auto srvParam = dataSource->srvParams;
  auto tileSettings = dataSource->cfgLayer->TileSettings[0];
  bool tileSettingsDebug = tileSettings->attr.debug.equals("true");
  int initialRequestLimit = DEFAULT_REQUEST_LIMIT;
  size_t maxTilesInImage = !tileSettings->attr.maxtilesinimage.empty() ? tileSettings->attr.maxtilesinimage.toInt() : DEFAULT_MAX_TILES_IN_IMAGE;

  // Estimate number of needed target tiles to match number of grid cells in source and destination
  int targetNrOfTiles = estimateNrOfTargetTiles(dataSource);

  // Obtain same proj string as base datasource
  CT::string tileprojection = getProjStringFromDb(dataSource);

  // Obtain requested bbox
  f8box inputbox;
  if (!srvParam->dFound_BBOX) {
    // If no serverbbox was given, set a default based on tileinfo from the db.
    try {
      inputbox = CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->getExtent(dataSource);
      inputbox.toArray(srvParam->Geo->dfBBOX);
      srvParam->Geo->CRS = tileprojection;
    } catch (int e) {
    }
  } else {
    inputbox = srvParam->Geo->dfBBOX;
  }

  // Query for all possible tiles within given domain by nativeViewPortBBOX
  dataSource->nativeViewPortBBOX = reprojectExtent(tileprojection, srvParam->Geo->CRS, srvParam, inputbox);
  dataSource->queryLevel = -1; // All tiles
  dataSource->queryBBOX = true;
  auto store = CDBFactory::getDBAdapter(srvParam->cfg)->getFilesAndIndicesForDimensions(dataSource, initialRequestLimit, false);
  if (store->size() == 0) {
    CDBError("No tiles found");
    delete store;
    return nullptr;
  }

  // Put the results per tiling level into a map, so we can find the closest matching tile level.
  std::map<int, int> levelMap;
  for (auto record : store->records) {
    auto tilingLevel = record.get("adaguctilinglevel")->toInt();
    if (tilingLevel > 0) {
      levelMap[tilingLevel]++;
    }
  }

  // Sort the map so that the closest targetNrOfTiles is first, this is the tiling level target
  using mypair = std::pair<int, int>;
  std::vector<mypair> v(std::begin(levelMap), std::end(levelMap));

  // If there are no tiled versions found, just pick the first result as querylevel. This is normally the untiled version
  if (v.size() == 0) {
    dataSource->queryLevel = store->records.at(0).get("adaguctilinglevel")->toInt();
  } else {
    // Find tiling level that has amount of tiles closest to targetNrOfTiles. If equal amount of tiles occur for tiling levels, the lowest tilinglevel is taken.
    std::stable_sort(begin(v), end(v), [targetNrOfTiles](const mypair &a, const mypair &b) { return fabs(targetNrOfTiles - a.second) < fabs(targetNrOfTiles - b.second); });
    dataSource->queryLevel = v.at(0).first;
  }

  // Now filter out the unwanted tiling levels from the list
  std::vector<CDBStore::Record> filteredRecords;
  for (auto record : store->records) {
    // Make sure that there are not too many tiles
    if (filteredRecords.size() >= maxTilesInImage) {
      break;
    }
    if (record.get("adaguctilinglevel")->toInt() == dataSource->queryLevel) {
      filteredRecords.push_back(record);
    }
  }

  // Assign the filtered records
  store->records = filteredRecords;

  if (tileSettingsDebug) {
    srvParam->mapTitle.print("level %d, tiles %d targetNrOfTiles %d", dataSource->queryLevel, store->getSize(), targetNrOfTiles);
  }

  // Return the store for CRequest.
  return store;
}
