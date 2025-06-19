#include "CCreateLegend.h"
#include "CDataReader.h"
#include "CImageDataWriter.h"
#include "numericutils.h"

#define MIN_SHADE_CLASS_BLOCK_SIZE 3
#define MAX_SHADE_CLASS_BLOCK_SIZE 12

// Aux function to calculate block height based on total height and number
// of classes in a legend based on shade classes.
float calculateShadeClassBlockHeight(int totalHeight, int intervals) {
  float blockHeight = float(totalHeight - 30) / float(intervals);
  /* Legend classes displayed as blocks in the legend can have a maximum
  and a minimum height */
  if (blockHeight > MAX_SHADE_CLASS_BLOCK_SIZE) return MAX_SHADE_CLASS_BLOCK_SIZE;
  if (blockHeight < MIN_SHADE_CLASS_BLOCK_SIZE) return MIN_SHADE_CLASS_BLOCK_SIZE;
  return blockHeight;
}

std::tuple<int, int> calculateShadedClassLegendClipping(int minValue, int maxValue, CStyleConfiguration *styleConfiguration) {
  // Calculate which part of the legend to draw (only between min and max)
  int minInterval = 0;
  int maxInterval = styleConfiguration->shadeIntervals->size();
  for (size_t j = 0; j < styleConfiguration->shadeIntervals->size(); j++) {
    CServerConfig::XMLE_ShadeInterval *s = (*styleConfiguration->shadeIntervals)[j];
    if (!s->attr.min.empty() && !s->attr.max.empty()) {
      float intervalMinf = parseFloat(s->attr.min.c_str());
      float intervalMaxf = parseFloat(s->attr.max.c_str());
      if (intervalMaxf >= maxValue && intervalMinf <= maxValue) {
        maxInterval = j + 1;
      }
      if (intervalMinf <= minValue && intervalMaxf >= minValue) {
        minInterval = j;
      }
    }
  }
  return std::make_tuple(minInterval, maxInterval);
}

int CCreateLegend::renderDiscreteLegend(CDataSource *dataSource, CDrawImage *legendImage, CStyleConfiguration *styleConfiguration, bool, bool estimateMinMax) {
#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("legendtype discrete");
#endif
  double scaling = dataSource->getScaling();
  float cbW = 20; // legendWidth/8;
  float legendHeight = legendImage->Geo->dHeight;
  float cbH = legendHeight - 13 - 13 * scaling;

  int pLeft = 4;
  int pTop = (int)(legendImage->Geo->dHeight - legendHeight);
  size_t szTempLength = 256;
  char szTemp[szTempLength];

  float fontSize;
  std::string fontLocation;
  std::tie(fontSize, fontLocation) = dataSource->srvParams->getLegendFont();

  CT::string textformatting;

  /* Take the textformatting from the Style->Legend configuration */
  if (styleConfiguration != NULL && styleConfiguration->styleConfig != NULL && styleConfiguration->styleConfig->Legend.size() > 0 &&
      !styleConfiguration->styleConfig->Legend[0]->attr.textformatting.empty()) {
    textformatting = styleConfiguration->styleConfig->Legend[0]->attr.textformatting;
  }

  // CDBDebug("styleTitle = [%s]", styleConfiguration->styleTitle.c_str());
  // CDBDebug("ShadeInterval = [%s]", styleConfiguration->styleConfig->ShadeInterval[0]->value.c_str());
  // CDBDebug("textformatting = [%s]", textformatting.c_str());

  // int dH=0;
  // cbW = 90.0/3.0;
  // We always need to have the min/max of the data
  // Always to show only the occuring data values in the legend,
  // and in some cases to stretch the colors over min max

  // Get the min/max values
  float minValue = CImageDataWriter::getValueForColorIndex(dataSource, 0);
  float maxValue = CImageDataWriter::getValueForColorIndex(dataSource, 239);
  if (styleConfiguration->legendHasFixedMinMax == false) {
    estimateMinMax = true;
  }
  if (estimateMinMax) {
    if (dataSource->statistics == NULL) {
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->calculate(dataSource);
    }
    minValue = (float)dataSource->statistics->getMinimum();
    maxValue = (float)dataSource->statistics->getMaximum();
  }
  // }

  // CDBDebug("Using %f and %f for legend values", minValue, maxValue);

  // Calculate the number of classes
  float legendInterval = styleConfiguration->shadeInterval;
  int numClasses = 8; // Default number of classes, if legendInterval not specified
  if (legendInterval != 0) {
    numClasses = int((maxValue - minValue) / legendInterval);
  }

/*
 *   // and reduce the number of classes when required...
 *   if(!dataSource->stretchMinMax){
 *     while(numClasses>15){
 *       legendInterval*=2;//(maxValue-minValue);
 *       numClasses=int((maxValue-minValue)/legendInterval);
}
}*/
#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("LayerName = %s", dataSource->layerName.c_str());
  CDBDebug("minValue=%f maxValue=%f", minValue, maxValue);
  CDBDebug("scale=%f offset=%f", styleConfiguration->legendScale, styleConfiguration->legendOffset);
#endif
  float iMin = convertValueToClass(minValue, legendInterval);
  float iMax = convertValueToClass(maxValue, legendInterval) + legendInterval;

#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("iMin=%f iMax=%f", iMin, iMax);
#endif

  // In case of auto scale and autooffset we will stretch the colors over the min/max values
  // Calculate new scale and offset for the new min/max:
  // When using min/max stretching, the classes need to be extended according to its shade itnerval
  if (dataSource->stretchMinMax == true) {
    // Calculate new scale and offset for the new min/max:
    float ls = 240 / ((iMax - iMin));
    float lo = -(iMin * ls);
    if (ls == 0) {
      ls = 240;
      lo = 0;
    }
    styleConfiguration->legendScale = ls;
    styleConfiguration->legendOffset = lo;
    // Check for infinities
    if (styleConfiguration->legendScale != styleConfiguration->legendScale || styleConfiguration->legendScale == INFINITY || styleConfiguration->legendScale == NAN ||
        styleConfiguration->legendScale == -INFINITY || styleConfiguration->legendOffset != styleConfiguration->legendOffset || styleConfiguration->legendOffset == INFINITY ||
        styleConfiguration->legendOffset == NAN || styleConfiguration->legendOffset == -INFINITY) {
      styleConfiguration->legendScale = 240.0;
      styleConfiguration->legendOffset = 0;
    }
  }

  bool discreteLegendOnInterval = false;
  bool definedLegendOnShadeClasses = false;
  bool definedLegendForFeatures = false;

  if (styleConfiguration->shadeIntervals != NULL) {
    if (styleConfiguration->shadeIntervals->size() > 0) {
      definedLegendOnShadeClasses = true;
    }
  }
  if (styleConfiguration->featureIntervals != NULL) {
    if (styleConfiguration->featureIntervals->size() > 0) {
      definedLegendForFeatures = true;
    }
  }
  if (legendInterval != 0) {
    discreteLegendOnInterval = true;
  }

#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("legendtype settext");
#endif
  /**
   * Defined blocks based on defined interval
   */
  if (definedLegendOnShadeClasses) {
    // If this type of legend has too many classes (15+) with a relatively small height (<=500 px), we
    // will treat it differently to create a summarised version, with the following characteristics:
    // - The class border is removed for extra visibility
    // - Labels are simplified and will only show the min of the interval the class represents
    // - One in every five labels will be displayed (for classes that are multiples of 5)
    // - If the cliplegend render option is set, only classes with the min and the max data value will be added
    char szTemp[1024];

    // Initial estimation of block height
    float initialBlockHeight = calculateShadeClassBlockHeight(legendImage->Geo->dHeight, styleConfiguration->shadeIntervals->size());

    // Based on the render settings, we can clip the values on the legend to only include the values
    // present in the data
    int minInterval = 0;
    int maxInterval = styleConfiguration->shadeIntervals->size();
    int angle = 0; // Text angle (in radians)

    if (styleConfiguration->styleConfig != NULL && styleConfiguration->styleConfig->RenderSettings.size() == 1 && styleConfiguration->styleConfig->RenderSettings[0]->attr.cliplegend.equals("true")) {
      std::tie(minInterval, maxInterval) = calculateShadedClassLegendClipping(minValue, maxValue, styleConfiguration);
    }
    // Only a subset of intervals to appear on screen to save space
    size_t drawIntervals = maxInterval - minInterval;
    // With a small blockHeight, use the compact legend for better visibility
    if (initialBlockHeight <= MIN_SHADE_CLASS_BLOCK_SIZE + 1) {
      // For right alignment of labels
      int maxTextWidth = 0;
      char tempText[1000];

      // Calculate the max text width, we can probably simplify this because monospace
      for (size_t j = 0; j < drawIntervals; j++) {
        CDBDebug("MAX WIDTH CALCULATION");
        size_t realj = minInterval + j;
        CServerConfig::XMLE_ShadeInterval *s = (*styleConfiguration->shadeIntervals)[realj];
        if (!s->attr.min.empty() && !s->attr.max.empty()) {
          if ((int)(std::abs(parseFloat(s->attr.min.c_str()))) % 5 != 0) {
            continue;
          }
          snprintf(tempText, 1000, "%s", s->attr.min.c_str());
          int textWidth = legendImage->getTextWidth(tempText, fontLocation.c_str(), fontSize * scaling, angle);
          CDBDebug("Width of %s = %d", tempText, textWidth);
          if (textWidth > maxTextWidth) maxTextWidth = textWidth;
        }
        CDBDebug("maxTextWidth = %d", maxTextWidth);
      }

      std::vector<CT::string> minColumn = extractColumn(drawIntervals, minInterval, styleConfiguration->shadeIntervals, true);
      std::vector<CT::string> maxColumn = extractColumn(drawIntervals, minInterval, styleConfiguration->shadeIntervals, false);

      // Recalculate block size based on the final intervals to appear on the legend
      float blockHeight = calculateShadeClassBlockHeight(legendImage->Geo->dHeight, drawIntervals - 1);
      int dashWidth = legendImage->getTextWidth("-", fontLocation.c_str(), fontSize * scaling, angle);
      int dotWidth = legendImage->getTextWidth(".", fontLocation.c_str(), fontSize * scaling, angle);
      // Assume monospaced for numbers
      int numberWidth = legendImage->getTextWidth("0", fontLocation.c_str(), fontSize * scaling, angle);

      for (size_t j = 0; j < drawIntervals; j++) {
        size_t realj = minInterval + j;
        CServerConfig::XMLE_ShadeInterval *s = (*styleConfiguration->shadeIntervals)[realj];
        if (s->attr.min.empty() == false && s->attr.max.empty() == false) {
          int cY1 = int(cbH - ((j)*blockHeight) * scaling);
          int cY2 = int(cbH - ((((j + 1) * blockHeight) - 2)) * scaling);
          CColor color;
          if (s->attr.fillcolor.empty() == false) {
            color = CColor(s->attr.fillcolor.c_str());
          } else {
            color = legendImage->getColorForIndex(CImageDataWriter::getColorIndexForValue(dataSource, parseFloat(s->attr.min.c_str())));
          }
          // This rectangle is borderless, and the resulting classes have no vertical blank space between them
          legendImage->rectangle(4 * scaling + pLeft, cY2 + pTop - 1, (int(cbW) + 7) * scaling + pLeft, cY1 + pTop + 1, color, color);
          // We print every label containing a multiple of 5.
          if ((int)(std::abs(parseFloat(s->attr.min.c_str()))) % 5 != 0) {
            continue;
          }
          snprintf(szTemp, 1000, "%s", s->attr.min.c_str());
          int textWidth = legendImage->getTextWidth(szTemp, fontLocation.c_str(), fontSize * scaling, angle);
          int textX = ((int)cbW + 12 + pLeft) * scaling + (maxTextWidth - textWidth);
          legendImage->drawText(textX, (cY1 + pTop) - ((fontSize * scaling) / 4) + 3, fontLocation.c_str(), fontSize * scaling, angle, szTemp, 248);
        }
      }
    } else {
      // General case for this type of legend, where we draw every label and print every class interval
      // We can also clip this type of legend
      char tempText[1000];
      int maxTextWidthMin = 0;
      int maxTextWidthMax = 0;

      for (size_t j = 0; j < drawIntervals; j++) {
        size_t realj = minInterval + j;
        CServerConfig::XMLE_ShadeInterval *s = (*styleConfiguration->shadeIntervals)[realj];

        if (!s->attr.min.empty() && !s->attr.max.empty()) {
          if ((int)(std::abs(parseFloat(s->attr.min.c_str()))) % 5 != 0) {
            continue;
          }

          // Note: This can probably be simplified because it's monospace (!)
          // Note2: This couldn't work well if we had decimals (maybe not needed? otherwise center around .)
          snprintf(tempText, sizeof(tempText), "%s", s->attr.min.c_str());
          int textWidthMin = legendImage->getTextWidth(tempText, fontLocation.c_str(), fontSize * scaling, angle);
          if (textWidthMin > maxTextWidthMin) {
            maxTextWidthMin = textWidthMin;
          }

          // Measure max text width
          snprintf(tempText, sizeof(tempText), "%s", s->attr.max.c_str());
          int textWidthMax = legendImage->getTextWidth(tempText, fontLocation.c_str(), fontSize * scaling, angle);
          if (textWidthMax > maxTextWidthMax) {
            maxTextWidthMax = textWidthMax;
          }
        }
      }

      float blockHeight = calculateShadeClassBlockHeight(legendImage->Geo->dHeight, drawIntervals);
      int dashWidth = legendImage->getTextWidth("-", fontLocation.c_str(), fontSize * scaling, angle);
      int dotWidth = legendImage->getTextWidth(".", fontLocation.c_str(), fontSize * scaling, angle);
      // Assume monospaced for numbers
      int numberWidth = legendImage->getTextWidth("0", fontLocation.c_str(), fontSize * scaling, angle);

      // We calculate the min column
      // Convert the min into an array of CT::string
      std::vector<CT::string> minColumn;
      for (size_t j = 0; j < drawIntervals; j++) {
        size_t realj = minInterval + j;
        CServerConfig::XMLE_ShadeInterval *s = (*styleConfiguration->shadeIntervals)[realj];
        if (!s->attr.min.empty() && !s->attr.max.empty()) {
          if ((int)(std::abs(parseFloat(s->attr.min.c_str()))) % 5 != 0) {
            continue;
          }
          minColumn.push_back(s->attr.min.c_str());
        }
      }

      std::vector<CT::string> maxColumn = extractColumn(drawIntervals, minInterval, styleConfiguration->shadeIntervals, false);

      for (size_t j = 0; j < drawIntervals; j++) {
        size_t realj = minInterval + j;
        CServerConfig::XMLE_ShadeInterval *s = (*styleConfiguration->shadeIntervals)[realj];
        if (s->attr.min.empty() == false && s->attr.max.empty() == false) {
          int cY1 = int(cbH - ((j)*blockHeight) * scaling);
          int cY2 = int(cbH - ((((j + 1) * blockHeight) - 2)) * scaling);
          CColor color;
          if (s->attr.fillcolor.empty() == false) {
            color = CColor(s->attr.fillcolor.c_str());
          } else {
            color = legendImage->getColorForIndex(CImageDataWriter::getColorIndexForValue(dataSource, parseFloat(s->attr.min.c_str())));
          }
          legendImage->rectangle(4 * scaling + pLeft, cY2 + pTop, (int(cbW) + 7) * scaling + pLeft, cY1 + pTop, color, CColor(0, 0, 0, 255));

          // TODO: How to measure font width of a text when not using CAIRO(?) and how to configure a test for this case
          if (s->attr.label.empty()) {
            snprintf(szTemp, 1000, "%s - %s", s->attr.min.c_str(), s->attr.max.c_str());
            int textY = (cY1 + pTop) - ((fontSize * scaling) / 4) + 3;

            CDBDebug("Drawing min");

            // Right edge of the min column
            int colRightMin = ((int)cbW + pLeft) * scaling + maxIntWidth(minColumn) * numberWidth;
            // int columnCenterMin = colRightMin - maxDecimalWidth(minColumn) * numberWidth;
            int columnCenterMin = colRightMin - numberWidth; // - maxDecimalWidth(minColumn) * numberWidth;

            // Draw min, dot-aligned
            float numericMinVal = atof(s->attr.min.c_str());

            // Calculate number of decimals for min column
            std::string floatFormat = "%." + std::to_string(maxDecimalWidth(minColumn)) + "f";

            snprintf(szTemp, sizeof(szTemp), floatFormat.c_str(), numericMinVal);
            const char *dotPos = strchr(szTemp, '.');
            int leftCharsMin = dotPos ? (dotPos - szTemp) : strlen(szTemp); // chars before dot

            // Positioning (leaving space for the class rectangle)
            int textXMin = columnCenterMin - (leftCharsMin * numberWidth);
            textXMin = textXMin + (int(cbW)) * scaling + pLeft;

            legendImage->drawText(textXMin, textY, fontLocation.c_str(), fontSize * scaling, angle, szTemp, 248);

            // Central dash
            CDBDebug("Drawing dash");
            int dashX = colRightMin + (maxDecimalWidth(minColumn) + 4) * numberWidth; // Leave gap between min column and this dash

            legendImage->drawText(dashX, textY, fontLocation.c_str(), fontSize * scaling, angle, "–", 248);

            // Max column (to the right)
            CDBDebug("Drawing max");

            // Right edge of the max column (min column + extra spacing + max column width)
            int colRightMax = colRightMin + maxTextWidthMax;
            // Calculate column center for max value
            int columnCenterMax = colRightMax + numberWidth * 2; //  - numberWidth; // colRightMax - maxDecimalWidth(maxColumn) * numberWidth;

            // Draw max, dot-aligned
            float numericMaxVal = atof(s->attr.max.c_str());
            std::string floatFormatMax = "%." + std::to_string(maxDecimalWidth(maxColumn)) + "f";
            snprintf(szTemp, sizeof(szTemp), floatFormatMax.c_str(), numericMaxVal);

            const char *dotPosMax = strchr(szTemp, '.');
            int leftCharsMax = dotPosMax ? (dotPosMax - szTemp) : strlen(szTemp);

            // Align max string so that the dot falls on the column center
            int textXMax = columnCenterMax - (leftCharsMax * numberWidth);

            // Apply overall left offset plus some spacing
            textXMax += ((int)cbW + pLeft) * scaling + (maxDecimalWidth(maxColumn) + 1) * numberWidth; // Think of the 15 number

            legendImage->drawText(textXMax, textY, fontLocation.c_str(), fontSize * scaling, angle, szTemp, 248);

          } else {
            // Do not align to the right: this is a non-numeric label
            snprintf(szTemp, 1000, "%s", s->attr.label.c_str());
            legendImage->drawText(((int)cbW + 12 + pLeft) * scaling, (cY1 + pTop) - ((fontSize * scaling) / 4) + 3, fontLocation.c_str(), fontSize * scaling, 0, szTemp, 248);
          }
        }
      }
    }
  }

  if (definedLegendForFeatures) {

    char szTemp[1024];
    for (size_t j = 0; j < styleConfiguration->featureIntervals->size(); j++) {
      CServerConfig::XMLE_FeatureInterval *s = (*styleConfiguration->featureIntervals)[j];
      //         if(s->attr.min.empty()==false&&s->attr.max.empty()==false){
      int cY1 = int(cbH - (j * 12));
      int cY2 = int(cbH - (((j + 1) * 12) - 2));
      CColor color;

      color = CColor(s->attr.fillcolor.c_str());

      legendImage->rectangle(4 + pLeft, cY2 + pTop, int(cbW) + 7 + pLeft, cY1 + pTop, color, CColor(0, 0, 0, 255));
      /*
           if(s->attr.label.empty()){
             snprintf(szTemp,1000,"%s - %s",s->attr.min.c_str(),s->attr.max.c_str());
           }else{
             snprintf(szTemp,1000,"%s",s->attr.label.c_str());
           }*/
      snprintf(szTemp, 1000, "%s", s->attr.label.c_str());
      legendImage->setText(szTemp, strlen(szTemp), int(cbW) + 12 + pLeft, cY2 + pTop, 248, -1);
    }
  }

  if (discreteLegendOnInterval) {
    /*
     * maxIterations is the maximum number of labels in the legendgraphic, more makes no sense as legend image is quite small
     * Sometimes this function tries to render thousands of labels in a few pixels, maxIterations prevents this.
     */
    int maxIterations = 250;
    int currentIteration = 0;

    /**
     * Discrete blocks based on continous interval
     */

    numClasses = int((iMax - iMin) / legendInterval);

    int classSizeY = ((legendHeight - 24) / (numClasses));
    if (classSizeY > 12) classSizeY = 12;
    classSizeY = (int)((float)classSizeY * scaling);

    // Rounding of legend text depends on legendInterval
    if (legendInterval == 0) {
      CDBError("legendInterval is zero");
      return 1;
    }
    int textRounding = 0;
    if (legendInterval != 0) {
      float fracPart = legendInterval - int(legendInterval);
      if (fracPart == 0) {
        textRounding = 0;
      } else {
        textRounding = -int(log10(fracPart) - 0.9999999f);
      }
    }

    int classNr = 0;
    for (float j = iMin; j < iMax + legendInterval; j = j + legendInterval) {
      currentIteration++;
      if (currentIteration > maxIterations) break;
      float v = j;
      int boxLowerY = int((cbH - (classNr - 5)) + 6);

      int dDistanceBetweenClasses = (classSizeY - 10);
      if (dDistanceBetweenClasses < 4) {
        dDistanceBetweenClasses = 2;
      };
      if (dDistanceBetweenClasses > 4) dDistanceBetweenClasses = 4;
      boxLowerY -= dDistanceBetweenClasses;
      int boxUpperY = int((cbH - (classNr + classSizeY - 5)) + 6);
      classNr += classSizeY;

      if (j < iMax) {
        int colorIndex = CImageDataWriter::getColorIndexForValue(dataSource, v);
        if (classSizeY > 4) {
          legendImage->rectangle(pLeft + 4 * scaling, pTop + boxUpperY, pLeft + (int(cbW) + 7) * scaling, pTop + boxLowerY, (colorIndex), 248);
        } else {
          legendImage->rectangle(pLeft + 4 * scaling, pTop + boxUpperY, pLeft + (int(cbW) + 7) * scaling, pTop + boxLowerY, (colorIndex), (colorIndex));
        }
        if (textformatting.empty() == false) {
          CT::string textFormat;
          textFormat.print("%s - %s", textformatting.c_str(), textformatting.c_str());
          snprintf(szTemp, szTempLength, textFormat.c_str(), v, v + legendInterval);
        } else {
          if (textRounding <= 0) snprintf(szTemp, szTempLength, "%2.0f - %2.0f", v, v + legendInterval);
          if (textRounding == 1) snprintf(szTemp, szTempLength, "%2.1f - %2.1f", v, v + legendInterval);
          if (textRounding == 2) snprintf(szTemp, szTempLength, "%2.2f - %2.2f", v, v + legendInterval);
          if (textRounding == 3) snprintf(szTemp, szTempLength, "%2.3f - %2.3f", v, v + legendInterval);
          if (textRounding == 4) snprintf(szTemp, szTempLength, "%2.4f - %2.4f", v, v + legendInterval);
          if (textRounding == 5) snprintf(szTemp, szTempLength, "%2.5f - %2.5f", v, v + legendInterval);
          if (textRounding == 5) snprintf(szTemp, szTempLength, "%2.6f - %2.6f", v, v + legendInterval);
          if (textRounding > 6) snprintf(szTemp, szTempLength, "%f - %f", v, v + legendInterval);
        }
        legendImage->drawText(((int)cbW + 10 + pLeft) * scaling, (((boxLowerY)) + pTop) - fontSize * scaling / 4 + 1, fontLocation.c_str(), fontSize * scaling, 0, szTemp, 248);
      }
    }
  }

#ifdef CIMAGEDATAWRITER_DEBUG

  CDBDebug("set units");
#endif
  // Get units
  CT::string units;
  if (dataSource->getDataObject(0)->getUnits().length() > 0) {
    units.concat(dataSource->getDataObject(0)->getUnits().c_str());
  }
  if (units.length() > 0) legendImage->drawText((2 + pLeft) * scaling, int(legendHeight) - pTop - scaling * 2, fontLocation.c_str(), fontSize * scaling, 0, units.c_str(), 248);
  // legendImage->crop(4,4);
  return 0;
}