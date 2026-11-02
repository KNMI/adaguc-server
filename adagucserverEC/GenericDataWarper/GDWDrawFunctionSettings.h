

#ifndef GDW_DRAWFUNCTION_UTILS_H
#define GDW_DRAWFUNCTION_UTILS_H

#include "CGenericDataWarper.h"
#include "CCDFTypes.h"
#include "CColor.h"
#include "CDataSource.h"
#include "CDrawImage.h"

enum InterpolationMethod { InterpolationMethodNearest, InterpolationMethodBilinear };
enum LegendMode { LegendModeContinuous, LegendModeDiscrete };

struct Interval {
  double min;
  double max;
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
  // Whether CImgWarpGeneric paints raster colors onto the canvas during GetMap.
  bool drawgrid = true;
  // Whether CImgWarpGeneric should trace and draw contour lines for this style.
  bool shouldDrawContours = false;
  InterpolationMethod interpolationMethod = InterpolationMethodNearest;
  std::vector<Interval> intervals;
  CDrawImage *drawImage;
  CDFType destinationDataType;
  void *destinationGrid = nullptr;
  double smoothingFiter = 0;
  std::vector<double> smoothingMemo;

  const CStyleConfiguration *styleConfiguration = nullptr; // avoids copying the 256-entry palette

  CColor getCColorFromLegendPaletteIndex(int index) const { return styleConfiguration->getCColorFromLegendPaletteIndex(index); }
  void setPixelByLegendPaletteIndex(int x, int y, int colorIndex) const { styleConfiguration->drawPixelFromPaletteIndex(drawImage, x, y, colorIndex); }
};

GDWDrawFunctionSettings getDrawFunctionSettings(CDataSource *dataSource, CDrawImage *drawImage, const CStyleConfiguration *styleConfiguration);

#endif