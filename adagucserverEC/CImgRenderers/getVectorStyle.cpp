
#include "getVectorStyle.h"
#include <CServerConfig_CPPXSD.h>
#include <cfloat>

VectorStyle getVectorStyle(CServerConfig::XMLE_Vector *vectorCfg, CServerConfig::XMLE_Configuration *globalConfig) {

  VectorStyle vectorStyle = {
    .lineStyle = {
        .lineColor = CColor(0, 0, 0, 255),
        .lineWidth = vectorCfg->attr.linewidth,
        .lineOutlineColor = CColor(255, 255, 255, 255),
        .lineOutlineWidth = vectorCfg->attr.outlinewidth,
      },

      .textStyle = {
        .textColor = CColor(0, 0, 0, 255),
        .fontSize = vectorCfg->attr.fontSize,

        .textOutlineColor = CColor(255, 255, 255, 255),
        .textOutlineWidth = 0,
      },

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

  // TODO: backwards compatible
  if (!vectorCfg->attr.textoutlinewidth.empty()) {
    vectorStyle.textStyle.textOutlineWidth = parseDouble(vectorCfg->attr.textoutlinewidth.c_str());
  } else {
    vectorStyle.textStyle.textOutlineWidth = vectorStyle.lineStyle.lineOutlineWidth;
  }

  if (vectorStyle.lineStyle.lineOutlineWidth <= vectorStyle.lineStyle.lineWidth) {
    CDBWarning("Outline is not visisble for vectors, outline width %.2f <= line width %.2f", vectorStyle.lineStyle.lineOutlineWidth, vectorStyle.lineStyle.lineWidth);
  }

  if (!vectorCfg->attr.fontfile.empty()) {
    vectorStyle.fontFile = vectorCfg->attr.fontfile.c_str();
  } else {
    // Try to get it from global WMS config
    vectorStyle.fontFile = globalConfig->WMS[0]->ContourFont[0]->attr.location.c_str();
  }

  if (!vectorCfg->attr.linecolor.empty()) {
    vectorStyle.lineStyle.lineColor = CColor(vectorCfg->attr.linecolor.c_str());
  }

  if (!vectorCfg->attr.textcolor.empty()) {
    vectorStyle.textStyle.textColor = CColor(vectorCfg->attr.textcolor.c_str());
  } else {
    vectorStyle.textStyle.textColor = CColor(vectorCfg->attr.linecolor.c_str());
  }

  if (!vectorCfg->attr.fillcolor.empty()) {
    vectorStyle.fillColor = CColor(vectorCfg->attr.fillcolor.c_str());
  }
  if (!vectorCfg->attr.outlinecolor.empty()) {
    vectorStyle.lineStyle.lineOutlineColor = CColor(vectorCfg->attr.outlinecolor.c_str());
  }

  if (!vectorCfg->attr.textoutlinecolor.empty()) {
    vectorStyle.textStyle.textOutlineColor = CColor(vectorCfg->attr.textoutlinecolor.c_str());
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
