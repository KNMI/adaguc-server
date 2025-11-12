
#include <CServerParams.h>
#include <CDataSource.h>
#include <CImageWarper.h>

#ifndef CREQUESTUTILS_H
#define CREQUESTUTILS_H

std::tuple<int, f8box> findBBoxForDataSource(std::vector<CDataSource *> dataSources);

CServerConfig::XMLE_Layer *findLayerConfigForRequestedLayer(CServerParams *serverParameters, CT::string requestedLayerName);

#endif