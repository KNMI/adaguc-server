#include <CServerConfig_CPPXSD.h>
#include <CTString.h>

#ifndef LAYERUTILS_H
#define LAYERUTILS_H

/**
 * Function which generates a unique layername from the Layer's configuration
 * @param cfgLayer t*he configuration object of the corresponding layer
 * @return the returned name
 */
CT::string makeUniqueLayerName(CServerConfig::XMLE_Layer *cfgLayer, const char *optionalLayerName = nullptr);

#endif