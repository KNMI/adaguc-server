#ifndef LAYER_METADATA_STORE_UTILS_H
#define LAYER_METADATA_STORE_UTILS_H

#include <string>
#include <vector>
#include "CXMLGen.h"

std::string convertProjectionListToJsonString(std::vector<WMSLayer::Projection *> projectionList);

int storeProjectionList(WMSLayer *layer);

int getProjectionList(WMSLayer *layer);

#endif