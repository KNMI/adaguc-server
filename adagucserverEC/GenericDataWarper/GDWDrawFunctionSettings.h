

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

  // Flattened copy of the style's 256-entry index-color palette, same as legendScale/legendOffset
  // above - avoids chasing a CStyleConfiguration pointer for every pixel of a hot warp loop.
  CColor clegend[256];

  // Looks up clegend[index]; transparent for an out-of-range index.
  // Defined inline (not in GDWDrawFunctionSettings.cpp) so it inlines straight into the generic
  // renderer's per-pixel call chain, same rationale as CDrawFunction.h.
  CColor getCColorFromLegendPaletteIndex(int index) const {
    if (index >= 0 && index < 256) {
      return clegend[index];
    }
    return CColor(0, 0, 0, 0);
  }

  // Looks up and blends in one call - skips the blend entirely when the resolved color is
  // transparent, cheaper than getCColorFromLegendPaletteIndex() plus a separate alpha check.
  void setPixelByLegendPaletteIndex(int x, int y, int colorIndex) const {
    if (colorIndex >= 0 && colorIndex < 256) {
      const CColor &c = clegend[colorIndex];
      if (c.a > 0) {
        drawImage->setPixel(x, y, c);
      }
    }
  }
};

GDWDrawFunctionSettings getDrawFunctionSettings(CDataSource *dataSource, CDrawImage *drawImage, const CStyleConfiguration *styleConfiguration);

#endif