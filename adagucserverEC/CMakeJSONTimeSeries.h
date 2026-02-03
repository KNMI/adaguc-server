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

class CMakeJSONTimeSeries {
public:
  static int MakeJSONTimeSeries(CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSources, int dX, int dY, CXMLParser::XMLElement *gfiStructure);
};

#endif