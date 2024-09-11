#ifndef XML_GEN_UTILS_H
#define XML_GEN_UTILS_H

#include "CXMLGen.h"

int getProjectionInformationForLayer(MetadataLayer *myWMSLayer);
int getDimsForLayer(MetadataLayer *myWMSLayer);
int getStylesForLayer(MetadataLayer *myWMSLayer);
bool compareStringCase(const std::string &s1, const std::string &s2);
bool compareProjection(const LayerMetadataProjection *p1, const LayerMetadataProjection *p2);
bool compareDim(const LayerMetadataDim *p1, const LayerMetadataDim *p2);
bool compareStyle(const LayerMetadataStyle *p1, const LayerMetadataStyle *p2);
int populateMyWMSLayerStruct(MetadataLayer *myWMSLayer, bool readFromDb);
int getTitleForLayer(MetadataLayer *myWMSLayer);
int getFileNameForLayer(MetadataLayer *myWMSLayer);
#endif