#ifndef GETVECTORSTYLE_H
#define GETVECTORSTYLE_H

#include "CTString.h"
#include "CColor.h"
#include <CServerConfig_CPPXSD.h>

struct TextStyle {
  CColor textColor;
  double fontSize;

  CColor textOutlineColor;
  double textOutlineWidth;
};

struct LineStyle {
  CColor lineColor;
  double lineWidth;

  CColor lineOutlineColor;
  double lineOutlineWidth;
};

struct VectorStyle {
  LineStyle lineStyle;
  TextStyle textStyle;
  CColor fillColor;
  CT::string drawVectorTextFormat;
  CT::string fontFile;
  bool drawVectorPlotStationId;
  bool drawVectorPlotValue;
  bool drawBarb;
  bool drawDiscs;
  bool drawVector;
  double discRadius;
  double symbolScaling;
  double min;
  double max;
};

VectorStyle getVectorStyle(CServerConfig::XMLE_Vector *vectorCfg, CServerConfig::XMLE_Configuration *globalConfig);

#endif