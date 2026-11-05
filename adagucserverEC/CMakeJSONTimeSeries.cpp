#include <vector>
#include "CMakeJSONTimeSeries.h"
#include "Definitions.h"
#include "CXMLParser.h"
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CDebugger.h"
#include "CUniqueRequests/CURUniqueRequests.h"
#include "CUniqueRequests/trajectory.h"
#include "CImageWarper.h"
#include "CServerError.h"

static const bool CMakeJSONTimeSeries_DEBUG = false;

int CMakeJSONTimeSeries::MakeJSONTimeSeries(CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY, CXMLParser::XMLElement *gfiStructure) {
  CURUniqueRequests uniqueRequest;
  /**
   * DataPostProc: Here our datapostprocessor comes into action!
   */
  if (dataSource->cfgLayer->DataPostProc.size() > 0) {
    uniqueRequest.readDataAsCDFDouble = true;
  }

  int numberOfDims = dataSource->requiredDims.size();
  int numberOfSteps = dataSource->getNumTimeSteps();

  if (CMakeJSONTimeSeries_DEBUG) {
    CDBDebug("1): ***** Find all individual files *****");

    CDBDebug("NumberOfDims = %d", numberOfDims);
    CDBDebug("NumberOfSteps = %d", numberOfSteps);
    for (int dimnr = 0; dimnr < numberOfDims; dimnr++) {
      COGCDims &ogcDim = dataSource->requiredDims[dimnr];
      CDBDebug("Dim %d:) ds: %s nc: %s", dimnr, ogcDim.name.c_str(), ogcDim.netCDFDimName.c_str());
    }
  }
  std::map<std::string, CURFileInfo> fileInfoMap;

  /* Find all individual files. uniqueRequest will group all files to single objects, and collect all the needed entries for those files*/
  for (int step = 0; step < numberOfSteps; step++) {
    dataSource->setTimeStep(step);
    // Keep one selection per file so its context can be restored when reading.
    if (dataSource->srvParams->isTrajectory) fileInfoMap[dataSource->getFileName()].representativeStep = step;
    for (int dimnr = 0; dimnr < numberOfDims; dimnr++) {
      auto &ogcDim = dataSource->requiredDims[dimnr];
      fileInfoMap[dataSource->getFileName()].dimInfoMap[ogcDim.netCDFDimName].dimValuesMap[dataSource->getDimensionIndex(dimnr)] = dataSource->getDimensionValue(dimnr);
    }
  }

  /* Bundle and make the requests */
  try {
    uniqueRequest.makeRequests(fileInfoMap, drawImage, imageWarper, dataSource, dX, dY, gfiStructure);
  } catch (int e) {
    CDBError("Error in makeRequests at line %d", e);
    if (dataSource->srvParams->isTrajectory) {
      setExceptionType(ServiceExceptionType::UnprocessableEntity);
      return 1;
    }
    throw(__LINE__);
  }

  return 0;
};
