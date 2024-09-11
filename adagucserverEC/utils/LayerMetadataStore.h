#ifndef LAYER_METADATA_STORE_UTILS_H
#define LAYER_METADATA_STORE_UTILS_H

#include <string>
#include <vector>
#include "CXMLGen.h"

int storeLayerMetadataInDb(MetadataLayer *myWMSLayer, CT::string metadataKey, std::string metadataBlob);
CT::string getLayerMetadataFromDb(MetadataLayer *myWMSLayer, CT::string metadataKey);

int storeMyWMSLayerIntoMetadataDb(MetadataLayer *myWMSLayer);

int storeLayerMetadataStructIntoMetadataDb(MetadataLayer *layer);
int loadLayerMetadataStructFromMetadataDb(MetadataLayer *layer);

int storeLayerProjectionAndExtentListIntoMetadataDb(MetadataLayer *layer);
int loadLayerProjectionAndExtentListFromMetadataDb(MetadataLayer *layer);

int storeLayerDimensionListIntoMetadataDb(MetadataLayer *myWMSLayer);
int loadLayerDimensionListFromMetadataDb(MetadataLayer *layer);

int storeLayerStyleListIntoMetadataDb(MetadataLayer *myWMSLayer);
int loadLayerStyleListFromMetadataDb(MetadataLayer *layer);

int updateMetaDataTable(CDataSource *dataSource);
#endif