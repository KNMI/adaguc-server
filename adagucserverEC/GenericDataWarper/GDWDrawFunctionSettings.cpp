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
  settings.styleConfiguration = styleConfiguration;

  const bool isLegacyStyle = isLegacyRenderMethod(styleConfiguration->renderMethod);

  if (isLegacyStyle) {
    // CImgWarpBilinear always sampled smooth; painted colors only for bilinear/shaded.
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

  // Generic renderer applies shading whenever ShadeInterval is set; legacy only if "shaded" is set.
  if (!settings.isUsingShadeIntervals && styleConfiguration->shadeIntervals.size() > 0) {
    const bool isGeneric = (styleConfiguration->renderMethod & RM_GENERIC) != 0 && !isLegacyStyle;
    settings.isUsingShadeIntervals = (styleConfiguration->renderMethod & RM_SHADED) != 0 || isGeneric;
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

  // For a legacy style, only the "contour" keyword should draw contour lines.
  if (isLegacyStyle) {
    settings.shouldDrawContours = (styleConfiguration->renderMethod & RM_CONTOUR) != 0;
  } else {
    settings.shouldDrawContours = styleConfiguration->contourLines.size() > 0;
  }

  return settings;
}
