#ifndef LAYER_METADATA_STORE_UTILS_H
#define LAYER_METADATA_STORE_UTILS_H

#include <string>
#include <vector>
#include "CXMLGen.h"
#include <json_adaguc.h>

json getDimensionListAsJson(std::vector<LayerMetadataDim> &dimList);

int getLayerBaseMetadataAsJson(MetadataLayer *myMetadataLayer, json &layerMetadataItem);
int getProjectionListAsJson(MetadataLayer *myMetadataLayer, json &projsettings);
int getStyleListMetadataAsJson(MetadataLayer *myMetadataLayer, json &styleListJson);

std::string getLayerMetadataFromDb(MetadataLayer *myMetadataLayer, const std::string &metadataKey);

int loadLayerMetadataStructFromMetadataDb(MetadataLayer *layer);

int loadLayerProjectionAndExtentListFromMetadataDb(MetadataLayer *layer);

int loadLayerDimensionListFromMetadataDb(MetadataLayer *layer);

int loadLayerStyleListFromMetadataDb(MetadataLayer *layer);

int updateMetaDataTable(CDataSource *dataSource);
int updateMetaDataTableLiveUpdate(CDataSource *dataSource);
#endif