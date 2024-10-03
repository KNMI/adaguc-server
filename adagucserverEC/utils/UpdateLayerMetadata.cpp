#include "UpdateLayerMetadata.h"
#include <CDebugger.h>
#include <CRequest.h>
#include "ConfigurationUtils.h"
#include <CDBFactory.h>
#include <CDBStore.h>
#include <json_adaguc.h>
#include "LayerUtils.h"

typedef std::pair<std::string, std::string> DatasetAndLayerPair;

int updateLayerMetadata(CRequest &request) {
  CServerParams *srvParam = request.getServerParams();

  auto datasetList = getEnabledDatasetsConfigurations(srvParam);
  // TODO: Remove dimension tables which don't have a matching configuration

  std::set<DatasetAndLayerPair> dataSetConfigsWithLayers;

  for (auto &dataset : datasetList) {
    CT::string datasetAsCTString = dataset.c_str();
    CT::string baseDataSetNameCT = datasetAsCTString.basename();
    baseDataSetNameCT.replaceSelf(".xml", "");
    std::string datasetBaseName = baseDataSetNameCT.c_str();

    if (dataset.length() > 0) {
      CDBDebug("\n\n *********************************** Updating metadatatable for dataset [%s] **************************************************", dataset.c_str());
    }
    CRequest requestPerDataset;

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

      dataSetConfigsWithLayers.insert(std::make_pair(datasetBaseName, layerName.c_str()));
    }
    CT::string layerPathToScan;
    CT::string tailPath;
    status = requestPerDataset.updatedb(&tailPath, &layerPathToScan, CDBFILESCANNER_UPDATEDB | CDBFILESCANNER_DONTREMOVEDATAFROMDB | CDBFILESCANNER_UPDATEDB_ONLYFILEFROMDEFAULTQUERY, "");
    if (status != 0) {
      CDBError("Error occured in updating the database");
      continue;
    }
  }

  // Check for datasets and or layers which are not configured anymore.
  json dataset;
  json layer;

  CDBStore::Store *layerMetaDataStore = CDBFactory::getDBAdapter(srvParam->cfg)->getLayerMetadataStore(nullptr);
  if (layerMetaDataStore == nullptr) {
    return 1;
  }
  auto records = layerMetaDataStore->getRecords();

  std::set<DatasetAndLayerPair> datasetNamesFromDB;

  for (auto record : records) {
    CT::string *datasetName = record->get("datasetname");
    CT::string *layerName = record->get("layername");
    if (datasetName != nullptr && layerName != nullptr) {
      datasetNamesFromDB.insert(std::make_pair(datasetName->c_str(), layerName->c_str()));
    }
  }

  std::set<DatasetAndLayerPair> layersToDeleteFromMetadataTable;

  std::set_difference(datasetNamesFromDB.begin(), datasetNamesFromDB.end(), dataSetConfigsWithLayers.begin(), dataSetConfigsWithLayers.end(),
                      std::inserter(layersToDeleteFromMetadataTable, layersToDeleteFromMetadataTable.end()));

  for (auto p : layersToDeleteFromMetadataTable) {
    CDBDebug("DROP: %s/%s ", p.first.c_str(), p.second.c_str());
    CDBFactory::getDBAdapter(srvParam->cfg)->dropLayerFromLayerMetadataStore(p.first.c_str(), p.second.c_str());
  }

  return 0;
}
