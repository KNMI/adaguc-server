#include "CCreateLegend.h"
#include "CDataReader.h"
#include "CImageDataWriter.h"
#include "numericutils.h"

#define MIN_SHADE_CLASS_BLOCK_SIZE 3
#define MAX_SHADE_CLASS_BLOCK_SIZE 12

// Aux function to plot numberic labels, optionally as two columns (representing an interval)
void plotNumericLabels(CDrawImage *legendImage, double scaling, std::string fontLocation, float fontSize, int angle, CServerConfig::XMLE_ShadeInterval *s, int cbW, int pLeft, int textY,
                       const std::vector<CT::string> &minColumn, const std::vector<CT::string> &maxColumn, int maxTextWidth) {

  // With a monospaced font, this will be the spacing for every character, numeric or not
  int numberWidth = legendImage->getTextWidth("0", fontLocation.c_str(), fontSize * scaling, angle);
  int minusWidth = legendImage->getTextWidth("-", fontLocation.c_str(), fontSize * scaling, angle);

  // Right edge of the min column
  int colRightMin = ((int)cbW + pLeft) * scaling + maxIntWidth(minColumn) * numberWidth;
  // int columnCenterMin = colRightMin - maxDecimalWidth(minColumn) * numberWidth;
  int columnCenterMin = colRightMin - numberWidth; // - maxDecimalWidth(minColumn) * numberWidth;

  // Draw min, dot-aligned
  float numericMinVal = atof(s->attr.min.c_str());

  // Calculate number of decimals for min column
  CT::string floatFormatMin;
  floatFormatMin.print("%%.%df", maxDecimalWidth(minColumn));

  CT::string tempText;
  tempText.print(floatFormatMin.c_str(), numericMinVal);
  const char *dotPos = strchr(tempText, '.');
  int leftCharsMin = dotPos ? (dotPos - tempText) : strlen(tempText); // chars before dot

  // Positioning (leaving space for the class rectangle)
  int textXMin = columnCenterMin - (leftCharsMin * numberWidth);
  textXMin = textXMin + (int(cbW)) * scaling + pLeft;
  if (numericMinVal < 0) {
    textXMin -= minusWidth - numberWidth;
  }

  legendImage->drawText(textXMin, textY, fontLocation.c_str(), fontSize * scaling, angle, tempText, 248);

  // If no maxColumn, stop
  if (maxColumn.empty()) {
    return;
  }

  // Draw central dash
  int dashX = colRightMin + (maxDecimalWidth(minColumn) + 4) * numberWidth; // Leave gap between min column and this dash
  legendImage->drawText(dashX, textY, fontLocation.c_str(), fontSize * scaling, angle, "–", 248);

  // Draw max column (to the right of the dash)
  int colRightMax = colRightMin + maxTextWidth;
  // Calculate column center for max value
  int columnCenterMax = colRightMax + numberWidth;

  // Draw max, dot-aligned
  float numericMaxVal = atof(s->attr.max.c_str());
  CT::string floatFormatMax;
  floatFormatMax.print("%%.%df", maxDecimalWidth(maxColumn));
  tempText.print(floatFormatMax.c_str(), numericMaxVal);

  const char *dotPosMax = strchr(tempText, '.');
  int leftCharsMax = dotPosMax ? (dotPosMax - tempText) : strlen(tempText);

  // Align max string so that the dot falls on the column center
  int textXMax = columnCenterMax - (leftCharsMax * numberWidth);
  if (numericMaxVal < 0) {
    textXMax -= minusWidth - numberWidth;
  }
  // Apply overall left offset plus some spacing
  textXMax += ((int)cbW + pLeft) * scaling + (maxDecimalWidth(maxColumn) + 1) * numberWidth; // Think of the 15 number

  legendImage->drawText(textXMax, textY, fontLocation.c_str(), fontSize * scaling, angle, tempText, 248);
}

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

  float fontSize;
  std::string fontLocation;
  std::tie(fontSize, fontLocation) = dataSource->srvParams->getLegendFont();

  legendImage->setTTFFontLocation(fontLocation.c_str());
  legendImage->setTTFFontSize(fontSize);

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

    // Calculate columns and text properties
    std::vector<CT::string> minColumn = extractColumn(drawIntervals, minInterval, styleConfiguration->shadeIntervals, true);
    std::vector<CT::string> maxColumn = extractColumn(drawIntervals, minInterval, styleConfiguration->shadeIntervals, false);
    int dashWidth = legendImage->getTextWidth("-", fontLocation.c_str(), fontSize * scaling, angle);
    int dotWidth = legendImage->getTextWidth(".", fontLocation.c_str(), fontSize * scaling, angle);
    // Assume monospaced for numbers
    int numberWidth = legendImage->getTextWidth("0", fontLocation.c_str(), fontSize * scaling, angle);

    // With a small blockHeight, use the compact legend for better visibility
    if (initialBlockHeight <= MIN_SHADE_CLASS_BLOCK_SIZE + 1) {
      // For right alignment of labels
      int maxTextWidth = 0;

      for (size_t j = 0; j < drawIntervals; j++) {
        size_t realj = minInterval + j;
        CServerConfig::XMLE_ShadeInterval *s = (*styleConfiguration->shadeIntervals)[realj];
        if (!s->attr.min.empty() && !s->attr.max.empty()) {
          if ((int)(std::abs(parseFloat(s->attr.min.c_str()))) % 5 != 0) {
            continue;
          }
          int textWidth = legendImage->getTextWidth(s->attr.min.c_str(), fontLocation.c_str(), fontSize * scaling, angle);
          if (textWidth > maxTextWidth) maxTextWidth = textWidth;
        }
      }
      // Recalculate block size based on the final intervals to appear on the legend
      float blockHeight = calculateShadeClassBlockHeight(legendImage->Geo->dHeight, drawIntervals - 1);

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

          int textY = (cY1 + pTop) - ((fontSize * scaling) / 4) + 3;
          // We only print min column values, because it would make no sense to print intervals when not every interval is printed.
          plotNumericLabels(legendImage, scaling, fontLocation, fontSize, angle, s, cbW, pLeft, textY, minColumn, {}, 0);
        }
      }
    } else {
      // General case for this type of legend, where we draw every label and print every class interval
      // We can also clip this type of legend
      float blockHeight = calculateShadeClassBlockHeight(legendImage->Geo->dHeight, drawIntervals);
      int maxTextWidthMax = fieldWidthAsPixels(maxColumn, dashWidth, dotWidth, numberWidth);

      for (size_t j = 0; j < drawIntervals; j++) {
        size_t realj = minInterval + j;
        CServerConfig::XMLE_ShadeInterval *s = (*styleConfiguration->shadeIntervals)[realj];
        if (s->attr.min.empty() || s->attr.max.empty()) {
          continue;
        }

        int cY1 = int(cbH - ((j)*blockHeight) * scaling);
        int cY2 = int(cbH - ((((j + 1) * blockHeight) - 2)) * scaling);
        CColor color;
        if (s->attr.fillcolor.empty() == false) {
          color = CColor(s->attr.fillcolor.c_str());
        } else {
          color = legendImage->getColorForIndex(CImageDataWriter::getColorIndexForValue(dataSource, parseFloat(s->attr.min.c_str())));
        }
        legendImage->rectangle(4 * scaling + pLeft, cY2 + pTop, (int(cbW) + 7) * scaling + pLeft, cY1 + pTop, color, CColor(0, 0, 0, 255));

        if (s->attr.label.empty()) {
          int textY = (cY1 + pTop) - ((fontSize * scaling) / 4) + 3;
          plotNumericLabels(legendImage, scaling, fontLocation, fontSize, angle, s, cbW, pLeft, textY, minColumn, maxColumn, maxTextWidthMax);

        } else {
          // Do not align to the right: this is a non-numeric label
          legendImage->drawText(((int)cbW + 12 + pLeft) * scaling, (cY1 + pTop) - ((fontSize * scaling) / 4) + 3, fontLocation.c_str(), fontSize * scaling, 0, s->attr.label.c_str(), 248);
        }
      }
    }
  }

  if (definedLegendForFeatures) {

    for (size_t j = 0; j < styleConfiguration->featureIntervals->size(); j++) {
      CServerConfig::XMLE_FeatureInterval *s = (*styleConfiguration->featureIntervals)[j];
      //         if(s->attr.min.empty()==false&&s->attr.max.empty()==false){
      int cY1 = int(cbH - (j * 12));
      int cY2 = int(cbH - (((j + 1) * 12) - 2));
      CColor color;

      color = CColor(s->attr.fillcolor.c_str());

      legendImage->rectangle(4 + pLeft, cY2 + pTop, int(cbW) + 7 + pLeft, cY1 + pTop, color, CColor(0, 0, 0, 255));
      legendImage->setText(s->attr.label.c_str(), s->attr.label.length(), int(cbW) + 12 + pLeft, cY2 + pTop, 248, -1);
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

    int numberWidth = legendImage->getTextWidth("0", fontLocation.c_str(), fontSize * scaling, 0);
    int maxWidthMin = 0;
    int maxWidthMax = 0;

    // Precalculate the formatting to be used
    std::string floatFormat;
    if (textformatting.empty() == false) {
      // Default case where a specific formatting is supplied
      floatFormat = textformatting.c_str();
    } else {
      if (textRounding > 6) {
        floatFormat = "%f";
      } else {
        floatFormat = "%." + std::to_string(textRounding) + "f";
      }
    }

    CT::string minText, maxText;
    for (float j = iMin; j < iMax + legendInterval; j = j + legendInterval) {
      // Look for the max width for the min value in this interval
      minText.print(floatFormat.c_str(), j);
      int widthMin = legendImage->getTextWidth(minText, fontLocation.c_str(), fontSize * scaling, 0);
      if (widthMin > maxWidthMin) maxWidthMin = widthMin;

      // Look for the max width for the max value in this interval
      maxText.print(floatFormat.c_str(), j + legendInterval);
      int widthMax = legendImage->getTextWidth(maxText, fontLocation.c_str(), fontSize * scaling, 0);
      if (widthMax > maxWidthMax) maxWidthMax = widthMax;
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
        // Prepare alignment
        CT::string minText, maxText;
        minText.print(floatFormat.c_str(), v);
        maxText.print(floatFormat.c_str(), v + legendInterval);
        int currentWidthMin = legendImage->getTextWidth(minText, fontLocation.c_str(), fontSize * scaling, 0);
        int currentWidthMax = legendImage->getTextWidth(maxText, fontLocation.c_str(), fontSize * scaling, 0);

        int textY = (((boxLowerY)) + pTop) - fontSize * scaling / 4 + 1;
        int colGap = 3 * numberWidth;

        int columnXMin = ((int)cbW + 10 + pLeft) * scaling;
        int columnXMax = columnXMin + maxWidthMin + colGap;

        int textXMin = columnXMin + (maxWidthMin - currentWidthMin);
        int textXMax = columnXMax + (maxWidthMax - currentWidthMax);

        // Draw as 3 columns (min dash max)
        legendImage->drawText(textXMin, textY, fontLocation.c_str(), fontSize * scaling, 0, minText, 248);
        legendImage->drawText(textXMax - 2 * numberWidth, textY, fontLocation.c_str(), fontSize * scaling, 0, "–", 248);
        legendImage->drawText(textXMax, textY, fontLocation.c_str(), fontSize * scaling, 0, maxText, 248);
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