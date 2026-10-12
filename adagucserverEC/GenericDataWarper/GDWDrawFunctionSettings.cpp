#include "GenericDataWarper/GDWDrawFunctionSettings.h"
#include <algorithm>
#include <sys/types.h>
#include <CImageOperators/smoothRasterField.h>
#include "CColor.h"
#include "CDrawImage.h"
#include "CCDFObject.h"
#include "CCDFTypes.h"
#include "CDataSource.h"
#include "CGenericDataWarper.h"
#include "CImageWarper.h"
#include "CServerConfig_CPPXSD.h"
#include "CTString.h"
#include "Types/CPointTypes.h"
#include "Types/GeoParameters.h"

GDWDrawFunctionSettings getDrawFunctionSettings(CDataSource *dataSource, CDrawImage *drawImage, const CStyleConfiguration *styleConfiguration) {
  GDWDrawFunctionSettings settings;

  settings.dfNodataValue = dataSource->getFirstAvailableDataObject()->dfNodataValue;
  settings.legendValueRange = styleConfiguration->hasLegendValueRange;
  settings.legendLowerRange = styleConfiguration->legendLowerRange;
  settings.legendUpperRange = styleConfiguration->legendUpperRange;
  settings.hasNodataValue = dataSource->getFirstAvailableDataObject()->hasNodataValue;

  settings.legendLog = styleConfiguration->legendLog;
  if (settings.legendLog > 0) {
    settings.legendLogAsLog = log10(settings.legendLog);
  } else {
    settings.legendLogAsLog = 0;
  }
  settings.legendScale = styleConfiguration->legendScale;
  settings.legendOffset = styleConfiguration->legendOffset;
  settings.drawImage = drawImage;

  // A legacy style can list several RenderMethod keywords on one style block (e.g.
  // "nearest,bilinear,shadedcontour") so that a single GetMap request can pick a specific subset
  // via STYLES=name/keyword; "legacy" here means the style uses one of those old keywords at all.
  bool isLegacyRenderMethod = isLegacyScalarRenderMethod(styleConfiguration->renderMethod);

  if (isLegacyRenderMethod) {
    // Legacy render methods (bilinear/shaded/contour), now routed through the generic warper.
    // CImgWarpBilinear always sampled values with smooth (gouraud) interpolation regardless of
    // its "nearest"/"bilinear" keyword, and only painted the field's own colors for the
    // bilinear/shaded render methods, not for contour-only rendering (which just draws lines
    // over whatever is already on the canvas). This also applies when a vector/barb render
    // method is combined with shaded/contour on the same style (e.g. "barbshadedcontour") - the
    // scalar background still needs smoothing and painting, the barbs are drawn separately
    // through points (see CImageDataWriter.cpp). These are defaults only: an explicit
    // <RenderSettings> interpolationmethod/drawgrid attribute below still overrides them - a
    // style can have a <RenderSettings> tag for an unrelated attribute (e.g. just a renderhint)
    // without wanting to give up the always-smooth legacy sampling.
    settings.interpolationMethod = InterpolationMethodBilinear;
    settings.drawgrid = (styleConfiguration->renderMethod & (RM_BILINEAR | RM_SHADED)) != 0;
  }

  if (styleConfiguration->renderSettings.size() > 0) {
    auto renderSetting = styleConfiguration->renderSettings.back();
    auto &renderSettingsAttr = renderSetting.attr;
    if (renderSettingsAttr.renderhint == (RENDERHINT_DISCRETECLASSES)) {
      settings.isUsingShadeIntervals = true;
    }
    // Obtain interpolationmethod
    if (renderSettingsAttr.interpolationmethod == ("nearest")) {
      settings.interpolationMethod = InterpolationMethodNearest;
    } else if (renderSettingsAttr.interpolationmethod == ("bilinear")) {
      settings.interpolationMethod = InterpolationMethodBilinear;
    }
    if (renderSettingsAttr.drawgrid == ("false")) {
      settings.drawgrid = false;
    } else if (renderSettingsAttr.drawgrid == ("true")) {
      settings.drawgrid = true;
    }

    if (renderSettingsAttr.drawgridboxoutline == ("true")) {
      settings.drawgridboxoutline = true;
    }
  }
  /* Check the if we want to use discrete type */

  // For a pure generic style (no legacy bilinear/shaded/contour keyword at all, e.g.
  // <RenderMethod>generic</RenderMethod> or no <RenderMethod> tag) there's only one rendering
  // mode, so configuring a ShadeInterval is enough to turn shading on. For a legacy style, only
  // the "shaded" keyword (RM_SHADED) should turn shading on - otherwise selecting the "bilinear"
  // subset of such a style would always draw the shaded classes too, even though "bilinear" was
  // deliberately chosen to get the plain continuous legend instead.
  if (settings.isUsingShadeIntervals == false && styleConfiguration->shadeIntervals.size() > 0) {
    if (styleConfiguration->renderMethod & RM_SHADED) {
      settings.isUsingShadeIntervals = true;
    } else if ((styleConfiguration->renderMethod & RM_GENERIC) && !isLegacyRenderMethod) {
      settings.isUsingShadeIntervals = true;
    }
  }

  // Smoothingfilter:
  if (styleConfiguration->smoothingFilterVector.size() > 0) {
    auto smoothingFilter = styleConfiguration->smoothingFilterVector.back();

    if (!smoothingFilter.elementValue.empty()) {
      settings.smoothingFiter = atof(smoothingFilter.elementValue.c_str());
    }
  }

  /* Make a shorthand vector from the shadeInterval configuration*/
  if (settings.isUsingShadeIntervals) {
    int numShadeDefs = (int)styleConfiguration->shadeIntervals.size();
    if (numShadeDefs > 0) {
      if (numShadeDefs == 1 && !styleConfiguration->shadeIntervals[0].elementValue.empty()) {
        settings.shadeInterval = atof(styleConfiguration->shadeIntervals[0].elementValue.c_str());
      } else {
        settings.intervals.reserve(numShadeDefs);
        for (const auto &shadeInterval : styleConfiguration->shadeIntervals) {
          settings.intervals.push_back(Interval({.min = shadeInterval.attr.min, .max = shadeInterval.attr.max, .color = CColor(shadeInterval.attr.fillcolor.c_str())}));
        }
        // Sort shaded intervals on min value
        std::sort(settings.intervals.begin(), settings.intervals.end(), [](const Interval &left, const Interval &right) { return left.min < right.min; });

        // Check for bgcolor in first element
        const std::string &firstElemBgColor = styleConfiguration->shadeIntervals.front().attr.bgcolor;
        if (!firstElemBgColor.empty()) {
          settings.bgColorDefined = true;
          settings.bgColor = CColor(firstElemBgColor);
        }
      }
    }
  }

  return settings;
}
