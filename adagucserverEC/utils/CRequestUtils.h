
#include <CServerParams.h>
#include <CDataSource.h>
#include <CImageWarper.h>

#ifndef CREQUESTUTILS_H
#define CREQUESTUTILS_H

std::tuple<int, f8box> findBBoxForDataSource(std::vector<CDataSource *> dataSources);

CServerConfig::XMLE_Layer *findLayerConfigForRequestedLayer(CServerParams *serverParameters, CT::string requestedLayerName);

/**
 * Finds the reference time dimension as configured in the Layer element of the Dataset.
 * @returns The name of the dimension in the CDF model, or empty string if not found
 */
std::string getReferenceTimeDimName(CDataSource &dataSource);

/**
 * Return a list of referencetimes for this datasource.
 */
std::vector<std::string> getReferenceTimes(CDataSource &dataSource);

int getMaxQueryLimit(CDataSource &dataSource);

std::string makeIsoStringFromDbString(std::string input);
#endif