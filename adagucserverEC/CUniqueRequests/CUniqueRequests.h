
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

#ifndef CURUNIQUEREQUESTS_H
#define CURUNIQUEREQUESTS_H

class CURUniqueRequests {
private:
  DEF_ERRORFUNCTION();

  class CURResult {
  private:
    CURUniqueRequests *parent;

  public:
    CURResult(CURUniqueRequests *parent) { this->parent = parent; }
    CT::string dimensionKeys[CCUniqueRequests_MAX_DIMS];
    CT::string value;
    int numDims;
    int *getDimOrder() { return parent->getDimOrder(); }
  };

  void recurDataStructure(CXMLParser::XMLElement *dataStructure, CURResult *result, int depth, int *dimOrdering, std::vector<int> dimIndicesToSkip);
  void createStructure(CDataSource::DataObject *dataObject, CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY, CXMLParser::XMLElement *gfiStructure);

  struct less_than_key {
    inline bool operator()(CURResult *result1, CURResult *result2) {
      int *dimOrder = result1->getDimOrder();
      std::string s1;
      std::string s2;

      for (int d = 0; d < result1->numDims; d++) {
        int dimOrderIndex = dimOrder[d];
        // CDBDebug("Dimension order = %d", dimOrderIndex);
        // CDBDebug("Dimension order A= %s", result1->dimensionKeys[dimOrderIndex].c_str());
        // CDBDebug("Dimension order B= %s", result2->dimensionKeys[dimOrderIndex].c_str());
        s1 += result1->dimensionKeys[dimOrderIndex].c_str();
        s2 += result2->dimensionKeys[dimOrderIndex].c_str();
      }
      if (s1.compare(s2) < 0) return true;
      return false;
      // return (struct1.key < struct2.key);
    }
  };

  std::map<std::string, CURFileInfo *> fileInfoMap; // File name is key
  typedef std::map<std::string, CURFileInfo *>::iterator it_type_file;
  CT::string dimensionKeys[CCUniqueRequests_MAX_DIMS];
  int dimOrdering[CCUniqueRequests_MAX_DIMS];
  int *getDimOrder() { return dimOrdering; }
  std::vector<CURResult *> results;
  CURAggregatedDimension *dimensions[CCUniqueRequests_MAX_DIMS];
  void expandData(CDataSource *dataSource, CDataSource::DataObject *dataObject, CDF::Variable *variable, size_t *start, size_t *count, int d, CURRequest *request, int index, int *multiplies);
  size_t size();
  CURFileInfo *get(size_t index);
  void addDimSet(CURDimInfo *dimInfo, int start, std::vector<CT::string> valueList);
  void nestRequest(it_type_diminfo diminfomapiterator, CURFileInfo *fileInfo, int depth);

public:
  bool readDataAsCDFDouble;
  CURUniqueRequests();
  ~CURUniqueRequests();

  void set(const char *filename, const char *dimName, size_t dimIndex, CT::string dimValue);
  void sortAndAggregate();
  void makeRequests(CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY, CXMLParser::XMLElement *gfiStructure);
};

#endif
