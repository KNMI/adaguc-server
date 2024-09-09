#ifndef LAYER_METADATA_STORE_UTILS_H
#define LAYER_METADATA_STORE_UTILS_H

#include <string>
#include <vector>
#include "CXMLGen.h"
#include <json_adaguc.h>

int getDimensionListAsJson(WMSLayer *myWMSLayer, json &dimListJson);
int getLayerMetadataAsJson(WMSLayer *myWMSLayer, json &layerMetadataItem);
int getProjectionListAsJson(WMSLayer *myWMSLayer, json &projsettings);
int getStyleListMetadataAsJson(WMSLayer *myWMSLayer, json &styleListJson);

int storeLayerMetadataInDb(WMSLayer *myWMSLayer, CT::string metadataKey, std::string metadataBlob);
CT::string getLayerMetadataFromDb(WMSLayer *myWMSLayer, CT::string metadataKey);

int storeMyWMSLayerIntoMetadataDb(WMSLayer *myWMSLayer);
int loadMyWMSLayerFromMetadataDb(WMSLayer *myWMSLayer);

int storeLayerMetadataStructIntoMetadataDb(WMSLayer *layer);
int loadLayerMetadataStructFromMetadataDb(WMSLayer *layer);

int storeLayerProjectionAndExtentListIntoMetadataDb(WMSLayer *layer);
int loadLayerProjectionAndExtentListFromMetadataDb(WMSLayer *layer);

int storeLayerDimensionListIntoMetadataDb(WMSLayer *myWMSLayer);
int loadLayerDimensionListFromMetadataDb(WMSLayer *layer);

int storeLayerStyleListIntoMetadataDb(WMSLayer *myWMSLayer);
int loadLayerStyleListFromMetadataDb(WMSLayer *layer);

int updateMetaDataTable(CDataSource *dataSource);
#endif