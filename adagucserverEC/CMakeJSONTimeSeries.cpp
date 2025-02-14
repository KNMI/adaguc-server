#include <vector>
#include <algorithm>
#include "CMakeJSONTimeSeries.h"
#include "CImageDataWriter.h"
#include "CUniqueRequests/CURUniqueRequests.h"

const char *CMakeJSONTimeSeries::className = "CMakeJSONTimeSeries";

// #define CMakeJSONTimeSeries_DEBUG

int CMakeJSONTimeSeries::MakeJSONTimeSeries(CDrawImage *drawImage, CImageWarper *imageWarper, std::vector<CDataSource *> dataSources, int dataSourceIndex, int dX, int dY,
                                            CXMLParser::XMLElement *gfiStructure) {
  CDataSource *dataSource = dataSources[dataSourceIndex];

  CURUniqueRequests uniqueRequest;
  /**
   * DataPostProc: Here our datapostprocessor comes into action!
   */
  if (dataSource->cfgLayer->DataPostProc.size() > 0) {
    uniqueRequest.readDataAsCDFDouble = true;
  }

  int numberOfDims = dataSource->requiredDims.size();
  int numberOfSteps = dataSource->getNumTimeSteps();

#ifdef CMakeJSONTimeSeries_DEBUG
  CDBDebug("1): ***** Find all individual files *****");

  CDBDebug("NumberOfDims = %d", numberOfDims);
  CDBDebug("NumberOfSteps = %d", numberOfSteps);
  for (int dimnr = 0; dimnr < numberOfDims; dimnr++) {
    COGCDims *ogcDim = dataSource->requiredDims[dimnr];
    CDBDebug("Dim %d:) ds: %s nc: %s", dimnr, ogcDim->name.c_str(), ogcDim->netCDFDimName.c_str());
  }

#endif

  /* Find all individual files. uniqueRequest will group all files to single objects, and collect all the needed entries for those files*/
  for (int step = 0; step < numberOfSteps; step++) {
    dataSource->setTimeStep(step);
    for (int dimnr = 0; dimnr < numberOfDims; dimnr++) {
      COGCDims *ogcDim = dataSource->requiredDims[dimnr];
      uniqueRequest.set(dataSource->getFileName(), ogcDim->netCDFDimName.c_str(), dataSource->getDimensionIndex(dimnr), dataSource->getDimensionValue(dimnr));
    }
  }

#ifdef CMakeJSONTimeSeries_DEBUG
  CDBDebug("2): ***** sortAndAggregate *****");
#endif
  /* Sort it */
  try {
    uniqueRequest.sortAndAggregate();
  } catch (int e) {
    CDBError("Error in sortAndAggregate at line %d", e);
    throw(__LINE__);
  }

#ifdef CMakeJSONTimeSeries_DEBUG
  CDBDebug("3): ***** makeRequests and expanddata *****");
#endif

  /* Now efficiently fetch the data from the data files */
  try {
    uniqueRequest.makeRequests(drawImage, imageWarper, dataSource, dX, dY, gfiStructure);
  } catch (int e) {
    CDBError("Error in makeRequests at line %d", e);
    throw(__LINE__);
  }

  return 0;
};
