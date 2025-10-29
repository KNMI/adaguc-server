#ifndef SERVERUTILS_H
#define SERVERUTILS_H

#include "CTString.h"
#include "CServerConfig_CPPXSD.h"

bool checkIfFileMatchesLayer(CT::string layerPathToScan, CServerConfig::XMLE_Layer *layer);

#endif