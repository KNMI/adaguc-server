#ifndef CMakeJSONTimeSeries_H
#define CMakeJSONTimeSeries_H

#include <string>
#include <map>
#include <set>

#include "Definitions.h"
#include "CStopWatch.h"
#include "CXMLParser.h"
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CImageDataWriter.h"
#include "CDebugger.h"


class CMakeJSONTimeSeries{
public:
   DEF_ERRORFUNCTION();
   
  static int MakeJSONTimeSeries(CDrawImage *drawImage,CImageWarper *imageWarper,std::vector<CDataSource *>dataSources,int dataSourceIndex,int dX,int dY,CXMLParser::XMLElement *gfiStructure);
};


#endif