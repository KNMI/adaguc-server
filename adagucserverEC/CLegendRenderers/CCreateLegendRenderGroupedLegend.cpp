#include "CCreateLegend.h"
#include "CDataReader.h"
#include "CImageDataWriter.h"
#include "numericutils.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

int CCreateLegend::renderGroupedLegend(CDataSource *dataSource, CDrawImage *legendImage, CStyleConfiguration *styleConfiguration, bool, bool) {
#ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("legendtype grouped");
#endif

  const auto &intervals = styleConfiguration->shadeIntervals;
  if (intervals.empty()) {
    CDBError("renderGroupedLegend requires at least one ShadeInterval");
    return 1;
  }

  double scaling = dataSource->getScaling();
  float legendHeight = legendImage->geoParams.height;
  float cbH = legendHeight - 13 - 13 * scaling;
  float cbW = 16;
  float lineWidth = 0.8 * scaling;

  int pLeft = 4;
  int pTop = (int)(legendImage->geoParams.height - legendHeight);

  float fontSize;
  std::string fontLocation;
  std::tie(fontSize, fontLocation) = dataSource->srvParams->getLegendFont();

  CT::string textformatting;
  if (styleConfiguration != nullptr && !styleConfiguration->legend.attr.textformatting.empty()) {
    textformatting = styleConfiguration->legend.attr.textformatting;
  }

  size_t numIntervals = intervals.size();

  // A gap exists before and interval if it doesn't start exactly where the
  // previous one ends. Represented by a blank space (enough for the labels
  // not to overlap)
  std::vector<bool> gapBeforeInterval(numIntervals, false);
  size_t numGaps = 0;
  for (size_t i = 1; i < numIntervals; i++) {
    double prevMax = atof(intervals[i - 1].attr.max.c_str());
    double thisMin = atof(intervals[i].attr.min.c_str());
    if (std::abs(thisMin - prevMax) > 1e-9) {
      gapBeforeInterval[i] = true;
      numGaps++;
    }
  }

  float gapHeight = std::max(8.0f, fontSize * (float)scaling * 1.4f);
  float bandHeight = std::max(1.0f, (cbH - numGaps * gapHeight) / float(numIntervals));

  // Go from bottom (lowest value) upward
  std::vector<float> bandYBottom(numIntervals), bandYTop(numIntervals);
  float yCursor = cbH;
  for (size_t i = 0; i < numIntervals; i++) {
    if (i > 0 && gapBeforeInterval[i]) {
      yCursor -= gapHeight;
    }
    bandYBottom[i] = yCursor;
    yCursor -= bandHeight;
    bandYTop[i] = yCursor;
  }

  int barRight = pLeft + ((int)cbW + 1) * scaling;

  // Render each interval
  for (size_t i = 0; i < numIntervals; i++) {
    const auto &iv = intervals[i];
    double intervalMin = atof(iv.attr.min.c_str());
    double intervalMax = atof(iv.attr.max.c_str());

    int yBottom = (int)std::lround(bandYBottom[i]);
    int yTop = (int)std::lround(bandYTop[i]);
    if (yTop < 0) yTop = 0;

    for (int y = yTop; y < yBottom; y++) {
      // Sample at the pixel's vertical centre, strictly inside (min, max) -
      // see the note above about getPixelColorForValue's half-open ranges.
      float frac = (yBottom > yTop) ? (float(yBottom - y) - 0.5f) / float(yBottom - yTop) : 0.5f;
      double value = intervalMin + frac * (intervalMax - intervalMin);
      CColor rowColor = CImageDataWriter::getPixelColorForValue(dataSource, (float)value);
      legendImage->rectangle(pLeft, y + 7 + pTop, barRight, y + 8 + pTop, rowColor, rowColor);
    }
  }

  // Frame for the intervals
  for (size_t i = 0; i < numIntervals; i++) {
    int yBottom = (int)std::lround(bandYBottom[i]) + 7 + pTop;
    int yTop = (int)std::lround(bandYTop[i]) + 7 + pTop;
    legendImage->line(pLeft, yTop, pLeft, yBottom, lineWidth, CColor(0, 0, 0, 255));
    legendImage->line(barRight, yTop, barRight, yBottom, lineWidth, 248);
  }

  legendImage->line(pLeft, (int)std::lround(bandYBottom[0]) + 7 + pTop, barRight, (int)std::lround(bandYBottom[0]) + 7 + pTop, lineWidth, CColor(0, 0, 0, 255));
  legendImage->line(pLeft, (int)std::lround(bandYTop[numIntervals - 1]) + 7 + pTop, barRight, (int)std::lround(bandYTop[numIntervals - 1]) + 7 + pTop, lineWidth, CColor(0, 0, 0, 255));

  auto trimTrailingZeroDecimal = [](const std::string &raw) -> CT::string {
    std::string s = raw;
    size_t dot = s.find('.');
    if (dot != std::string::npos) {
      size_t lastNonZero = s.find_last_not_of('0');
      if (lastNonZero == dot) {
        s = s.substr(0, dot);
      } else if (lastNonZero > dot) {
        s = s.substr(0, lastNonZero + 1);
      }
    }
    return CT::string(s.c_str());
  };

  auto makeLabel = [&](const std::string &raw, double value) -> CT::string {
    if (!textformatting.empty()) {
      // Style explicitly asked for a specific numeric format - honor it.
      char temp[64];
      CT::string textFormat;
      textFormat.print("%s", textformatting.c_str());
      snprintf(temp, sizeof(temp), textFormat.c_str(), value);
      return CT::string(temp);
    }
    return trimTrailingZeroDecimal(raw);
  };

  // One label per tick/boundary
  struct TickEntry {
    float y;
    CT::string label;
  };
  std::vector<TickEntry> tickEntries;
  tickEntries.reserve(numIntervals + 1 + numGaps);
  for (size_t i = 0; i < numIntervals; i++) {
    double intervalMin = atof(intervals[i].attr.min.c_str());
    double intervalMax = atof(intervals[i].attr.max.c_str());
    if (i == 0 || gapBeforeInterval[i]) {
      tickEntries.push_back({bandYBottom[i], makeLabel(intervals[i].attr.min, intervalMin)});
    }
    tickEntries.push_back({bandYTop[i], makeLabel(intervals[i].attr.max, intervalMax)});
  }

  std::vector<CT::string> allLabelStrings;
  allLabelStrings.reserve(tickEntries.size());
  for (auto &e: tickEntries) allLabelStrings.push_back(e.label);

  int numberWidth = legendImage->getTextWidth("0", fontLocation, fontSize, 0);
  int minusWidth = legendImage->getTextWidth("-", fontLocation, fontSize, 0);
  int intWidth = maxIntWidth(allLabelStrings);
  int colRight = ((int)cbW + pLeft) * scaling + intWidth * numberWidth;
  int columnCenter = colRight - numberWidth;

  for (auto &e: tickEntries) {
    int y = (int)std::lround(e.y) + 6 + pTop;
    legendImage->line(((int)cbW - 1) * scaling + pLeft, y, ((int)cbW + 6) * scaling + pLeft, y, lineWidth, 248);

    if (!fontLocation.empty()) {
      char tempText[64];
      snprintf(tempText, sizeof(tempText), "%s", e.label.c_str());
      const char *dotPos = strchr(tempText, '.');
      int leftChars = dotPos ? (dotPos - tempText) : strlen(tempText);
      int textX = columnCenter - (leftChars * numberWidth) + ((int)cbW) * scaling + pLeft;
      if (tempText[0] == '-') textX -= (minusWidth - numberWidth);
      legendImage->drawText(textX, y + 4, fontLocation.c_str(), fontSize * scaling, 0, tempText, 248);
    }
  }

  // Render the units
  CT::string units;
  if (dObjgetUnits(*dataSource->getDataObject(0)).length() > 0) {
    units.concat(dObjgetUnits(*dataSource->getDataObject(0)).c_str());
  }
  if (units.length() == 0) units = "-";
  if (!fontLocation.empty()) {
    legendImage->drawText((2 + pLeft) * scaling, int(legendHeight) - pTop - scaling * 2, fontLocation.c_str(), fontSize * scaling, 0, units.c_str(), 248);
  }

  return 0;
}