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
  static int renderDiscreteLegend(CDataSource *dataSource,CDrawImage *legendImage, CStyleConfiguration *styleConfiguration, bool rotate,  bool estimateMinMax);
public:
  static int createLegend(CDataSource *dataSource,CDrawImage *legendImage);
  static int createLegend(CDataSource *dataSource,CDrawImage *legendImage, bool rotate);
};

#endif
