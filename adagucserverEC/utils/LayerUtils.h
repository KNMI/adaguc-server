#include <CServerConfig_CPPXSD.h>
#include <CTString.h>

#ifndef LAYERUTILS_H
#define LAYERUTILS_H

/**
 * Function which generates a unique layername from the Layer's configuration
 * @param layerName the returned name
 * @param cfgLayer the configuration object of the corresponding layer
 */
int makeUniqueLayerName(CT::string *layerName, CServerConfig::XMLE_Layer *cfgLayer);

#endif