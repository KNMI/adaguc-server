#ifndef CCreateLegend_H
#define CCreateLegend_H

#include "Definitions.h"
#include "CStopWatch.h"
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CDebugger.h"

class CCreateLegend{
private:
  DEF_ERRORFUNCTION();
public:
  static int createLegend(CDataSource *dataSource,CDrawImage *legendImage);
};

#endif
