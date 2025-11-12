#include "GenericDataWarper/GDWDrawFunctionSettings.h"
#include "CImageOperators/smoothRasterField.h"
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
  if (styleConfiguration != NULL && styleConfiguration->styleConfig != NULL && styleConfiguration->styleConfig->RenderSettings.size() == 1) {
    CT::string renderHint = styleConfiguration->styleConfig->RenderSettings[0]->attr.renderhint;
    if (renderHint.equals(RENDERHINT_DISCRETECLASSES)) {
      settings.isUsingShadeIntervals = true;
    }
  }

  /* Make a shorthand vector from the shadeInterval configuration*/
  if (settings.isUsingShadeIntervals) {
    int numShadeDefs = (int)styleConfiguration->shadeIntervals.size();
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
  return settings;
}
