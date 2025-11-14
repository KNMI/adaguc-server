#include "GenericDataWarper/GDWDrawFunctionSettings.h"
#include <sys/types.h>

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

  /* Check the if we want to use discrete type */
  if (styleConfiguration->styleConfig != nullptr) {
    auto numRenderSettings = styleConfiguration->styleConfig->RenderSettings.size();
    CDBDebug("numRenderSettings %d", numRenderSettings);
    if (numRenderSettings > 0) {
      auto renderSettings = styleConfiguration->styleConfig->RenderSettings[numRenderSettings - 1];
      auto &renderSettingsAttr = renderSettings->attr;
      if (renderSettingsAttr.renderhint.equals(RENDERHINT_DISCRETECLASSES)) {
        settings.isUsingShadeIntervals = true;
      }
      // Obtain interpolationmethod
      if (renderSettingsAttr.interpolationmethod.equals("nearest")) {
        settings.drawInImage = DrawInImageNearest;
      } else if (renderSettingsAttr.interpolationmethod.equals("bilinear")) {
        settings.drawInImage = DrawInImageBilinear;
      } else if (renderSettingsAttr.interpolationmethod.equals("none")) {
        settings.drawInImage = DrawInImageNone;
      }

      if (renderSettingsAttr.drawgridboxoutline.equals("true")) {
        settings.drawgridboxoutline = true;
      }
    }
    // For the generic renderer and when shadeinterval is set, always apply shading.
    CDBDebug("%d %d", settings.isUsingShadeIntervals, styleConfiguration->shadeIntervals.size());
    if (settings.isUsingShadeIntervals == false && styleConfiguration->shadeIntervals.size() > 0) {
      size_t numRenderSettings = styleConfiguration->styleConfig->RenderMethod.size();
      if (numRenderSettings > 0 && styleConfiguration->renderMethod == RM_GENERIC) {
        settings.isUsingShadeIntervals = true;
      }
    }
  } else {
    CDBWarning("No style configuration");
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
