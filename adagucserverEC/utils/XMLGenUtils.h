#ifndef XML_GEN_UTILS_H
#define XML_GEN_UTILS_H

// Negative status codes means that there is an error
#define XMLGENUTILS_CHECKDEP_NODATASOURCE -1
#define XMLGENUTILS_CHECKDEP_DATASOURCE_NOT_CONFIGURED -2
#define XMLGENUTILS_CHECKDEP_DATASOURCE_NO_DIMS_IN_LAYERMETADATA -4
#define XMLGENUTILS_CHECKDEP_DATASOURCE_UNABLE_TO_PARSETIME -5
#define XMLGENUTILS_DATASOURCE_ERROR -10

// Positive status codes are not an error
#define XMLGENUTILS_CHECKDEP_DATASOURCE_NO_TIME 1
#define XMLGENUTILS_CHECKDEP_DATASOURCE_NO_ISO_DURATION 2

#include "CXMLGen.h"

int getProjectionInformationForLayer(MetadataLayer *metadataLayer);
int getDimsForLayer(MetadataLayer *metadataLayer);
int getStylesForLayer(MetadataLayer *metadataLayer);
bool compareStringCase(const std::string &s1, const std::string &s2);
bool compareProjection(const LayerMetadataProjection &p1, const LayerMetadataProjection &p2);
bool compareDim(const LayerMetadataDim &p1, const LayerMetadataDim &p2);
int populateMetadataLayerStruct(MetadataLayer *metadataLayer, bool readFromDb);
int getTitleForLayer(MetadataLayer *metadataLayer);
int getFileNameForLayer(MetadataLayer *metadataLayer);
bool multiTypeSort(const CT::string &a, const CT::string &b);
int checkDependenciesBetweenDims(MetadataLayer *mL);
#endif