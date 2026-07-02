
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

/**
 * Time formatting / Formats time to correct iso8601 string with a 'T' at position 10 and a trailing 'Z'.
 * If the timestring is shorter then the same string is simply returned.
 * @param input time string like '2025-01-02 01:02:03`
 * @returns formatted iso string like  '2025-01-02T01:02:03Z`
 */
std::string makeIsoStringFromDbString(std::string input);
#endif