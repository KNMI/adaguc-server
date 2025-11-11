#include <CServerConfig_CPPXSD.h>
#include <cfloat>
#include "getPointStyle.h"

PointStyle getPointStyle(CServerConfig::XMLE_Point *pointCfg, CServerConfig::XMLE_Configuration *cfg) {
  auto attr = pointCfg->attr;

  PointStyle pointStyle = {
      .style = "point",

      .fillColor = CColor(0, 0, 0, 128),
      .lineColor = CColor(0, 0, 0, 255),
      .textOutlineColor = CColor(255, 255, 255, 0),
      .textColor = CColor(0, 0, 0, 255),

      .useTextColor = false,
      .useFillColor = false,

      .fontSize = 8,
      .fontFile = cfg->WMS[0]->ContourFont[0]->attr.location.c_str(),

      .discRadius = 8,
      .textRadius = 16,

      .angleStart = -90,
      .angleStep = 180,
      .useAngles = false,

      .textFormat = "%0.1f",
      .plotStationId = false,

      .symbol = "",

      .maxPointCellSize = attr.maxpointcellsize,
      .maxPointsPerCell = attr.maxpointspercell,

      .min = attr.min,
      .max = attr.max,
  };

  if (attr.pointstyle.empty() == false) {
    pointStyle.style = attr.pointstyle.toLowerCase().c_str();
  }

  if (attr.fillcolor.empty() == false) {
    pointStyle.fillColor = CColor(attr.fillcolor.c_str());
    pointStyle.useFillColor = true;
  }
  if (attr.linecolor.empty() == false) {
    pointStyle.lineColor = CColor(attr.linecolor.c_str());
  }
  if (attr.textcolor.empty() == false) {
    pointStyle.textColor = CColor(attr.textcolor.c_str());
    pointStyle.useTextColor = true;
  }
  if (attr.textoutlinecolor.empty() == false) {
    pointStyle.textOutlineColor = CColor(attr.textoutlinecolor.c_str());
  }
  if (attr.fontsize.empty() == false) {
    pointStyle.fontSize = attr.fontsize.toDouble();
  }
  if (attr.fontfile.empty() == false) {
    pointStyle.fontFile = attr.fontfile.c_str();
  }
  if (attr.discradius.empty() == false) {
    pointStyle.discRadius = attr.discradius.toDouble();
    if (attr.textradius.empty()) {
      pointStyle.textRadius = pointStyle.discRadius + 4;
    }
  }
  if (attr.textradius.empty() == false) {
    pointStyle.textRadius = attr.textradius.toDouble();
  }
  if (attr.anglestart.empty() == false) {
    pointStyle.angleStart = attr.anglestart.toDouble();
    pointStyle.useAngles = true;
  }
  if (attr.anglestep.empty() == false) {
    pointStyle.angleStep = attr.anglestep.toDouble();
    pointStyle.useAngles = true; // TODO: wasn't here before, but seems logical?
  }

  if (attr.textformat.empty() == false) {
    pointStyle.textFormat = attr.textformat.c_str();
  }

  if (attr.plotstationid.empty() == false) {
    pointStyle.plotStationId = attr.plotstationid.equalsIgnoreCase("true");
  }

  if (attr.symbol.empty() == false) {
    pointStyle.symbol = attr.symbol.c_str();
  }

  return pointStyle;
}