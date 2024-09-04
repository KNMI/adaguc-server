#ifndef XML_GEN_UTILS_H
#define XML_GEN_UTILS_H

#include "CXMLGen.h"

int getProjectionInformationForLayer(WMSLayer *myWMSLayer);
int getDimsForLayer(WMSLayer *myWMSLayer);
int getStylesForLayer(WMSLayer *myWMSLayer);
bool compareStringCase(const std::string &s1, const std::string &s2);
int populateMyWMSLayerStruct(WMSLayer *myWMSLayer);
int getTitleForLayer(WMSLayer *myWMSLayer);
int getFileNameForLayer(WMSLayer *myWMSLayer);
#endif