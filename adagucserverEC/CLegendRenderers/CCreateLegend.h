#ifndef CCreateLegend_H
#define CCreateLegend_H

#include "Definitions.h"
#include "CStopWatch.h"
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CDebugger.h"

class CCreateLegend {
private:
  static int renderDiscreteLegend(CDataSource *dataSource, CDrawImage *legendImage, CStyleConfiguration *styleConfiguration, bool rotate, bool estimateMinMax);
  static int renderContinuousLegend(CDataSource *dataSource, CDrawImage *legendImage, CStyleConfiguration *styleConfiguration, bool rotate, bool estimateMinMax);
  static double nextTick(double prev);
  static double classCount(double min, double max);
  static char *formatTickLabel(CT::string textformatting, char *szTemp, size_t szTempLength, double tick, double min, double max, int tickRound);

public:
  static int createLegend(CDataSource *dataSource, CDrawImage *legendImage);
  static int createLegend(CDataSource *dataSource, CDrawImage *legendImage, bool rotate);
};

#endif
