#ifndef GETVECTORSTYLE_H
#define GETVECTORSTYLE_H

#include "CTString.h"
#include "CColor.h"
#include <CServerConfig_CPPXSD.h>

struct VectorStyle {
  CColor lineColor;
  double lineWidth;
  CColor outlinecolor;
  double outlinewidth;
  CColor textColor;
  CT::string drawVectorTextFormat;
  bool drawVectorPlotStationId;
  bool drawVectorPlotValue;
  bool drawBarb;
  bool drawDiscs;
  bool drawVector;
  double symbolScaling;
  double min;
  double max;
};

VectorStyle getVectorStyle(CServerConfig::XMLE_Vector *vectorCfg);

#endif