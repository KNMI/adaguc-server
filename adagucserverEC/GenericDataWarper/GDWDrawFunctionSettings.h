

#ifndef GDW_DRAWFUNCTION_UTILS_H
#define GDW_DRAWFUNCTION_UTILS_H

#include "CGenericDataWarper.h"
#include "CColor.h"
#include "CDrawImage.h"

enum InterpolationMethod { InterpolationMethodNearest, InterpolationMethodBilinear };
enum LegendMode { LegendModeContinuous, LegendModeDiscrete };

struct Interval {
  float min;
  float max;
  CColor color;
};

struct GDWDrawFunctionSettings {
  double dfNodataValue;
  double legendValueRange;
  double legendLowerRange;
  double legendUpperRange;
  bool isUsingShadeIntervals = false;
  bool bgColorDefined = false;
  double shadeInterval = 0;
  double legendLog;
  double legendLogAsLog;
  double legendScale;
  double legendOffset;
  CColor bgColor;
  bool hasNodataValue;
  bool drawgridboxoutline = false;
  bool drawgrid = true;
  InterpolationMethod interpolationMethod = InterpolationMethodNearest;
  std::vector<Interval> intervals;
  CDrawImage *drawImage;
  CDFType destinationDataType;
  void *destinationGrid = nullptr;
  double smoothingFiter = 0;
  std::vector<double> smoothingDistanceMatrix;
  std::vector<double> smoothingMemo;
};

GDWDrawFunctionSettings getDrawFunctionSettings(CDataSource *dataSource, CDrawImage *drawImage, const CStyleConfiguration *styleConfiguration);

#endif