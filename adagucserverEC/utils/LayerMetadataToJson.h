#ifndef LAYERMETADATATOJSON_H
#define LAYERMETADATATOJSON_H
#include <CDataSource.h>
#include <json_adaguc.h>

int getLayerMetadataAsJson(CDataSource *dataSource, json &result);

#endif