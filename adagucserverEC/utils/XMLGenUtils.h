#ifndef XML_GEN_UTILS_H
#define XML_GEN_UTILS_H

#include "CXMLGen.h"

int getProjectionInformationForLayer(MetadataLayer *metadataLayer);
int getDimsForLayer(MetadataLayer *metadataLayer);
int getStylesForLayer(MetadataLayer *metadataLayer);
bool compareStringCase(const std::string &s1, const std::string &s2);
bool compareProjection(const LayerMetadataProjection &p1, const LayerMetadataProjection &p2);
bool compareDim(const LayerMetadataDim &p1, const LayerMetadataDim &p2);
int populateLayerMetadataStruct(MetadataLayer *metadataLayer, bool readFromDb);
int getTitleForLayer(MetadataLayer *metadataLayer);
int getFileNameForLayer(MetadataLayer *metadataLayer);
#endif