#ifndef TOJSONANDBACK_H
#define TOJSONANDBACK_H

#include <string>
#include <vector>
#include "CXMLGen.h"

std::string convertProjectionListToJsonString(std::vector<WMSLayer::Projection *> projectionList);

#endif