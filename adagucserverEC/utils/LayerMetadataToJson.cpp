#include "LayerMetadataToJson.h"
#include <Types/LayerMetadataType.h>
#include "LayerMetadataStore.h"

int getLayerMetadataAsJson(CDataSource *dataSource, json &result) {
  json dataset;
  json layer;

  WMSLayer *myWMSLayer = new WMSLayer();
  myWMSLayer->readFromDb = true;
  myWMSLayer->layer = dataSource->cfgLayer;
  myWMSLayer->srvParams = dataSource->srvParams;
  myWMSLayer->dataSource = dataSource;
  loadMyWMSLayerFromMetadataDb(myWMSLayer);

  json dimListJson, layerMetadataItem, projsettings, styleListJson;

  getDimensionListAsJson(myWMSLayer, dimListJson);
  getLayerBaseMetadataAsJson(myWMSLayer, layerMetadataItem);
  getProjectionListAsJson(myWMSLayer, projsettings);
  getStyleListMetadataAsJson(myWMSLayer, styleListJson);
  CT::string datasetName = myWMSLayer->dataSource->srvParams->datasetLocation;
  if (datasetName.empty()) {
    CDBDebug("Not a dataset");
    return 1;
  }
  CT::string layerName = myWMSLayer->dataSource->getLayerName();

  layer["dims"] = dimListJson;
  layer["layer"] = layerMetadataItem;
  layer["projections"] = projsettings;
  layer["styles"] = styleListJson;
  dataset[layerName.c_str()] = layer;
  result[datasetName.c_str()] = dataset;
  return 0;
}