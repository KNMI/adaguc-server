#ifndef GETPOINTSTYLE_H
#define GETPOINTSTYLE_H

#include "CTString.h"
#include "CColor.h"
#include <CServerConfig_CPPXSD.h>

struct PointStyle {
  std::string style;

  CColor fillColor;
  CColor lineColor;
  CColor textOutlineColor;
  CColor textColor;

  bool useTextColor;
  bool useFillColor;

  double fontSize;
  std::string fontFile;

  double discRadius;
  double textRadius;
  bool dot;

  CT::string textFormat;
  bool plotStationId;

  // symbol used for radiusandvalue
  std::string symbol;

  int maxPointCellSize;
  int maxPointsPerCell;

  double min;
  double max;
  bool isOutsideMinMax(float value) { return (value < min || value > max); }
};

PointStyle getPointStyle(CServerConfig::XMLE_Point *pointCfg, CServerConfig::XMLE_Configuration *cfg);

#endif