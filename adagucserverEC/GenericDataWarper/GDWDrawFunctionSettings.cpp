#include "GenericDataWarper/GDWDrawFunctionSettings.h"
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
    if (renderSettingsAttr.renderhint.equals(RENDERHINT_DISCRETECLASSES)) {
      settings.isUsingShadeIntervals = true;
    }
    // Obtain interpolationmethod
    if (renderSettingsAttr.interpolationmethod.equals("nearest")) {
      settings.interpolationMethod = InterpolationMethodNearest;
    } else if (renderSettingsAttr.interpolationmethod.equals("bilinear")) {
      settings.interpolationMethod = InterpolationMethodBilinear;
    }
    if (renderSettingsAttr.drawgrid.equals("false")) {
      settings.drawgrid = false;
    } else if (renderSettingsAttr.drawgrid.equals("true")) {
      settings.drawgrid = true;
    }

    if (renderSettingsAttr.drawgridboxoutline.equals("true")) {
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
    if (settings.smoothingDistanceMatrix == nullptr) {
      if (!smoothingFilter->value.empty()) {
        settings.smoothingFiter = smoothingFilter->value.toDouble();
        smoothingMakeDistanceMatrix(settings);
      }
    }
  }

  /* Make a shorthand vector from the shadeInterval configuration*/
  if (settings.isUsingShadeIntervals) {
    int numShadeDefs = (int)styleConfiguration->shadeIntervals.size();
    if (numShadeDefs == 1 && !styleConfiguration->shadeIntervals[0]->value.empty()) {
      settings.shadeInterval = styleConfiguration->shadeIntervals[0]->value.toDouble();
    } else {
      settings.intervals.reserve(numShadeDefs);
      for (int j = 0; j < numShadeDefs; j++) {
        CServerConfig::XMLE_ShadeInterval *shadeInterVal = styleConfiguration->shadeIntervals[j];
        settings.intervals.push_back(Interval({.min = shadeInterVal->attr.min.toFloat(), .max = shadeInterVal->attr.max.toFloat(), .color = CColor(shadeInterVal->attr.fillcolor.c_str())}));
        /* Check for bgcolor */
        if (j == 0) {
          if (shadeInterVal->attr.bgcolor.empty() == false) {
            settings.bgColorDefined = true;
            settings.bgColor = CColor(shadeInterVal->attr.bgcolor.c_str());
          }
        }
      }
    }
  }

  return settings;
}
