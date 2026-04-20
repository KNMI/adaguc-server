#ifndef CURUNIQUEREQUESTS_H
#define CURUNIQUEREQUESTS_H

#include <cstdio>
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
public:
  bool readDataAsCDFDouble = false;
  void makeRequests(std::map<std::string, CURFileInfo> &fileInfoMap, CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY, CXMLParser::XMLElement *gfiStructure);
};

#endif
