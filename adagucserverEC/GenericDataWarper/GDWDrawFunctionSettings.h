

#ifndef GDW_DRAWFUNCTION_UTILS_H
#define GDW_DRAWFUNCTION_UTILS_H

#include <vector>
#include "CCDFTypes.h"
#include "CColor.h"

class CDataSource;
class CDrawImage;
class CStyleConfiguration;

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
  // Whether CCreateLegend should bother building a legend at all for this style - distinct from
  // paintFieldColors below, which only controls whether GetMap itself paints raster pixel colors.
  // A contour-only legacy style paints no raster colors but can still have a meaningful legend.
  bool drawgrid = true;
  // Whether the generic warper paints the field's own (interpolated/shaded) colors onto the
  // canvas during GetMap. Kept separate from drawgrid (see above) so that disabling raster
  // painting for a legacy contour-only style doesn't also blank its legend.
  bool paintFieldColors = true;
  // Whether CImgWarpGeneric should trace and draw contour lines for this style. Resolved once
  // here (alongside the analogous isUsingShadeIntervals decision above) rather than re-derived
  // independently wherever it's consumed.
  bool shouldDrawContours = false;
  InterpolationMethod interpolationMethod = InterpolationMethodNearest;
  std::vector<Interval> intervals;
  CDrawImage *drawImage;
  CDFType destinationDataType;
  void *destinationGrid = nullptr;
  double smoothingFiter = 0;
  std::vector<double> smoothingMemo;
};

GDWDrawFunctionSettings getDrawFunctionSettings(CDataSource *dataSource, CDrawImage *drawImage, const CStyleConfiguration *styleConfiguration);

#endif