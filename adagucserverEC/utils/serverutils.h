#ifndef SERVERUTILS_H
#define SERVERUTILS_H

#include "CServerConfig_CPPXSD.h"
#include <set>
#include <cstdio>

bool checkIfFileMatchesLayer(std::string layerPathToScan, CServerConfig::XMLE_Layer *layer);
/**
 * @param layerPathToScan: the provided file to scan
 */
std::set<std::string> findDataSetsToScan(std::string layerPathToScan, bool verbose);

/**
 * Copies the rest of in to out in blocks. In fork mode stdout is unbuffered, so copying byte by byte would do a write call per byte.
 * @return the number of bytes written
 */
size_t copyFileToStream(FILE *in, FILE *out);
#endif