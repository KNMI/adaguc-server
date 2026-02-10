#ifndef SERVERUTILS_H
#define SERVERUTILS_H

#include "CTString.h"
#include "CServerConfig_CPPXSD.h"
#include <set>

bool checkIfFileMatchesLayer(CT::string layerPathToScan, CServerConfig::XMLE_Layer *layer);
/**
 * @param layerPathToScan: the provided file to scan
 */
std::set<std::string> findDataSetsToScan(CT::string layerPathToScan, bool verbose);
#endif