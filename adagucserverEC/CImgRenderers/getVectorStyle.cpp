
#include "getVectorStyle.h"
#include <CServerConfig_CPPXSD.h>
#include <cfloat>

VectorStyle getVectorStyle(CServerConfig::XMLE_Vector *vectorCfg, CServerConfig::XMLE_Configuration *globalConfig) {

  VectorStyle vectorStyle = {
      .lineColor = CColor(0, 0, 0, 255),
      .lineWidth = vectorCfg->attr.linewidth,
      .outlineColor = CColor(255, 255, 255, 255),
      .outlineWidth = vectorCfg->attr.outlinewidth,

      .textStyle = {
        .textColor = CColor(0, 0, 0, 255),
        .fontSize = vectorCfg->attr.fontSize,

        .textOutlineColor = CColor(255, 255, 255, 255),
        .textOutlineWidth = vectorCfg->attr.textoutlinewidth,
      }

      // .textOutlineColor = CColor(255, 255, 255, 255),
      // .textOutlineWidth = vectorCfg->attr.textoutlinewidth,
      // .textColor = CColor(0, 0, 0, 255),
      .fillColor = CColor(0, 0, 0, 128),
      .drawVectorTextFormat = "%0.1f",
      .fontFile = "",
      // .fontSize = vectorCfg->attr.fontSize,
      .drawVectorPlotStationId = false,
      .drawVectorPlotValue = false,
      .drawBarb = false,
      .drawDiscs = false,
      .drawVector = false,
      .discRadius = 20,
      .symbolScaling = 1.0,
      .min = vectorCfg->attr.min,
      .max = vectorCfg->attr.max,
  };

  if (vectorStyle.outlineWidth <= vectorStyle.lineWidth) {
    CDBWarning("Outline is not visisble for vectors, outline width %.2f <= line width %.2f", vectorStyle.outlineWidth, vectorStyle.lineWidth);
  }

  if (!vectorCfg->attr.fontfile.empty()) {
    vectorStyle.fontFile = vectorCfg->attr.fontfile.c_str();
  } else {
    // Try to get it from global WMS config
    vectorStyle.fontFile = globalConfig->WMS[0]->ContourFont[0]->attr.location.c_str();
  }

  if (!vectorCfg->attr.linecolor.empty()) {
    vectorStyle.lineColor = CColor(vectorCfg->attr.linecolor.c_str());
  }

  if (!vectorCfg->attr.textcolor.empty()) {
    vectorStyle.textColor = CColor(vectorCfg->attr.textcolor.c_str());
  } else {
    vectorStyle.textColor = CColor(vectorCfg->attr.linecolor.c_str());
  }

  if (!vectorCfg->attr.fillcolor.empty()) {
    vectorStyle.fillColor = CColor(vectorCfg->attr.fillcolor.c_str());
  }
  if (!vectorCfg->attr.outlinecolor.empty()) {
    vectorStyle.outlineColor = CColor(vectorCfg->attr.outlinecolor.c_str());
  }

  if (!vectorCfg->attr.textoutlinecolor.empty()) {
    vectorStyle.textOutlineColor = CColor(vectorCfg->attr.textoutlinecolor.c_str());
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

  vectorStyle.discRadius = vectorCfg->attr.discradius;

  return vectorStyle;
}
