#ifndef CCreateLegend_H
#define CCreateLegend_H

#include "CDataSource.h"
#include "CDrawImage.h"

class CCreateLegend {
private:
  static int renderDiscreteLegend(CDataSource *dataSource, CDrawImage *legendImage, CStyleConfiguration *styleConfiguration, bool rotate, bool estimateMinMax);
  static int renderContinuousLegend(CDataSource *dataSource, CDrawImage *legendImage, CStyleConfiguration *styleConfiguration, bool rotate, bool estimateMinMax);
  static double nextTick(double prev);
  static double classCount(double min, double max);
  static std::string formatTickLabel(const std::string &textformatting, double tick, double min, double max, int tickRound);

public:
  static int createLegend(CDataSource *dataSource, CDrawImage *legendImage);
  static int createLegend(CDataSource *dataSource, CDrawImage *legendImage, bool rotate);
};

#endif
