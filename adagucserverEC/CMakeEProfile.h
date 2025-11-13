#ifndef CMakeEProfile_H
#define CMakeEProfile_H

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

class CMakeEProfile {
public:
  DEF_ERRORFUNCTION();
  class DayPass {
  public:
    DayPass(int x, double offset) {
      this->x = x;
      this->offset = offset;
    };
    double offset;
    int x;
  };
  static int MakeEProfile(CDrawImage *drawImage, CImageWarper *imageWarper, CDataSource *dataSource, int dX, int dY, CT::string *json);
};

#endif