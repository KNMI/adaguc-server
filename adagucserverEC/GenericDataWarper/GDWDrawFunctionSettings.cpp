#include "GenericDataWarper/GDWDrawFunctionSettings.h"
#include <algorithm>
#include <sys/types.h>
#include <CImageOperators/smoothRasterField.h>

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

  if (styleConfiguration->renderSettings.size() > 0) {
    auto renderSetting = styleConfiguration->renderSettings.back();
    auto &renderSettingsAttr = renderSetting->attr;
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

  // For the generic renderer and when shadeinterval is set, always apply shading.
  if (settings.isUsingShadeIntervals == false && styleConfiguration->shadeIntervals.size() > 0) {
    if (styleConfiguration->renderMethod == RM_GENERIC) {
      settings.isUsingShadeIntervals = true;
    }
  }

  // Smoothingfilter:
  if (styleConfiguration->smoothingFilterVector.size() > 0) {
    auto smoothingFilter = styleConfiguration->smoothingFilterVector.back();

    if (!smoothingFilter->elementValue.empty()) {
      settings.smoothingFiter = atof(smoothingFilter->elementValue.c_str());
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
          settings.intervals.push_back(Interval({.min = atof(shadeInterval.attr.min.c_str()), .max = atof(shadeInterval.attr.max.c_str()), .color = CColor(shadeInterval.attr.fillcolor.c_str())}));
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
