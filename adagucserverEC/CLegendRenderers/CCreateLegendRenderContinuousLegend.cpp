#include "CCreateLegend.h"
#include "CDataReader.h"
#include "CImageDataWriter.h"
#include <algorithm>

double CCreateLegend::nextTick(double prev) {
  // scale will be between pow(10,floor(prevLog)) and pow(10,ceil(prevLog))
  double prevLog = log10(prev);
  // number 15
  // log (15) = 1.17
  // between 1 (10) and 2 (100)

  // log (1) = 0
  // between 0 (1) and 1 (10)

  double conversion = pow(10, floor(prevLog));
  double tick02 = 2 * conversion;
  if (prev < tick02) {
    return tick02;
  }
  double tick05 = 5 * conversion;
  if (prev < tick05) {
    return tick05;
  }
  return 10 * conversion;
}

char *CCreateLegend::formatTickLabel(CT::string textformatting, char *szTemp, size_t szTempLength, double tick, double min, double max, int tickRound) {
  if (textformatting.empty() == false) {
    CT::string textFormat;
    textFormat.print("%s", textformatting.c_str());
    snprintf(szTemp, szTempLength, textFormat.c_str(), tick);
  } else {
    if (tickRound == 0) {
      floatToString(szTemp, 255, min, max, tick);
    } else {
      floatToString(szTemp, 255, tickRound, tick);
    }
  }
  return szTemp;
}

int CCreateLegend::renderContinuousLegend(CDataSource *dataSource, CDrawImage *legendImage, CStyleConfiguration *styleConfiguration, bool, bool) {
#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("legendtype continous");
#endif
  bool drawUpperTriangle = true;
  bool drawLowerTriangle = true;

  float fontSize;
  std::string fontLocation;
  std::tie(fontSize, fontLocation) = dataSource->srvParams->getLegendFont();
  CT::string textformatting;

  /* Take the textformatting from the Style->Legend configuration */
  if (styleConfiguration != NULL && styleConfiguration->styleConfig != NULL && styleConfiguration->styleConfig->Legend.size() > 0 &&
      !styleConfiguration->styleConfig->Legend[0]->attr.textformatting.empty()) {
    textformatting = styleConfiguration->styleConfig->Legend[0]->attr.textformatting;
  }
  CDBDebug("TextFormatting=%s", textformatting.c_str());
  double scaling = dataSource->getScaling();
  float legendHeight = legendImage->geoParams.height;
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
  int pTop = (int)(legendImage->geoParams.height - legendHeight);
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

  if (styleConfiguration->legendTickRound > 0) {
    tickRound = int(round(log10(styleConfiguration->legendTickRound)) + 3);
  }

  if (std::abs(increment) > std::max(max, min) - std::min(max, min)) {
    increment = max - min;
  }

  if ((max - min) / increment > 250) increment = (max - min) / 250;
  if (increment <= 0) {
    increment = -increment;
  }

  classes = abs(int((max - min) / increment));
  if (increment <= 0) increment = (std::max(max, min) - std::min(max, min)) / 3;

  if (styleConfiguration->legendLog != 0) {
    // vertical axis going from log(min) to log(max)
    // log(intermediate values)
    // Fixed number of intermediate classes: 2,5,10
    // assume log 10
    // 1. Collect all tick values and their formatted labels
    std::vector<CT::string> logLabels;
    std::vector<double> tickValues;

    double tick = min;
    while (tick < max) {
      char temp[256];
      formatTickLabel(textformatting, temp, sizeof(temp), tick, min, max, tickRound);
      logLabels.push_back(CT::string(temp));
      tickValues.push_back(tick);
      tick = nextTick(tick);
    }

    // Include max explicitly
    char temp[256];
    formatTickLabel(textformatting, temp, sizeof(temp), max, min, max, tickRound);
    logLabels.push_back(CT::string(temp));
    tickValues.push_back(max);

    // Calculate widths
    int numberWidth = legendImage->getTextWidth("0", fontLocation.c_str(), fontSize * scaling, 0);
    int minusWidth = legendImage->getTextWidth("-", fontLocation.c_str(), fontSize * scaling, 0);
    int intWidth = maxIntWidth(logLabels);

    // Calculate center (either last decimal digit or decimal dot)
    int colRight = ((int)cbW + pLeft) * scaling + intWidth * numberWidth;
    int columnCenter = colRight - numberWidth;

    // Render the labels, aligned to the center above
    for (size_t i = 0; i < tickValues.size(); ++i) {
      double tickVal = tickValues[i];

      double difference = log(max) - log(tickVal);
      double c = (difference / (log(max) - log(min))) * cbH;
      int labelY = (int)c + 6 + dH + pTop;

      legendImage->line(((int)cbW - 1) * scaling + pLeft, labelY, ((int)cbW + 6) * scaling + pLeft, labelY, lineWidth, 248);

      if (!fontLocation.empty()) {
        const CT::string &label = logLabels[i];
        char tempText[256];
        snprintf(tempText, sizeof(tempText), "%s", label.c_str());

        const char *dotPos = strchr(tempText, '.');
        int leftChars = dotPos ? (dotPos - tempText) : strlen(tempText);

        int textX = columnCenter - (leftChars * numberWidth) + ((int)cbW) * scaling + pLeft;
        if (tempText[0] == '-') {
          textX -= (minusWidth - numberWidth);
        }

        int textY = labelY + 4;
        legendImage->drawText(textX, textY, fontLocation.c_str(), fontSize * scaling, 0, tempText, 248);
      }
    }

  } else {
    bool isInverted = min > max;
    double loopMin = min;
    double loopMax = max;
    if (isInverted) {
      loopMin = -min;
      loopMax = -max;
    }
    std::vector<CT::string> allLabels;
    char tempText[1024];

    // Pre-calculate all labels first
    for (double j = loopMin; j < loopMax + increment; j += increment) {
      double v = isInverted ? -j : j;

      if (!textformatting.empty()) {
        CT::string textFormat;
        textFormat.print("%s", textformatting.c_str());
        snprintf(tempText, sizeof(tempText), textFormat.c_str(), v);
      } else {
        if (tickRound == 0) {
          floatToString(tempText, sizeof(tempText), min, max, v);
        } else {
          floatToString(tempText, sizeof(tempText), tickRound, v);
        }
      }

      allLabels.push_back(CT::string(tempText));
    }

    // Calculate widths
    int numberWidth = legendImage->getTextWidth("0", fontLocation.c_str(), fontSize * scaling, 0);
    int minusWidth = legendImage->getTextWidth("-", fontLocation.c_str(), fontSize * scaling, 0);

    int intWidth = maxIntWidth(allLabels);

    // Calculate center (either last decimal digit or decimal dot)
    int colRight = ((int)cbW + pLeft) * scaling + intWidth * numberWidth;
    int columnCenter = colRight - numberWidth;

    // Render the labels, aligned to the center above
    int labelIndex = 0;
    for (double j = loopMin; j < loopMax + increment; j += increment) {
      double lineY = cbH - ((j - loopMin) / (loopMax - loopMin)) * cbH;
      if (lineY < -2 || lineY >= cbH + 2) continue;

      legendImage->line(((int)cbW - 1) * scaling + pLeft, (int)lineY + 6 + dH + pTop, ((int)cbW + 6) * scaling + pLeft, (int)lineY + 6 + dH + pTop, lineWidth, 248);

      CT::string label = allLabels[labelIndex++];
      snprintf(tempText, sizeof(tempText), "%s", label.c_str());

      const char *dotPos = strchr(tempText, '.');
      int leftChars = dotPos ? (dotPos - tempText) : strlen(tempText);

      int textX = columnCenter - (leftChars * numberWidth) + ((int)cbW) * scaling + pLeft;

      if (tempText[0] == '-') {
        textX -= (minusWidth - numberWidth); // Fix for non-monospaced fonts
      }

      legendImage->drawText(textX, ((int)lineY + dH + pTop) + ((fontSize * scaling) / 4) + 6, fontLocation.c_str(), fontSize * scaling, 0, tempText, 248);
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

  if (!fontLocation.empty()) {
    legendImage->drawText((2 + pLeft) * scaling, int(legendHeight) - pTop - scaling * 2, fontLocation.c_str(), fontSize * scaling, 0, units.c_str(), 248);
  }
#ifdef CIMAGEDATAWRITER_DEBUG

  CDBDebug("set units");
#endif
  return 0;
}