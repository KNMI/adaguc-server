#include "CCreateLegend.h"
#include "CDataReader.h"
#include "CImageDataWriter.h"

int CCreateLegend::renderContinuousLegend(CDataSource *dataSource, CDrawImage *legendImage, CStyleConfiguration *styleConfiguration, bool, bool estimateMinMax) {
#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("legendtype continous");
#endif
  bool drawUpperTriangle = true;
  bool drawLowerTriangle = true;

  float fontSize = dataSource->srvParams->cfg->WMS[0]->ContourFont[0]->attr.size.toDouble();
  const char *fontLocation = dataSource->srvParams->cfg->WMS[0]->ContourFont[0]->attr.location.c_str();

  double scaling = dataSource->getScaling();
  float legendHeight = legendImage->Geo->dHeight;
  float cbH = legendHeight - 13 - 13 * scaling;
  float lineWidth = 0.8 * scaling;

  if (styleConfiguration->hasLegendValueRange) {
    float minValue = CImageDataWriter::getValueForColorIndex(dataSource, 0);
    float maxValue = CImageDataWriter::getValueForColorIndex(dataSource, 239);
    if (styleConfiguration->legendLowerRange >= minValue) {
      drawLowerTriangle = false;
    }
    if (styleConfiguration->legendUpperRange <= maxValue) {
      drawUpperTriangle = false;
    }
  }

  float cbW = 16;
  int triangleWidth = ((int)cbW + 1) * scaling;
  float triangleShape = 1.1;

  float triangleHeight = triangleWidth / triangleShape;
  int dH = 0;

  if (drawUpperTriangle) {
    dH += int(triangleHeight);
    cbH -= triangleHeight;
  }
  if (drawLowerTriangle) {
    cbH -= triangleHeight;
  }

  int minColor = 239;
  int maxColor = 0;

  int pLeft = 4;
  int pTop = (int)(legendImage->Geo->dHeight - legendHeight);
  for (int j = 0; j < cbH; j++) {
    int c = int((float(cbH - (j + 1)) / cbH) * 240.0f + 0.5);
    for (int x = pLeft; x < pLeft + ((int)cbW + 1) * scaling; x++) {
      legendImage->setPixelIndexed(x, j + 7 + dH + pTop, c);
    }
  }

  // TODO: Remove scaling, investigate how scaling factor applies
  legendImage->line(pLeft, 6 + dH + pTop, pLeft, (int)cbH + 7 + dH + pTop, lineWidth, CColor(0, 0, 0, 255));
  legendImage->line(((int)cbW + 1) * scaling + pLeft, 6 + dH + pTop, ((int)cbW + 1) * scaling + pLeft, (int)cbH + 7 + dH + pTop, lineWidth, 248);

  int triangleLX = pLeft;                         // start X of triangle
  int triangleRX = pLeft + triangleWidth;         // end X of triangle
  int triangleMX = (triangleRX + triangleLX) / 2; // middle point of triangle (X axis)

  int triangle1BY = 7 + dH + pTop - 1;
  int triangle1TY = int(7 + dH + pTop - triangleHeight);

  int triangle2TY = (int)cbH + 7 + dH + pTop;
  int triangle2BY = int(cbH + 7 + dH + pTop + triangleHeight);

  float triangleSlope = (triangleMX - triangleLX) / (float(triangle1BY - triangle1TY));

  if (drawUpperTriangle) {
    // Draw upper triangle
    for (int j = 0; j < (triangle1BY - triangle1TY) + 1; j++) {
      for (int x = triangleSlope * float(j); x < triangleWidth / 2 + 1; x++) {
        legendImage->setPixelIndexed(x + triangleLX, triangle1BY - j, int(minColor));
        legendImage->setPixelIndexed(triangleRX - x, triangle1BY - j, int(minColor));
      }
    }
    legendImage->line(triangleLX, triangle1BY, triangleMX, triangle1TY - 1, lineWidth, 248);
    legendImage->line(triangleRX, triangle1BY, triangleMX, triangle1TY - 1, lineWidth, 248);
  } else {
    legendImage->line(triangleLX, triangle1BY + 1, triangleRX, triangle1BY + 1, lineWidth, 248);
  }

  if (drawLowerTriangle) {
    // Draw lower triangle
    for (int j = 0; j < (triangle2BY - triangle2TY) + 1; j++) {
      for (int x = triangleSlope * float((triangle2BY - triangle2TY) + 1 - j); x < triangleWidth / 2 + 1; x++) {
        legendImage->setPixelIndexed(x + triangleLX, triangle2BY - j, int(maxColor));
        legendImage->setPixelIndexed(triangleRX - x, triangle2BY - j, int(maxColor));
      }
    }

    legendImage->line(triangleLX, triangle2TY, triangleMX, triangle2BY + 1, lineWidth, 248);
    legendImage->line(triangleRX, triangle2TY, triangleMX, triangle2BY + 1, lineWidth, 248);
  } else {
    legendImage->line(triangleLX, triangle2TY, triangleRX, triangle2TY, lineWidth, 248);
  }

  double classes = 6;
  int tickRound = 0;
  double min = CImageDataWriter::getValueForColorIndex(dataSource, 0);
  double max = CImageDataWriter::getValueForColorIndex(dataSource, 240); // TODO CHECK 239
  if (max == INFINITY) max = 239;
  if (min == INFINITY) min = 0;
  if (max == min) max = max + 0.000001;
  double increment = (max - min) / classes;
  if (styleConfiguration->legendTickInterval > 0) {
    increment = double(styleConfiguration->legendTickInterval);
  }
  if (increment <= 0) increment = 1;

  if (styleConfiguration->legendTickRound > 0) {
    tickRound = int(round(log10(styleConfiguration->legendTickRound)) + 3);
  }

  if (increment > max - min) {
    increment = max - min;
  }
  if ((max - min) / increment > 250) increment = (max - min) / 250;
  if (increment <= 0) {
    CDBDebug("Increment is 0, setting to 1");
    increment = 1;
  }
  classes = abs(int((max - min) / increment));

  char szTemp[256];
  if (styleConfiguration->legendLog != 0) {
    for (int j = 0; j <= classes; j++) {
      double c = ((double(classes - j) / classes)) * (cbH);
      double v = ((double(j) / classes)) * (240.0f);
      v -= styleConfiguration->legendOffset;
      if (styleConfiguration->legendScale != 0) v /= styleConfiguration->legendScale;
      if (styleConfiguration->legendLog != 0) {
        v = pow(styleConfiguration->legendLog, v);
      }
      {
        legendImage->line(((int)cbW - 1) * scaling + pLeft, (int)c + 6 + dH + pTop, ((int)cbW + 6) * scaling + pLeft, (int)c + 6 + dH + pTop, lineWidth, 248);
        if (tickRound == 0) {
          floatToString(szTemp, 255, min, max, v);
        } else {
          floatToString(szTemp, 255, tickRound, v);
        }
        legendImage->drawText(((int)cbW + 12 + pLeft) * scaling, (pTop) - ((fontSize * scaling) / 4) + 3, fontLocation, fontSize * scaling, 0, szTemp, 248);
      }
    }
  } else {

    for (double j = min; j < max + increment; j = j + increment) {
      double lineY = cbH - ((j - min) / (max - min)) * cbH;
      double v = j; // pow(j,10);

      if (lineY >= -2 && lineY < cbH + 2) {
        legendImage->line(((int)cbW - 1) * scaling + pLeft, (int)lineY + 6 + dH + pTop, ((int)cbW + 6) * scaling + pLeft, (int)lineY + 6 + dH + pTop, lineWidth, 248);
        if (tickRound == 0) {
          floatToString(szTemp, 255, min, max, v);
        } else {
          floatToString(szTemp, 255, tickRound, v);
        }
        legendImage->drawText(((int)cbW + 12 + pLeft) * scaling, ((int)lineY + dH + pTop) + ((fontSize * scaling) / 4) + 6, fontLocation, fontSize * scaling, 0, szTemp, 248);
      }
    }
  }
  // Get units
  CT::string units;
  if (dataSource->getDataObject(0)->getUnits().length() > 0) {
    units.concat(dataSource->getDataObject(0)->getUnits().c_str());
  }
  if (units.length() == 0) {
    units = "-";
  }

  legendImage->drawText((2 + pLeft) * scaling, int(legendHeight) - pTop - scaling * 2, fontLocation, fontSize * scaling, 0, units.c_str(), 248);

#ifdef CIMAGEDATAWRITER_DEBUG

  CDBDebug("set units");
#endif
  return 0;
}