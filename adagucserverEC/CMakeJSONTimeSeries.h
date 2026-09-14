#ifndef CMakeJSONTimeSeries_H
#define CMakeJSONTimeSeries_H

#include <string>
#include <map>
#include <set>

#include "CImageDataWriter.h"
#include "CDrawImage.h"
#include "CImageWarper.h"
#include "CXMLParser.h"

class CMakeJSONTimeSeries {
public:
  static int MakeJSONTimeSeries(CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSources, int dX, int dY, CXMLParser::XMLElement *gfiStructure);
};

#endif