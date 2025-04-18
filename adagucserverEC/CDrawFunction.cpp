#include "CDrawFunction.h"
#include "CImageOperators/smoothRasterField.h"

GDWDrawFunctionSettings getDrawFunctionSettings(CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration) {
  GDWDrawFunctionSettings settings;

  settings.dfNodataValue = dataSource->getDataObject(0)->dfNodataValue;
  settings.legendValueRange = styleConfiguration->hasLegendValueRange;
  settings.legendLowerRange = styleConfiguration->legendLowerRange;
  settings.legendUpperRange = styleConfiguration->legendUpperRange;
  settings.hasNodataValue = dataSource->getDataObject(0)->hasNodataValue;

  settings.legendLog = styleConfiguration->legendLog;
  if (settings.legendLog > 0) {
    settings.legendLogAsLog = log10(settings.legendLog);
  } else {
    settings.legendLogAsLog = 0;
  }
  settings.legendScale = styleConfiguration->legendScale;
  settings.legendOffset = styleConfiguration->legendOffset;
  settings.drawImage = drawImage;
  if (styleConfiguration != NULL && styleConfiguration->styleConfig != NULL && styleConfiguration->styleConfig->RenderSettings.size() == 1) {
    /* Check the if we want to use discrete type */
    auto renderSettings = styleConfiguration->styleConfig->RenderSettings[0];
    if (renderSettings->attr.renderhint.equals(RENDERHINT_DISCRETECLASSES)) {
      settings.isUsingShadeIntervals = true;
    }

    /* Check if we want to interpolate nearest or bilinear */
    if (renderSettings->attr.render.equals(RENDERSETTINGS_RENDER_BILINEAR)) {
      settings.drawInImage = DrawInImageBilinear;

    } else if (renderSettings->attr.render.equals(RENDERSETTINGS_RENDER_SHADED)) {
      settings.drawInImage = DrawInImageBilinear;
    } else if (renderSettings->attr.render.equals(RENDERSETTINGS_RENDER_CONTOUR)) {
      settings.drawInDataGrid = DrawInDataGridBilinear;
    } else {
      settings.drawInImage = DrawInImageNearest;
    }
  }
  if (styleConfiguration != NULL) {
    // CDBDebug("Using styleindex %d", styleConfiguration->styleIndex);
    // CDBDebug("%s", styleConfiguration->dump().c_str());

    for (auto renderSetting : styleConfiguration->renderSettings) {
      /* Check the if we want to use discrete type */
      if (renderSetting->attr.renderhint.equals(RENDERHINT_DISCRETECLASSES)) {
        settings.isUsingShadeIntervals = true;
      }

      /* Check if we want to interpolate nearest or bilinear */
      if (renderSetting->attr.render.equals(RENDERSETTINGS_RENDER_BILINEAR)) {
        settings.drawInImage = DrawInImageBilinear;

      } else if (renderSetting->attr.render.equals(RENDERSETTINGS_RENDER_SHADED)) {
        settings.drawInImage = DrawInImageBilinear;
      } else if (renderSetting->attr.render.equals(RENDERSETTINGS_RENDER_CONTOUR)) {
        settings.drawInDataGrid = DrawInDataGridBilinear;
      } else {
        settings.drawInImage = DrawInImageNearest;
      }
    }
  }

  /* Make a shorthand vector from the shadeInterval configuration*/
  if (settings.isUsingShadeIntervals) {
    int numShadeDefs = (int)styleConfiguration->shadeIntervals.size();
    if (numShadeDefs > 0) {
      if (((styleConfiguration->shadeIntervals)[0])->value.empty()) {
        settings.intervals.reserve(numShadeDefs);
        for (int j = 0; j < numShadeDefs; j++) {
          CServerConfig::XMLE_ShadeInterval *shadeInterVal = ((styleConfiguration->shadeIntervals)[j]);
          settings.intervals.push_back(Interval(shadeInterVal->attr.min.toFloat(), shadeInterVal->attr.max.toFloat(), CColor(shadeInterVal->attr.fillcolor.c_str())));
          /* Check for bgcolor */
          if (j == 0) {
            if (shadeInterVal->attr.bgcolor.empty() == false) {
              settings.bgColorDefined = true;
              settings.bgColor = CColor(shadeInterVal->attr.bgcolor.c_str());
            }
          }
        }
      } else {
        settings.shadeInterval = ((styleConfiguration->shadeIntervals)[0])->value.toDouble();
      }
    }
  }

  settings.smoothingFiter = 0;
  settings.smoothingDistanceMatrix = nullptr; // smoothingMakeDistanceMatrix(settings.smoothingFiter);
  return settings;
}
