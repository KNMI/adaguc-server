#ifndef CURUNIQUEREQUESTS_H
#define CURUNIQUEREQUESTS_H

#include <stdio.h>
#include <map>
#include "CTString.h"
#include "CDebugger.h"
#include "CURConstants.h"
#include "CURDimInfo.h"
#include "CURFileInfo.h"
#include "CServerParams.h"
#include "CDrawImage.h"
#include "CDataSource.h"
#include "CImageWarper.h"
#include "CUniqueRequests/CURResult.h"
#include "CUniqueRequests/requestUtils.h"
#include "requestUtils.h"

class CURResult;

class CURUniqueRequests {

  typedef std::map<std::string, CURFileInfo *>::iterator it_type_file;

private:
  DEF_ERRORFUNCTION();
  std::map<std::string, CURFileInfo *> fileInfoMap; // File name is key
  CT::string dimensionKeys[CCUniqueRequests_MAX_DIMS];
  int dimOrdering[CCUniqueRequests_MAX_DIMS];
  std::vector<CURResult> results;
  CURAggregatedDimension *dimensions[CCUniqueRequests_MAX_DIMS];

  void recurDataStructure(CXMLParser::XMLElement *dataStructure, CURResult *result, int depth, int *dimOrdering, std::vector<int> dimIndicesToSkip);
  void createStructure(CDataSource::DataObject *dataObject, CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY, CXMLParser::XMLElement *gfiStructure);
  void expandData(CDataSource *dataSource, CDataSource::DataObject *dataObject, CDF::Variable *variable, size_t *start, size_t *count, int d, CURRequest *request, int index, int *multiplies);

  CURFileInfo *get(size_t index);
  void addDimSet(CURDimInfo *dimInfo, int start, std::vector<std::string> valueList);
  void nestRequest(it_type_diminfo diminfomapiterator, CURFileInfo *fileInfo, int depth);
  size_t size();

public:
  CURUniqueRequests();

  bool readDataAsCDFDouble;

  int *getDimOrder();
  void makeRequests(CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY, CXMLParser::XMLElement *gfiStructure);
  void set(const char *filename, const char *dimName, size_t dimIndex, CT::string dimValue);
  void sortAndAggregate();
};

#endif
