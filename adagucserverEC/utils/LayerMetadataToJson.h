#ifndef LAYERMETADATATOJSON_H
#define LAYERMETADATATOJSON_H
#include "CDataSource.h"
#include "CServerError.h"
#include <json_adaguc.h>

ServiceExceptionType getLayerMetadataAsJson(CServerParams *srvParams, json &result);

#endif