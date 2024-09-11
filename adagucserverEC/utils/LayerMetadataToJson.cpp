#include "LayerMetadataToJson.h"
#include <Types/LayerMetadataType.h>
#include "LayerMetadataStore.h"
#include <CDBStore.h>
#include <CDBFactory.h>

CT::string getBlob(CDBStore::Store *layerMetaDataStore, const char *datasetName, const char *layerName, const char *metadataKey) {
  auto records = layerMetaDataStore->getRecords();
  for (auto record : records) {
    if (record->get("datasetname")->equals(datasetName) && record->get("layername")->equals(layerName) && record->get("metadatakey")->equals(metadataKey)) {
#ifdef MEASURETIME
      StopWatch_Stop("<CDBAdapterPostgreSQL::getLayerMetadata");
#endif
      return record->get("blob");
    }
  }
  return "";
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
  for (auto record : records) {
    CT::string *datasetName = record->get("datasetname");
    CT::string *layerName = record->get("layername");
    if (datasetName != nullptr && layerName != nullptr) {
      datasetNames[record->get("datasetname")->c_str()].insert(record->get("layername")->c_str());
    }
  }

  CT::string layerNameInRequest;
  if (srvParams->WMSLayers != nullptr && srvParams->WMSLayers->count > 0) {
    layerNameInRequest = srvParams->WMSLayers[0].c_str();
  }

  for (auto dataset : datasetNames) {
    auto dataSetName = dataset.first;
    if (srvParams->datasetLocation.empty() || srvParams->datasetLocation.equals(dataSetName)) {
      json datasetJSON;
      for (auto layerName : dataset.second) {
        if (layerNameInRequest.empty() || layerNameInRequest.equals(layerName.c_str())) {
          json layer;
          json a;
          layer["layer"] = a.parse(getBlob(layerMetaDataStore, dataSetName.c_str(), layerName.c_str(), "layermetadata").c_str());
          layer["dims"] = a.parse(getBlob(layerMetaDataStore, dataSetName.c_str(), layerName.c_str(), "dimensionlist").c_str());
          datasetJSON[layerName.c_str()] = layer;
        }
      }

      result[dataSetName.c_str()] = datasetJSON;
    }
  }

  return 0;
}