#include "UpdateLayerMetadata.h"
#include <CDebugger.h>
#include <CRequest.h>
#include "ConfigurationUtils.h"
#include <CDBFactory.h>
#include <CDBStore.h>
#include <json_adaguc.h>
#include "LayerUtils.h"

void serverLogFunctionNothing(const char *) {}

/* Set config file from environment variable ADAGUC_CONFIG */
int setCRequestConfigFromEnvironment(CRequest *request, const char *additionalDataset) {
  char *configfile = getenv("ADAGUC_CONFIG");
  if (configfile != NULL) {
    CT::string configWithAdditionalDataset = configfile;
    if (additionalDataset != nullptr && strlen(additionalDataset) > 0) {
      configWithAdditionalDataset.concat(",");
      configWithAdditionalDataset.concat(additionalDataset);
    }
    int status = request->setConfigFile(configWithAdditionalDataset.c_str());

    /* Check logging level */
    if (request->getServerParams()->isDebugLoggingEnabled() == false) {
      setDebugFunction(serverLogFunctionNothing);
    }

    return status;
  } else {
    CDBError("No configuration file is set. Please set ADAGUC_CONFIG environment variable accordingly.");
    return 1;
  }
  return 0;
}

int updateLayerMetadata(CRequest &request) {
  CServerParams *srvParam = request.getServerParams();

  auto datasetList = getEnabledDatasetsConfigurations(srvParam);
  // TODO: Remove datasets and layers in metadatable which don't have a matching configuration
  // TODO: Remove dimension tables which don't have a matching configuration

  std::map<std::string, std::set<std::string>> dataSetConfigsWithLayers;

  for (auto &dataset : datasetList) {
    CT::string datasetAsCTString = dataset.c_str();
    CT::string baseDataSetNameCT = datasetAsCTString.basename();
    baseDataSetNameCT.replaceSelf(".xml", "");
    std::string datasetBaseName = baseDataSetNameCT.c_str();

    if (dataset.length() > 0) {
      CDBDebug("\n\n *********************************** Updating metadatatable for dataset [%s] **************************************************", dataset.c_str());
    }
    CRequest requestPerDataset;
    // dataset = "/data/adaguc-datasets/test.uwcw_ha43_dini_5p5km_10x8.xml";
    // dataset = "/data/adaguc-datasets/testdata.xml";
    int status = setCRequestConfigFromEnvironment(&requestPerDataset, dataset.c_str());
    if (status != 0) {
      CDBError("Unable to read configuration file");
      continue;
    }
    for (auto layer : requestPerDataset.getServerParams()->cfg->Layer) {
      CT::string layerName;
      makeUniqueLayerName(&layerName, layer);
      if (isalpha(layerName.charAt(0)) == 0) {
        CT::string tmp = layerName;
        layerName.print("ID_%s", tmp.c_str());
      }

      dataSetConfigsWithLayers[datasetBaseName].insert(layerName.c_str());
    }
    CT::string layerPathToScan;
    CT::string tailPath;
    status = requestPerDataset.updatedb(&tailPath, &layerPathToScan, CDBFILESCANNER_UPDATEDB | CDBFILESCANNER_DONTREMOVEDATAFROMDB | CDBFILESCANNER_UPDATEDB_ONLYFILEFROMDEFAULTQUERY, "");
    if (status != 0) {
      CDBError("Error occured in updating the database");
      continue;
    }
    // return 0;
  }

  // Check for datasets which are not configured anymore.
  json dataset;
  json layer;

  CDBStore::Store *layerMetaDataStore = CDBFactory::getDBAdapter(srvParam->cfg)->getLayerMetadataStore(nullptr);
  if (layerMetaDataStore == nullptr) {
    return 1;
  }
  auto records = layerMetaDataStore->getRecords();
  std::map<std::string, std::set<std::string>> datasetNamesFromDB;
  for (auto record : records) {
    CT::string *datasetName = record->get("datasetname");
    CT::string *layerName = record->get("layername");
    if (datasetName != nullptr && layerName != nullptr) {
      datasetNamesFromDB[datasetName->c_str()].insert(layerName->c_str());
    }
  }

  std::map<std::string, std::set<std::string>> layersToDeleteFromMetadataTable;
  for (auto datasetFromDB : datasetNamesFromDB) {
    for (auto layerNameDb : datasetFromDB.second) {
      bool hasLayer = false;
      for (auto datasetConfig : dataSetConfigsWithLayers) {
        if (datasetConfig.first == datasetFromDB.first) {
          for (auto layerNameConfig : datasetConfig.second) {
            if (layerNameConfig == layerNameDb) {
              hasLayer = true;
              break;
            }
          }
        }
      }
      if (!hasLayer) {
        // CDBDebug("NO: %s/%s ", datasetFromDB.first.c_str(), layerNameDb.c_str());
        layersToDeleteFromMetadataTable[datasetFromDB.first].insert(layerNameDb);
      } else {
        // CDBDebug("YES: %s/%s ", datasetFromDB.first.c_str(), layerNameDb.c_str());
      }
    }
  }

  for (auto dataset : layersToDeleteFromMetadataTable) {
    for (auto layer : dataset.second) {
      CDBDebug("DROP: %s/%s ", dataset.first.c_str(), layer.c_str());
      CDBFactory::getDBAdapter(srvParam->cfg)->dropLayerFromLayerMetadataStore(dataset.first.c_str(), layer.c_str());
    }
  }

  return 0;
}
