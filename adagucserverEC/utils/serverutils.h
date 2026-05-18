#ifndef SERVERUTILS_H
#define SERVERUTILS_H

#include "CTString.h"
#include "CServerConfig_CPPXSD.h"
#include <string>

bool checkIfFileMatchesLayer(const std::string &layerPathToScan, CServerConfig::XMLE_Layer *layer);

#endif