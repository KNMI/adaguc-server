#ifndef CCreateHistogram_H
#define CCreateHistogram_H

#include "Definitions.h"
#include "CStopWatch.h"
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CDebugger.h"

class CCreateHistogram{
private:
  DEF_ERRORFUNCTION();
public:
  static int createHistogram(CDataSource *dataSource,CDrawImage *legendImage){
    CDBDebug("createHistogram");
    return 0;
  }
};

#endif

