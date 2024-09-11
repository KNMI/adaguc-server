#ifndef LAYER_METADATA_STORE_UTILS_H
#define LAYER_METADATA_STORE_UTILS_H

#include <string>
#include <vector>
#include "CXMLGen.h"
#include <json_adaguc.h>

int getDimensionListAsJson(MetadataLayer *myMetadataLayer, json &dimListJson);
int getLayerBaseMetadataAsJson(MetadataLayer *myMetadataLayer, json &layerMetadataItem);
int getProjectionListAsJson(MetadataLayer *myMetadataLayer, json &projsettings);
int getStyleListMetadataAsJson(MetadataLayer *myMetadataLayer, json &styleListJson);

int storeLayerMetadataInDb(MetadataLayer *myMetadataLayer, CT::string metadataKey, std::string metadataBlob);
CT::string getLayerMetadataFromDb(MetadataLayer *myMetadataLayer, CT::string metadataKey);

int storeMyMetadataLayerIntoMetadataDb(MetadataLayer *myMetadataLayer);

int storeLayerMetadataStructIntoMetadataDb(MetadataLayer *layer);
int loadLayerMetadataStructFromMetadataDb(MetadataLayer *layer);

int storeLayerProjectionAndExtentListIntoMetadataDb(MetadataLayer *layer);
int loadLayerProjectionAndExtentListFromMetadataDb(MetadataLayer *layer);

int storeLayerDimensionListIntoMetadataDb(MetadataLayer *myMetadataLayer);
int loadLayerDimensionListFromMetadataDb(MetadataLayer *layer);

int storeLayerStyleListIntoMetadataDb(MetadataLayer *myMetadataLayer);
int loadLayerStyleListFromMetadataDb(MetadataLayer *layer);

int updateMetaDataTable(CDataSource *dataSource);
#endif