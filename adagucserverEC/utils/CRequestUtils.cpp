#include <CServerParams.h>
#include <CDataSource.h>
#include <CImageWarper.h>

#include "CRequestUtils.h"
#include "LayerUtils.h"
#include <tuple>
#include <array>
#include <CDBFactory.h>
#include <handleTileRequest.h>

std::tuple<int, f8box> findBBoxForDataSource(std::vector<CDataSource *> dataSources) {
  double dfBBOX[4] = {-180, -90, 180, 90};
  for (size_t d = 0; d < dataSources.size(); d++) {
    if (dataSources[d]->dLayerType != CConfigReaderLayerTypeGraticule && dataSources[d]->dLayerType != CConfigReaderLayerTypeBaseLayer &&
        dataSources[d]->dLayerType != CConfigReaderLayerTypeLiveUpdate) {
      CImageWarper warper;
      CDataReader reader;
      int status = reader.open(dataSources[d], CNETCDFREADER_MODE_OPEN_HEADER);
      // TODO https://github.com/KNMI/adaguc-server/issues/570

      reader.close();
      status = warper.initreproj(dataSources[d], dataSources[0]->srvParams->geoParams, &dataSources[0]->srvParams->cfg->Projection);
      if (status != 0) {
        warper.closereproj();
        CDBDebug("Unable to initialize projection ");
      }
      warper.findExtent(dataSources[d], dfBBOX);
      warper.closereproj();
      CDBDebug("Found bbox %s %f %f %f %f", dataSources[0]->srvParams->geoParams.crs.c_str(), dfBBOX[0], dfBBOX[1], dfBBOX[2], dfBBOX[3]);
      f8box box = {dfBBOX[0], dfBBOX[1], dfBBOX[2], dfBBOX[3]};
      return std::make_tuple(0, box);
    }
  }

  f8box box = {dfBBOX[0], dfBBOX[1], dfBBOX[2], dfBBOX[3]};
  return std::make_tuple(-1, box);
}

CServerConfig::XMLE_Layer *findLayerConfigForRequestedLayer(CServerParams *srvParam, CT::string requestedLayerName) {
  for (size_t layerNo = 0; layerNo < srvParam->cfg->Layer.size(); layerNo++) {
    auto cfgLayer = srvParam->cfg->Layer[layerNo];
    if (makeUniqueLayerName(cfgLayer).equals(requestedLayerName)) {
      return cfgLayer;
    }
  }
  return nullptr;
}

std::string getReferenceTimeDimName(CDataSource &dataSource) {
  auto dimList = dataSource.cfgLayer->Dimension;
  auto it = std::find_if(dimList.begin(), dimList.end(), [](const auto &dim) { return CT::toUpperCase(dim->value) == "REFERENCE_TIME"; });
  if (it != dimList.end()) {
    return (*it)->attr.name;
  }
  return "";
}

std::string makeIsoStringFromDbString(std::string input) {
  if (input.length() < 12) {
    return input;
  }
  input.at(10) = 'T';
  // 01234567890123456789
  // YYYY-MM-DDTHH:MM:SSZ
  if (input.length() == 19) {
    input += "Z";
  }
  return input;
}

std::vector<std::string> getReferenceTimes(CDataSource &dataSource) {
  auto refTimeDim = getReferenceTimeDimName(dataSource);
  if (refTimeDim.empty()) return {};
  auto srvParam = dataSource.srvParams;
  std::string tableName;
  try {
    tableName = CDBFactory::getDBAdapter(srvParam->cfg)
                    ->getTableNameForPathFilterAndDimension(dataSource.cfgLayer->FilePath[0]->value.c_str(), dataSource.cfgLayer->FilePath[0]->attr.filter.c_str(), refTimeDim.c_str(), &dataSource);
  } catch (int e) {
    CDBError("Unable to create tableName from '%s' '%s' '%s'", dataSource.cfgLayer->FilePath[0]->value.c_str(), dataSource.cfgLayer->FilePath[0]->attr.filter.c_str(), refTimeDim.c_str());
    return {};
  }

  CDBStore::Store *store = CDBFactory::getDBAdapter(srvParam->cfg)->getUniqueValuesOrderedByValue(refTimeDim.c_str(), -1, false, tableName.c_str());
  if (store == NULL) {
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for layer %s", dataSource.cfgLayer->Name[0]->value.c_str());
    return {};
  }
  std::vector<std::string> resultList;
  for (size_t k = 0; k < store->getSize(); k++) {
    resultList.push_back(makeIsoStringFromDbString(*store->getRecord(k)->get(0)));
  }
  delete store;

  return resultList;
}

int getMaxQueryLimit(CDataSource &dataSource) {
  int maxQueryResultLimit = 512;

  /* Get maxquerylimit from database configuration */
  if (dataSource.srvParams->cfg->DataBase.size() == 1 && dataSource.srvParams->cfg->DataBase[0]->attr.maxquerylimit.empty() == false) {
    maxQueryResultLimit = dataSource.srvParams->cfg->DataBase[0]->attr.maxquerylimit.toInt();
  }
  /* Get maxquerylimit from layer */
  if (dataSource.isConfigured && dataSource.cfgLayer != NULL && dataSource.cfgLayer->FilePath.size() > 0) {
    if (dataSource.cfgLayer->FilePath[0]->attr.maxquerylimit.empty() == false) {
      maxQueryResultLimit = dataSource.cfgLayer->FilePath[0]->attr.maxquerylimit.toInt();
    }
  }
  return maxQueryResultLimit;
}