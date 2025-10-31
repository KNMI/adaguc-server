
#include "getVectorStyle.h"
#include <CServerConfig_CPPXSD.h>
#include <cfloat>

VectorStyle getVectorStyle(CServerConfig::XMLE_Vector *vectorCfg) {

  VectorStyle vectorStyle = {
      .lineColor = CColor(0, 0, 128, 255),
      .lineWidth = vectorCfg->attr.linewidth,
      .outlinecolor = CColor(255, 255, 255, 255),
      .outlinewidth = vectorCfg->attr.outlinewidth,
      .textColor = CColor(0, 0, 0, 255),
      .drawVectorTextFormat = "%0.1f",
      .fontSize = vectorCfg->attr.fontSize,
      .drawVectorPlotStationId = false,
      .drawVectorPlotValue = false,
      .drawBarb = false,
      .drawDiscs = false,
      .drawVector = false,
      .symbolScaling = 1.0,
      .min = vectorCfg->attr.min,
      .max = vectorCfg->attr.max,
  };

  if (!vectorCfg->attr.linecolor.empty()) {
    vectorStyle.lineColor = CColor(vectorCfg->attr.linecolor.c_str());
  }

  if (!vectorCfg->attr.textcolor.empty()) {
    vectorStyle.textColor = CColor(vectorCfg->attr.textcolor.c_str());
  } else {
    vectorStyle.textColor = CColor(vectorCfg->attr.linecolor.c_str());
  }

  if (!vectorCfg->attr.outlinecolor.empty()) {
    vectorStyle.outlinecolor = CColor(vectorCfg->attr.outlinecolor.c_str());
  }

  if (vectorCfg->attr.vectorstyle.empty() == false) {
    if (vectorCfg->attr.vectorstyle.equalsIgnoreCase("barb")) {
      vectorStyle.drawBarb = true;
    } else if (vectorCfg->attr.vectorstyle.equalsIgnoreCase("disc")) {
      vectorStyle.drawDiscs = true;
    } else if (vectorCfg->attr.vectorstyle.equalsIgnoreCase("vector")) {
      vectorStyle.drawVector = true;
    } else {
      vectorStyle.drawBarb = true;
    }
  }

  if (vectorCfg->attr.plotstationid.empty() == false) {
    vectorStyle.drawVectorPlotStationId = vectorCfg->attr.plotstationid.equalsIgnoreCase("true");
  }
  if (vectorCfg->attr.plotvalue.empty() == false) {
    vectorStyle.drawVectorPlotValue = vectorCfg->attr.plotvalue.equalsIgnoreCase("true");
  }
  if (vectorCfg->attr.textformat.empty() == false) {
    vectorStyle.drawVectorTextFormat = vectorCfg->attr.textformat;
  }

  vectorStyle.symbolScaling = vectorCfg->attr.scale;

  return vectorStyle;
}
