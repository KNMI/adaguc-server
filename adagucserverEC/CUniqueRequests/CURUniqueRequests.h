#ifndef CURUNIQUEREQUESTS_H
#define CURUNIQUEREQUESTS_H

#include <stdio.h>
#include <map>
#include "CTString.h"
#include "CDebugger.h"
#include "CURTypes.h"
#include "CServerParams.h"
#include "CDrawImage.h"
#include "CDataSource.h"
#include "CImageWarper.h"
#include "CUniqueRequests/requestUtils.h"
#include "requestUtils.h"

class CURUniqueRequests {

private:
  DEF_ERRORFUNCTION();
  std::map<std::string, CURFileInfo> fileInfoMap;
  int dimOrdering[CCUniqueRequests_MAX_DIMS];
  CURAggregatedDimension aggregatedDimensions[CCUniqueRequests_MAX_DIMS];

  void recurDataStructure(CXMLParser::XMLElement *dataStructure, CURResult *result, int depth, int *dimOrdering, std::vector<int> dimIndicesToSkip);
  void createStructure(std::vector<CURResult> results, CDataSource::DataObject *dataObject, CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY,
                       CXMLParser::XMLElement *gfiStructure);
  void addDimSet(CURDimInfo &dimInfo, int start, std::vector<std::string> valueList);
  void nestRequest(it_type_diminfo diminfomapiterator, CURFileInfo &fileInfo, int depth);
  void sortAndAggregate();

public:
  bool readDataAsCDFDouble = false;
  int *__getDimOrder();

  void addDimensionRangeRequest(const char *filename, const char *dimName, size_t dimIndex, std::string dimValue);
  void makeRequests(CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY, CXMLParser::XMLElement *gfiStructure);
};

#endif
