#ifndef LAYER_METADATA_STORE_UTILS_H
#define LAYER_METADATA_STORE_UTILS_H

#include <string>
#include <vector>
#include "CXMLGen.h"

int storeLayerMetadataInDb(MetadataLayer *metadataLayer, CT::string metadataKey, std::string metadataBlob);
CT::string getLayerMetadataFromDb(MetadataLayer *metadataLayer, CT::string metadataKey);

int storeMetadataLayerIntoMetadataDb(MetadataLayer *metadataLayer);

int storeLayerMetadataStructIntoMetadataDb(MetadataLayer *layer);
int loadLayerMetadataStructFromMetadataDb(MetadataLayer *layer);

int storeLayerProjectionAndExtentListIntoMetadataDb(MetadataLayer *layer);
int loadLayerProjectionAndExtentListFromMetadataDb(MetadataLayer *layer);

int storeLayerDimensionListIntoMetadataDb(MetadataLayer *metadataLayer);
int loadLayerDimensionListFromMetadataDb(MetadataLayer *layer);

int storeLayerStyleListIntoMetadataDb(MetadataLayer *metadataLayer);
int loadLayerStyleListFromMetadataDb(MetadataLayer *layer);

int updateMetaDataTable(CDataSource *dataSource);
#endif