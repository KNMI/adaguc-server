

#ifndef GDW_DRAWFUNCTION_UTILS_H
#define GDW_DRAWFUNCTION_UTILS_H

#include "CGenericDataWarper.h"
#include "CColor.h"
#include "CDrawImage.h"

enum DrawInImage { DrawInImageNone, DrawInImageNearest, DrawInImageBilinear };
enum DrawInDataGrid { DrawInDataGridNone, DrawInDataGridNearest, DrawInDataGridBilinear };
enum LegendMode { LegendModeContinuous, LegendModeDiscrete };

class Interval { // TODO Struct
public:
  float min;
  float max;
  CColor color;
  Interval(float min, float max, CColor color) {
    this->min = min;
    this->max = max;
    this->color = color;
  }
};

struct GDWDrawFunctionSettings : GDWDrawFunctionState {
  double dfNodataValue;
  double legendValueRange;
  double legendLowerRange;
  double legendUpperRange;
  double smoothingFiter = 0;
  bool isUsingShadeIntervals = false;
  bool bgColorDefined = false;
  double shadeInterval = 0;
  double legendLog;
  double legendLogAsLog;
  double legendScale;
  double legendOffset;
  CColor bgColor;
  bool hasNodataValue;
  DrawInImage drawInImage = DrawInImageNone;
  DrawInDataGrid drawInDataGrid = DrawInDataGridNone;
  std::vector<Interval> intervals;
  float *smoothingDistanceMatrix = nullptr;
  CDrawImage *drawImage;
};

void gdwDrawFunction(GDWDrawFunctionState *_drawSettings);

#endif