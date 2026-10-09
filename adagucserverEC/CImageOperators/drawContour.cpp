
#include "drawContour.h"
#include <CDrawImage.h>
#include <set>
#include "CDebugger.h"
#include "CStopWatch.h"
#include "CTString.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>

static const bool CImgWarpBilinear_DEBUG = false;

#define CONTOURDEFINITIONLOOKUPLENGTH 32
#define DISTANCEFIELDTYPE unsigned int

/*
Search window for xdir and ydir:
      -1  0  1  (x)
  -1   6  5  4
   0   7  X  3
   1   0  1  2
  (y)
            0  1  2  3  4  5  6  7 */
const int xdir[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
const int ydir[8] = {1, 1, 1, 0, -1, -1, -1, 0};
/*                0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 */
const int xdirOuter[16] = {-2, -1, 0, 1, 2, 2, 2, 2, 2, 1, 0, -1, -2, -2, -2, -2};
const int ydirOuter[16] = {2, 2, 2, 2, 2, 1, 0, -1, -2, -2, -2, -2, -2, -1, 0, 1};

struct ContourLineStructure {
  std::vector<double> classes;
  double interval = 0;
  std::string textformatting = "%0.1f";
  CColor lineColor = CColor(0, 0, 0, 255);
  CColor textColor = CColor(0, 0, 0, 255);
  CColor textstrokecolor = CColor(0, 0, 0, 0);
  double lineWidth = 4;
  double fontSize = 10;
  double textStrokeWidth = 0.75;
  std::vector<double> dashes;
};

// Statistics per contour definition, reported with StopWatch_Measure
struct ContourLineStats {
  size_t numLines = 0;
  size_t numLineSegments = 0;
  size_t numTexts = 0;
  double traceMs = 0; // Time spent following the lines in the distance field
  double drawMs = 0;  // Time spent drawing the line segments and texts
  double textMs = 0;  // Part of drawMs spent on drawing texts
};

// Fast rounding to the nearest integer (ties to even) without a library call: adding and subtracting 1.5 * 2^23 drops the fraction.
// This is exact for |v| < 2^22, larger values fall back to nearbyint.
static inline float fastRoundf(float v) {
  constexpr float magic = 12582912.0f;
  if (std::fabs(v) < 4194304.0f) return (v + magic) - magic;
  return std::nearbyint(v);
}

static double msSince(const std::chrono::steady_clock::time_point &start) { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count(); }

bool IsTextTooClose(std::vector<i4point> &textLocations, int x, int y) {
  for (auto &textLocation: textLocations) {
    int dx = x - textLocation.x;
    int dy = y - textLocation.y;
    if ((dx * dx + dy * dy) < 10 * 10) {
      return true;
    }
  }
  return false;
}

void drawTextForContourLines(CDrawImage *drawImage, ContourLineStructure &contourDefinition, int lineX, int lineY, int endX, int endY, float value, const char *fontLocation, float scaling) {

  /* Draw text */
  std::string text;
  text = contourDefinition.textformatting.empty() ? CT::printf("%g", value) : CT::printf(contourDefinition.textformatting.c_str(), value);
  float fontSize = contourDefinition.fontSize * scaling;
  float textStrokeWidth = contourDefinition.textStrokeWidth * scaling;
  double angle = atan2(lineX - endX, lineY - endY) - M_PI / 2;
  double angleP = atan2(endY - lineY, endX - lineX) + M_PI / 2;

  if (angle < -M_PI / 2 || angle > M_PI / 2) {
    int x = lineX + cos(angleP) * (fontSize / 2);
    int y = lineY + sin(angleP) * (fontSize / 2);
    drawImage->setTextStroke(x, y, -angleP + M_PI / 2, text.c_str(), fontLocation, fontSize, textStrokeWidth, contourDefinition.textstrokecolor, contourDefinition.textColor);
  } else {
    int x = endX - cos(angleP) * (fontSize / 2);
    int y = endY - sin(angleP) * (fontSize / 2);
    drawImage->setTextStroke(x, y, angle, text.c_str(), fontLocation, fontSize, textStrokeWidth, contourDefinition.textstrokecolor, contourDefinition.textColor);
  }
}

void traverseLine(CDrawImage *drawImage, DISTANCEFIELDTYPE *distance, float *valueField, int lineX, int lineY, int dImageWidth, int dImageHeight, ContourLineStructure &contourDefinition,
                  DISTANCEFIELDTYPE lineMask, std::vector<i4point> &textLocations, double scaling, const char *fontLocation, ContourLineStats &stats) {
  auto traceStart = std::chrono::steady_clock::now();
  size_t p = lineX + lineY * dImageWidth; /* Starting pointer */
  bool foundLine = true;                  /* This function starts at the beginning of a line segment */
  int maxLineDistance = 5;                /* Maximum length of each line segment */
  int currentLineDistance = maxLineDistance;
  std::vector<i4point> lineSegments;
  lineSegments.reserve(500);

  size_t numDashes = contourDefinition.dashes.size();
  double *dashes = &contourDefinition.dashes[0];

  /* Push the beginning of this line*/
  lineSegments.push_back({.x = lineX, .y = lineY});

  double lineSegmentsValue = valueField[p];
  double binnedLineSegmentsValue;

  if (!contourDefinition.classes.empty()) {
    float closestValue;
    int definedIntervalIndex = 0;
    int j = 0;
    for (auto c: contourDefinition.classes) {
      float d = fabs(lineSegmentsValue - c);
      if (j == 0)
        closestValue = d;
      else {
        if (d < closestValue) {
          closestValue = d;
          definedIntervalIndex = j;
        }
      }
      j++;
    }
    binnedLineSegmentsValue = contourDefinition.classes[definedIntervalIndex];
  } else {
    double interval = contourDefinition.interval;
    binnedLineSegmentsValue = round(lineSegmentsValue / interval) * interval;
    // Avoid printing -0;
    if (binnedLineSegmentsValue > -interval / 2 && binnedLineSegmentsValue < interval / 2) {
      binnedLineSegmentsValue = 0;
    }
  }

  /* Use the distance field and walk the line */
  while (foundLine) {
    distance[p] &= ~lineMask; /* Indicate found, set to false */
    /* Search around using the small search window and find the continuation of this line */
    foundLine = false;
    int nextLineX = lineX;
    int nextLineY = lineY;
    for (int j = 0; j < 8; j++) {
      int tx = lineX + xdir[j];
      int ty = lineY + ydir[j];
      if (tx >= 0 && tx < dImageWidth && ty >= 0 && ty < dImageHeight) {
        p = tx + ty * dImageWidth;
        if (distance[p] & lineMask && !foundLine) {
          nextLineX = tx;
          nextLineY = ty;
          foundLine = true;
        }
        distance[p] = 0; //&= ~lineMask; /* Indicate found, set to false */
      }
    }

    /* Search line with outer window. */
    if (!foundLine) {
      // Try to find the line with an outer window...
      for (int j = 0; j < 16; j++) {
        int tx = lineX + xdirOuter[j];
        int ty = lineY + ydirOuter[j];
        if (tx >= 0 && tx < dImageWidth && ty >= 0 && ty < dImageHeight) {
          p = tx + ty * dImageWidth;
          if (distance[p] & lineMask && !foundLine) {
            nextLineX = tx;
            nextLineY = ty;
            foundLine = true;
          }
          distance[p] = 0; //~lineMask; /* Indicate found, set to false */
        }
      }
    }
    if (!foundLine) {
      if (lineSegments[0].distance(lineSegments.back()) < 8) {
        lineSegments.push_back({.x = lineSegments[0].x, .y = lineSegments[0].y});
      }
    }
    lineX = nextLineX;
    lineY = nextLineY;
    /* Decrease the max currentLineDist counter,
       when zero, the max line distance is reached
       and we should add a line segment */
    currentLineDistance--;
    if (currentLineDistance <= 0 || foundLine == false) {
      currentLineDistance = maxLineDistance;

      lineSegments.push_back({.x = lineX, .y = lineY});
    }
  }

  stats.traceMs += msSince(traceStart);
  stats.numLines++;
  stats.numLineSegments += lineSegments.size();
  auto drawStart = std::chrono::steady_clock::now();

  /* Now draw this line */
  drawImage->moveTo(lineSegments[0].x, lineSegments[0].y);

  bool doDrawText = !contourDefinition.textformatting.empty();

  bool textSkip = false;
  bool textOn = false;

  int drawTextAtEveryNPixels = 60 * int(scaling);
  int startAtStep = 0; // drawTextAtEveryNPixels / 2;
  int spaceForTextNr = 5 * int(scaling) * (contourDefinition.fontSize / 8);

  float scaledLineWidth = contourDefinition.lineWidth * scaling;
  int numLineSegments = (int)lineSegments.size();
  if (doDrawText && numLineSegments > 20) {
    for (int j = 0; j < numLineSegments; j++) {
      auto &lineSegment = lineSegments[j];
      int matchModulo = j % drawTextAtEveryNPixels;
      bool startText = matchModulo == startAtStep;
      if (startText && j + spaceForTextNr < numLineSegments) {
        textOn = false;
        if (IsTextTooClose(textLocations, lineSegment.x, lineSegment.y) == false && j + spaceForTextNr < numLineSegments) {
          textSkip = false;
          textLocations.push_back(lineSegment);

          int endX = lineSegments[j + spaceForTextNr].x;
          int endY = lineSegments[j + spaceForTextNr].y;
          auto textStart = std::chrono::steady_clock::now();
          drawTextForContourLines(drawImage, contourDefinition, lineSegment.x, lineSegment.y, endX, endY, binnedLineSegmentsValue, fontLocation, scaling);
          stats.textMs += msSince(textStart);
          stats.numTexts++;
          textOn = true;
        } else {
          textSkip = true;
        }
      }
      if (startText && textOn && !textSkip) {
        drawImage->endLine(dashes, numDashes);
      }
      if (matchModulo > spaceForTextNr || textSkip) {
        drawImage->lineTo(lineSegment.x, lineSegment.y, scaledLineWidth, contourDefinition.lineColor);
      }
    }

  } else {
    for (auto &lineSegment: lineSegments) {
      drawImage->lineTo(lineSegment.x, lineSegment.y, scaledLineWidth, contourDefinition.lineColor);
    }
  }

  drawImage->endLine(dashes, numDashes);
  stats.drawMs += msSince(drawStart);
}

void drawContour(float *sourceGrid, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration) {

  if (styleConfiguration->contourLines.size() == 0) {
    return;
  }
  StopWatch_Measure("[drawContour] %d contour definitions", (int)styleConfiguration->contourLines.size());

  double scaling = dataSource->getContourScaling();
  const char *fontLocation = dataSource->srvParams->cfg->WMS[0].ContourFont[0].attr.location.c_str();

  double defaultLineWidth = 4;
  double defaultStrokeWidth = 0.75;
  double defaultFontSize = atof(dataSource->srvParams->cfg->WMS[0].ContourFont[0].attr.size.c_str());
  CColor defaultLineColor = CColor(0, 0, 0, 255);
  CColor defaultTextColor = CColor(0, 0, 0, 255);
  CColor defaultTextStrokeColor = CColor(0, 0, 0, 0);

  int dImageWidth = drawImage->geoParams.width;
  int dImageHeight = drawImage->geoParams.height;
  size_t imageSize = (dImageHeight + 0) * (dImageWidth + 1);

  // Create a distance field, this is where the line information will be put in.
  // calloc gives zeroed memory, the OS can provide this lazily which is much faster than memset for large images.
  DISTANCEFIELDTYPE *distance = (DISTANCEFIELDTYPE *)calloc(imageSize, sizeof(DISTANCEFIELDTYPE));
  StopWatch_Measure("drawContour: allocated distance field %dx%d", dImageWidth, dImageHeight);

  std::vector<ContourLineStructure> contourlineList;

  for (const auto &contourLine: (styleConfiguration->contourLines)) {
    auto &attr = contourLine.attr;
    std::vector<double> classes;
    double interval = 0;
    if (attr.classes.empty() == false) {
      auto classesString = CT::split(attr.classes, ",");
      for (const auto &classString: classesString) {
        classes.push_back(atof(classString.c_str()));
      }
    } else if (attr.interval.empty() == false) {
      interval = atof(attr.interval.c_str());
    }
    std::vector<double> dashes;
    auto dashesStrings = CT::split(attr.dashing, ",");
    for (const auto &dashString: dashesStrings) {
      dashes.push_back(atof(dashString.c_str()));
    }

    // Check for lines at specified classes

    contourlineList.push_back({.classes = classes,
                               .interval = interval,
                               .textformatting = attr.textformatting,
                               .lineColor = attr.linecolor.empty() ? defaultLineColor : CColor(attr.linecolor),
                               .textColor = attr.textcolor.empty() ? defaultTextColor : CColor(attr.textcolor),
                               .textstrokecolor = attr.textstrokecolor.empty() ? defaultTextStrokeColor : CColor(attr.textstrokecolor),
                               .lineWidth = attr.width.empty() ? defaultLineWidth : atof(attr.width.c_str()),
                               .fontSize = attr.textsize.empty() ? defaultFontSize : atof(attr.textsize.c_str()),
                               .textStrokeWidth = attr.textstrokewidth.empty() ? defaultStrokeWidth : atof(attr.textstrokewidth.c_str()),
                               .dashes = dashes});
  }

  StopWatch_Measure("drawContour: contour definitions parsed, start filling distance field");
  // Sorted copies of the classes, so that a binary search can find if a class lies within [min, max) of a pixel.
  // The order does not matter for that check. The original order is kept in contourlineList, it is used to pick the closest class for the text.
  std::vector<std::vector<double>> sortedClassesList;
  // Interval and its inverse in float, so that the interval check in the loop below needs no division
  std::vector<float> intervals;
  std::vector<float> inverseIntervals;
  for (const auto &contourLine: contourlineList) {
    intervals.push_back(contourLine.interval);
    inverseIntervals.push_back(contourLine.interval > 0 ? 1.0f / float(contourLine.interval) : 0);
    std::vector<double> sortedClasses;
    for (double cc: contourLine.classes) {
      if (cc == cc) sortedClasses.push_back(cc); // NaN never matches, leave it out so sorting is well defined
    }
    std::sort(sortedClasses.begin(), sortedClasses.end());
    sortedClassesList.push_back(sortedClasses);
  }
  size_t numContourLines = contourlineList.size();

  float fNodataValue = dataSource->getDataObject(0)->dfNodataValue;
  // Pixels where at least one contour definition set a bit. The distance field is sparse, so tracing below only
  // needs to look at these candidates instead of rescanning the full image once per contour definition. No bit is
  // ever set after this point, only cleared, so this candidate set stays valid for every contour definition.
  std::vector<i4point> candidatePixels;
  // Fills the distance field for rows [rowStart, rowEnd).
  auto fillRows = [&](int rowStart, int rowEnd) {
    for (int y = rowStart; y < rowEnd; y++) {
      for (int x = 0; x < dImageWidth - 1; x++) {
        size_t p1 = size_t(x + y * dImageWidth);
        const float v0 = sourceGrid[p1], v1 = sourceGrid[p1 + 1], v2 = sourceGrid[p1 + dImageWidth], v3 = sourceGrid[p1 + dImageWidth + 1];
        // Check if all pixels have values...
        if (!(v0 != fNodataValue && v1 != fNodataValue && v2 != fNodataValue && v3 != fNodataValue && v0 == v0 && v1 == v1 && v2 == v2 && v3 == v3)) {
          continue;
        }
        float min = std::min(v0, std::min(v1, std::min(v2, v3)));
        float max = std::max(v0, std::max(v1, std::max(v2, v3)));
        // A line is drawn where a contour value lies within [min, max). This range is empty when all values are equal.
        if (min == max) {
          continue;
        }
        DISTANCEFIELDTYPE mask = 1;
        DISTANCEFIELDTYPE foundLines = 0;
        for (size_t j = 0; j < numContourLines; j++) {
          // Check for lines at specified classes
          const auto &sortedClasses = sortedClassesList[j];
          if (!sortedClasses.empty()) {
            auto firstClassAboveMin = std::lower_bound(sortedClasses.begin(), sortedClasses.end(), (double)min);
            if (firstClassAboveMin != sortedClasses.end() && *firstClassAboveMin < max) {
              foundLines |= mask;
            }
          }
          float interval = intervals[j];
          if (interval > 0) {
            // Check for lines at continous interval
            float cc = fastRoundf(min * inverseIntervals[j]) * interval;
            if (cc >= min && cc < max) {
              foundLines |= mask;
            }
          }
          mask = mask + mask;
        }
        // Only write when a line was found, untouched parts of the calloc-ed distance field then do not need to be mapped into memory
        if (foundLines) {
          distance[p1] |= foundLines;
          candidatePixels.push_back({.x = x, .y = y});
        }
      }
    }
  };

  int numRows = dImageHeight - 1;
  StopWatch_Measure("drawContour: filling distance field");
  fillRows(0, numRows);

  StopWatch_Measure("drawContour: done filling distance field");

  std::vector<i4point> textLocations;

  DISTANCEFIELDTYPE lineMask = 1;

  int contourLineIndex = 0;
  for (auto &contourLine: contourlineList) {
    StopWatch_Measure("drawContour: start contour definition %d", contourLineIndex);
    ContourLineStats stats;
    for (const auto &candidate: candidatePixels) {
      size_t p = candidate.x + candidate.y * dImageWidth;
      if (distance[p] & lineMask) {
        traverseLine(drawImage, distance, sourceGrid, candidate.x, candidate.y, dImageWidth, dImageHeight, contourLine, lineMask, textLocations, scaling, fontLocation, stats);
      }
    }
    StopWatch_Measure("drawContour: done contour definition %d: %zu lines, %zu line segments, %zu texts. Tracing %.1f ms, drawing %.1f ms (of which texts %.1f ms)", contourLineIndex, stats.numLines,
                      stats.numLineSegments, stats.numTexts, stats.traceMs, stats.drawMs, stats.textMs);
    lineMask = lineMask + lineMask;
    contourLineIndex++;
  }

  if (CImgWarpBilinear_DEBUG) {
    CDBDebug("Deleting distance[]");
  }

  free(distance);

  if (CImgWarpBilinear_DEBUG) {
    CDBDebug("Finished drawing lines and text");
  }
  StopWatch_Measure("[/drawContour]");
}