#include "CCreateScaleBar.h"
int CCreateScaleBar::createScaleBar(CDrawImage *scaleBarImage, CGeoParams &geoParams, float scaling) {

  CCreateScaleBar::Props p = CCreateScaleBar::getScaleBarProperties(geoParams, scaling);

  int offsetX = int(3.0f * scaling);
  int scaleBarHeight = int(23.0f * scaling);

  // Draw horizontal base, thick line
  scaleBarImage->line(offsetX - 0.5, scaleBarHeight - 2.0f * scaling, p.width * 2.0f + offsetX + 0.5, scaleBarHeight - int(2.0f * scaling), scaling * 2, 240);

  // Draw four horizontal smaller subdivisions
  float subDivXW = p.width / 5.0f;
  for (int j = 1; j < 5; j++) {
    scaleBarImage->line(offsetX + subDivXW * float(j), scaleBarHeight - 2.0f * scaling, offsetX + subDivXW * float(j), scaleBarHeight - (5.0f * scaling), scaling * .5, 240);
  }

  // Draw three bigger divisions
  scaleBarImage->line(offsetX, scaleBarHeight - 1.0f * scaling, offsetX, scaleBarHeight - 9.0f * scaling, scaling * 1, 240);
  scaleBarImage->line(offsetX + p.width * 1.0f, scaleBarHeight - 2.0f * scaling, offsetX + p.width * 1.0f, scaleBarHeight - 9.0f * scaling, scaling * 1, 240);
  scaleBarImage->line(offsetX + p.width * 2.0f, scaleBarHeight - 1.0f * scaling, offsetX + p.width * 2.0f, scaleBarHeight - 9.0f * scaling, scaling * 1, 240);

  // Draw text
  CT::string units = "";
  CT::string projection = geoParams.crs.c_str();
  if (projection.equals("EPSG:3411")) units = "meter";
  if (projection.equals("EPSG:3412")) units = "meter";
  if (projection.equals("EPSG:3575")) units = "meter";
  if (projection.equals("EPSG:4326")) units = "degrees";
  if (projection.equals("EPSG:28992")) units = "meter";
  if (projection.equals("EPSG:32661")) units = "meter";
  if (projection.equals("EPSG:3857")) units = "meter";
  if (projection.equals("EPSG:900913")) units = "meter";
  if (projection.equals("EPSG:102100")) units = "meter";

  if (units.equals("meter")) {
    if (p.mapunits > 1000) {
      p.mapunits /= 1000;
      units = "km";
    }
  }

  CT::string valueStr;
  const char *fontFile = scaleBarImage->getFontLocation();
  ;
  float fontSize = scaleBarImage->getFontSize() * scaling;
  scaleBarImage->drawText(offsetX - 2.5 * scaling, scaleBarHeight - 11.0f * scaling, fontFile, fontSize * .7, 0, "0", 240);

  valueStr.print("%g", (p.mapunits));
  scaleBarImage->drawText(offsetX + p.width * 1.0f - valueStr.length() * (2.0 * scaling) + 0, scaleBarHeight - 11.0f * scaling, fontFile, fontSize * .7, 0, valueStr.c_str(), 240);

  valueStr.print("%g", (p.mapunits * 2));
  scaleBarImage->drawText(offsetX + p.width * 2.0f - valueStr.length() * (2.0 * scaling) + 0, scaleBarHeight - 11.0f * scaling, fontFile, fontSize * .7, 0, valueStr.c_str(), 240);

  scaleBarImage->drawText(offsetX + p.width * 2.0f + 10, scaleBarHeight - (3.0f * scaling), fontFile, fontSize * .7, 0, units.c_str(), 240);
  scaleBarImage->crop(4 * scaling);
  return 0;
}

CCreateScaleBar::Props CCreateScaleBar::getScaleBarProperties(CGeoParams &geoParams, float scaling) {
  double desiredWidth = 25 * scaling;
  double realWidth = 0;
  double numMapUnits = 1. / 10000000.;

  double boxWidth = geoParams.bbox.span().x;
  double pixelsPerUnit = double(geoParams.width) / boxWidth;
  if (pixelsPerUnit <= 0) {
    throw(__LINE__);
  }

  double a = (desiredWidth / pixelsPerUnit);

  double divFactor = 0;
  do {
    numMapUnits *= 10.;
    divFactor = a / numMapUnits;
    if (divFactor == 0) throw(__LINE__);
    realWidth = desiredWidth / divFactor;

  } while (realWidth < desiredWidth);

  do {
    numMapUnits /= 2.;
    divFactor = a / numMapUnits;
    if (divFactor == 0) throw(__LINE__);
    realWidth = desiredWidth / divFactor;

  } while (realWidth > desiredWidth);

  do {
    numMapUnits *= 1.2;
    divFactor = a / numMapUnits;
    if (divFactor == 0) throw(__LINE__);
    realWidth = desiredWidth / divFactor;

  } while (realWidth < desiredWidth);

  double roundedMapUnits = numMapUnits;

  double d = pow(10, round(log10(numMapUnits) + 0.5) - 1);

  roundedMapUnits = round(roundedMapUnits / d);
  if (roundedMapUnits < 2.5) roundedMapUnits = 2.5;
  if (roundedMapUnits > 2.5 && roundedMapUnits < 7.5) roundedMapUnits = 5;
  if (roundedMapUnits > 7.5) roundedMapUnits = 10;
  roundedMapUnits = (roundedMapUnits * d);

  divFactor = (desiredWidth / pixelsPerUnit) / roundedMapUnits;
  realWidth = desiredWidth / divFactor;
  Props p;

  p.width = realWidth;

  p.mapunits = roundedMapUnits;

  return p;
}
