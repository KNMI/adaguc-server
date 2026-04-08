#include "LayerMetadataToJson.h"
#include <Types/LayerMetadataType.h>
#include "LayerMetadataStore.h"
#include <CDBStore.h>
#include <CDBFactory.h>
#include "LayerUtils.h"
#include "CRequestUtils.h"
#include "XMLGenUtils.h"

CT::string getBlob(CDBStore::Store *layerMetaDataStore, const char *datasetName, const char *layerName, const char *metadataKey) {
  auto records = layerMetaDataStore->getRecords();
  for (auto record: records) {
    if (record.get("datasetname")->equals(datasetName) && record.get("layername")->equals(layerName) && record.get("metadatakey")->equals(metadataKey)) {
#ifdef MEASURETIME
      StopWatch_Stop("<CDBAdapterPostgreSQL::getLayerMetadata");
#endif
      return record.get("blob");
    }
  }
  return "";
}

std::map<std::string, std::vector<std::string>> getAllDimensionCombinationsFromDb(CDataSource &dataSource) {
  std::map<std::string, std::vector<std::string>> dimensionNameAndValues;
  try {
    CRequest::fillDimValuesForDataSource(&dataSource, dataSource.srvParams);
    // Find time dim for this datasource
    auto it = std::find_if(dataSource.requiredDims.begin(), dataSource.requiredDims.end(), [](const auto &a) { return a.name == "time"; });
    int timeDimIndexInRequiredDims = it != dataSource.requiredDims.end() ? std::distance(std::begin(dataSource.requiredDims), it) : -1;
    if (timeDimIndexInRequiredDims == -1) {
      return {};
    }
    // Set the time dim to *
    // dataSource.requiredDims[timeDimIndexInRequiredDims].value = "*";
    CDBStore::Store *store = CDBFactory::getDBAdapter(dataSource.srvParams->cfg)->getFilesAndIndicesForDimensions(&dataSource, getMaxQueryLimit(dataSource), false);
    if (store == nullptr || store->getSize() == 0) {
      CDBError("No result for dim set");
      throw InvalidDimensionValue;
    }
    for (size_t d = 0; d < dataSource.requiredDims.size(); d++) {
      for (size_t k = 0; k < store->getSize(); k++) {
        const auto &reqDim = dataSource.requiredDims[d];
        std::string dimValueFromDb = store->getRecord(k)->get(1 + d * 2)->c_str();
        dimensionNameAndValues[reqDim.name].push_back(dimValueFromDb);
      }
    }
    delete store;
  } catch (ServiceExceptionCode e) {
    CDBDebug("Unable to fillDimValuesForDataSource");
    return {};
  }
  return dimensionNameAndValues;
}

int getLayerMetadataAsJson(CServerParams *srvParams, json &result) {
  json dataset;
  json layer;
  CDBStore::Store *layerMetaDataStore = CDBFactory::getDBAdapter(srvParams->cfg)->getLayerMetadataStore(nullptr);
  if (layerMetaDataStore == nullptr) {
    return 1;
  }
  auto records = layerMetaDataStore->getRecords();
  std::map<std::string, std::set<std::string>> datasetNames;
  for (auto record: records) {
    CT::string *datasetName = record.get("datasetname");
    CT::string *layerName = record.get("layername");
    if (datasetName != nullptr && layerName != nullptr) {
      datasetNames[datasetName->c_str()].insert(layerName->c_str());
    }
  }

  std::string layerNameInRequest;
  if (srvParams->requestedLayerNames.size() > 0) {
    layerNameInRequest = srvParams->requestedLayerNames[0];
  }

  // Parse http://localhost:8080/adagucserver?dataset=adaguc.tests.wikgeneric&&service=wms&version=1.3.0&request=getmetadata&format=application/json&DIM_REFERENCE_TIME=2025-11-21T00:00:00Z

  std::string hasReferenceTimeValue;
  auto it = std::find_if(srvParams->requestDims.begin(), srvParams->requestDims.end(), [](const auto &a) { return a.name == "REFERENCE_TIME"; });
  if (it != srvParams->requestDims.end()) {
    hasReferenceTimeValue = (*it).value;
  }

  CDBDebug("hasReferenceTimeValue: %s", hasReferenceTimeValue.c_str());

  for (auto [dataSetName, layers]: datasetNames) {
    if (srvParams->datasetLocation.empty() || srvParams->datasetLocation.equals(dataSetName)) {
      json datasetJSON;
      for (auto layerName: layers) {
        if (layerNameInRequest.empty() || layerNameInRequest == layerName) {

          try {
            json layer;
            json a;
            layer["layer"] = a.parse(getBlob(layerMetaDataStore, dataSetName.c_str(), layerName.c_str(), "layermetadata").c_str());

            // Find layer and create datasource
            auto it = hasReferenceTimeValue.empty() ? srvParams->cfg->Layer.end()
                                                    : std::find_if(srvParams->cfg->Layer.begin(), srvParams->cfg->Layer.end(), [&layerName](auto *a) { return makeUniqueLayerName(a) == layerName; });

            if (it != srvParams->cfg->Layer.end()) {
              if ((*it)->Variable.size() == 0) {
                CDBError("No variables defined");
                return 1;
              }
              CDataSource dataSource;
              auto index = it - srvParams->cfg->Layer.begin();
              dataSource.setCFGLayer(srvParams, (*it), index);
              if (dataSource.dataObjects.size() == 0) {
                CDBError("No dataobjects defined for %s", dataSource.layerName.c_str());
                return 1;
              }
              try {
                auto dimCombiAndData = getAllDimensionCombinationsFromDb(dataSource);
                std::vector<LayerMetadataDim> dimList;
                getDimsForLayer(&dataSource, dimList, dimCombiAndData);
                getDimensionListAsJson(dimList, layer["dims"]);
              } catch (ServiceExceptionCode e) {
                            }
            } else {
              layer["dims"] = a.parse(getBlob(layerMetaDataStore, dataSetName.c_str(), layerName.c_str(), "dimensionlist").c_str());
            }
            // layer["styles"] = a.parse(getBlob(layerMetaDataStore, dataSetName.c_str(), layerName.c_str(), "stylelist").c_str());
            // layer["projected_extents"] = a.parse(getBlob(layerMetaDataStore, dataSetName.c_str(), layerName.c_str(), "projected_extents").c_str());

            datasetJSON[layerName.c_str()] = layer;

          } catch (json::exception &e) {
            CDBWarning("Unable to parse json for layer %s", layerName.c_str());
          }
        }
      }

      result[dataSetName.c_str()] = datasetJSON;
    }
  }

  return 0;
}