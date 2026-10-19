
#include "drawContour.h"
#include <CDrawImage.h>
#include <set>
#include "CDebugger.h"
#include "CStopWatch.h"
#include "CTString.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <unordered_map>

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

// Spatial hash of placed-label proximity points, so gap lookups don't linear-scan every label.
class TextLocationIndex {
public:
  void add(int x, int y) { cells[cellKeyFor(x, y)].push_back({.x = x, .y = y}); }

  bool isTooClose(int x, int y) const {
    int cx = cellCoord(x);
    int cy = cellCoord(y);
    for (int ny = cy - 1; ny <= cy + 1; ny++) {
      for (int nx = cx - 1; nx <= cx + 1; nx++) {
        auto it = cells.find(cellKey(nx, ny));
        if (it == cells.end()) continue;
        for (const auto &point: it->second) {
          int dx = x - point.x;
          int dy = y - point.y;
          if ((dx * dx + dy * dy) < kRadius * kRadius) return true;
        }
      }
    }
    return false;
  }

private:
  static constexpr int kCellSize = 10;
  static constexpr int kRadius = 10;
  static int cellCoord(int v) { return v >= 0 ? v / kCellSize : (v - kCellSize + 1) / kCellSize; }
  static int64_t cellKey(int cx, int cy) { return (int64_t(cx) << 32) | uint32_t(cy); }
  int64_t cellKeyFor(int x, int y) const { return cellKey(cellCoord(x), cellCoord(y)); }

  std::unordered_map<int64_t, std::vector<i4point>> cells;
};

void drawTextForContourLines(CDrawImage *drawImage, ContourLineStructure &contourDefinition, int lineX, int lineY, int endX, int endY, float value, const char *fontLocation, float scaling) {

  /* Draw text */
  std::string text;
  text = contourDefinition.textformatting.empty() ? CT::printf("%g", value) : CT::printf(contourDefinition.textformatting.c_str(), value);
  float fontSize = contourDefinition.fontSize * scaling;
  float textStrokeWidth = contourDefinition.textStrokeWidth * scaling;
  double angle = atan2(lineX - endX, lineY - endY) - M_PI / 2;
  double angleP = atan2(endY - lineY, endX - lineX) + M_PI / 2;

  // Center the text within the reserved gap, without overflowing it.
  double gapDx = endX - lineX;
  double gapDy = endY - lineY;
  double gapLength = sqrt(gapDx * gapDx + gapDy * gapDy);
  int textWidth = drawImage->getTextWidth(text, fontLocation, fontSize, 0);
  double centerOffset = std::max(0.0, (gapLength - textWidth) / 2.0);

  if (angle < -M_PI / 2 || angle > M_PI / 2) {
    // Text runs forward from lineX,lineY towards endX,endY.
    int x = lineX + sin(angleP) * centerOffset + cos(angleP) * (fontSize / 2);
    int y = lineY - cos(angleP) * centerOffset + sin(angleP) * (fontSize / 2);
    drawImage->setTextStroke(x, y, -angleP + M_PI / 2, text.c_str(), fontLocation, fontSize, textStrokeWidth, contourDefinition.textstrokecolor, contourDefinition.textColor);
  } else {
    // Text runs forward from endX,endY towards lineX,lineY.
    int x = endX - sin(angleP) * centerOffset - cos(angleP) * (fontSize / 2);
    int y = endY + cos(angleP) * centerOffset - sin(angleP) * (fontSize / 2);
    drawImage->setTextStroke(x, y, angle, text.c_str(), fontLocation, fontSize, textStrokeWidth, contourDefinition.textstrokecolor, contourDefinition.textColor);
  }
}

void traverseLine(CDrawImage *drawImage, DISTANCEFIELDTYPE *distance, float *valueField, int lineX, int lineY, int dImageWidth, int dImageHeight, ContourLineStructure &contourDefinition,
                  DISTANCEFIELDTYPE lineMask, TextLocationIndex &textLocations, double scaling, const char *fontLocation) {
  size_t p = lineX + lineY * dImageWidth; /* Starting pointer */
  if (!(distance[p] & lineMask)) {
    return; // Already traced while following another line that passed through this pixel.
  }

  bool foundLine = true;   /* This iteration starts at the beginning of a line segment */
  int maxLineDistance = 5; /* Maximum length of each line segment */
  int currentLineDistance = maxLineDistance;
  std::vector<i4point> lineSegments;
  lineSegments.reserve(500);

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
    // Recomputed rather than reusing the inner loops' stale p, which pointed at the wrong pixel.
    p = lineX + lineY * dImageWidth;
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
        if (distance[p] & lineMask) {
          if (!foundLine) {
            nextLineX = tx;
            nextLineY = ty;
            foundLine = true;
          }
          // Clear only this definition's bit, not the whole word, so neighboring contour levels' bits survive.
          distance[p] &= ~lineMask;
        }
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
          if (distance[p] & lineMask) {
            if (!foundLine) {
              nextLineX = tx;
              nextLineY = ty;
              foundLine = true;
            }
            distance[p] &= ~lineMask;
          }
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

  /* Now draw this line */
  drawImage->moveTo(lineSegments[0].x, lineSegments[0].y);

  bool doDrawText = !contourDefinition.textformatting.empty();

  bool textSkip = false;

  int drawTextAtEveryNPixels = 60 * int(scaling);
  int startAtStep = 0; // drawTextAtEveryNPixels / 2;
  int spaceForTextNr = 5 * int(scaling) * (contourDefinition.fontSize / 8);

  float scaledLineWidth = contourDefinition.lineWidth * scaling;
  int numLineSegments = (int)lineSegments.size();

  size_t numDashes = contourDefinition.dashes.size();
  double *dashes = numDashes > 0 ? &contourDefinition.dashes[0] : nullptr;

  // textLocations is shared across contour definitions, so a line gaps around any label it
  // crosses, not just its own.
  bool inTextGap = false;
  auto drawOrGap = [&](const i4point &point, bool gap) {
    if (gap) {
      if (!inTextGap) {
        // Flush so the next lineTo(), once the gap ends, starts a fresh sub-path.
        drawImage->endLine(dashes, numDashes);
        inTextGap = true;
      }
    } else {
      inTextGap = false;
      drawImage->lineTo(point.x, point.y, scaledLineWidth, contourDefinition.lineColor);
    }
  };

  if (doDrawText && numLineSegments > 20) {
    for (int j = 0; j < numLineSegments; j++) {
      auto &lineSegment = lineSegments[j];
      int matchModulo = j % drawTextAtEveryNPixels;
      bool startText = matchModulo == startAtStep;
      if (startText && j + spaceForTextNr < numLineSegments) {
        if (textLocations.isTooClose(lineSegment.x, lineSegment.y) == false) {
          textSkip = false;

          int endX = lineSegments[j + spaceForTextNr].x;
          int endY = lineSegments[j + spaceForTextNr].y;
          // Sample a few points along the label's span, not just its start, so its far end is protected too.
          for (int s = 0; s <= 3; s++) {
            textLocations.add(lineSegment.x + (endX - lineSegment.x) * s / 3, lineSegment.y + (endY - lineSegment.y) * s / 3);
          }

          drawTextForContourLines(drawImage, contourDefinition, lineSegment.x, lineSegment.y, endX, endY, binnedLineSegmentsValue, fontLocation, scaling);
        } else {
          textSkip = true;
        }
      }
      bool ownTextGap = matchModulo <= spaceForTextNr && !textSkip;
      drawOrGap(lineSegment, ownTextGap || textLocations.isTooClose(lineSegment.x, lineSegment.y));
    }

  } else {
    for (auto &lineSegment: lineSegments) {
      drawOrGap(lineSegment, textLocations.isTooClose(lineSegment.x, lineSegment.y));
    }
  }

  drawImage->endLine(dashes, numDashes);
}

void drawContour(float *sourceGrid, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration) {

  if (styleConfiguration->contourLines.size() == 0) {
    return;
  }
  StopWatch_Measure("[drawContour] %d contour definitions", (int)styleConfiguration->contourLines.size());

  double scaling = dataSource->getContourScaling();
  // getContourFont tolerates a configuration without WMS or ContourFont elements, e.g. when the config could not be fully loaded
  auto [contourFontSize, contourFontLocation] = dataSource->srvParams->getContourFont();
  const char *fontLocation = contourFontLocation.c_str();

  double defaultLineWidth = 4;
  double defaultStrokeWidth = 0.75;
  double defaultFontSize = contourFontSize;
  CColor defaultLineColor = CColor(0, 0, 0, 255);
  CColor defaultTextColor = CColor(0, 0, 0, 255);
  CColor defaultTextStrokeColor = CColor(0, 0, 0, 0);

  int dImageWidth = drawImage->geoParams.width;
  int dImageHeight = drawImage->geoParams.height;
  size_t imageSize = (dImageHeight + 0) * (dImageWidth + 1);

  // Create a distance field, this is where the line information will be put in.
  // calloc gives zeroed memory, the OS can provide this lazily which is much faster than memset for large images.
  std::unique_ptr<DISTANCEFIELDTYPE[], void (*)(void *)> distanceStorage(static_cast<DISTANCEFIELDTYPE *>(calloc(imageSize, sizeof(DISTANCEFIELDTYPE))), free);
  DISTANCEFIELDTYPE *distance = distanceStorage.get();
  if (distance == NULL) {
    CDBError("drawContour: unable to allocate distance field of %zu elements for %dx%d image", imageSize, dImageWidth, dImageHeight);
    return;
  }
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
  // Sorted copies of classes, for a binary-search range check below.
  std::vector<std::vector<double>> sortedClassesList;
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
  // Pixels where at least one contour definition set a bit, so tracing doesn't rescan the whole image.
  std::vector<i4point> candidatePixels;
  auto fillRows = [&]() {
    for (int y = 0; y < dImageHeight - 1; y++) {
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
            // Smallest multiple >= min, not nearest: rounding to nearest can land below min on a
            // steep gradient and silently drop the most tightly packed contour lines.
            float cc = std::ceil(min * inverseIntervals[j]) * interval;
            if (cc >= min && cc < max) {
              foundLines |= mask;
            }
          }
          mask = mask + mask;
        }
        // Skip the write when nothing was found, so the untouched calloc page stays unmapped.
        if (foundLines) {
          distance[p1] |= foundLines;
          candidatePixels.push_back({.x = x, .y = y});
        }
      }
    }
  };

  StopWatch_Measure("drawContour: filling distance field");
  fillRows();

  StopWatch_Measure("drawContour: done filling distance field");

  TextLocationIndex textLocations;

  DISTANCEFIELDTYPE lineMask = 1;

  int contourLineIndex = 0;
  for (auto &contourLine: contourlineList) {
    StopWatch_Measure("drawContour: start contour definition %d", contourLineIndex);
    for (const auto &candidate: candidatePixels) {
      size_t p = candidate.x + candidate.y * dImageWidth;
      if (distance[p] & lineMask) {
        traverseLine(drawImage, distance, sourceGrid, candidate.x, candidate.y, dImageWidth, dImageHeight, contourLine, lineMask, textLocations, scaling, fontLocation);
      }
    }
    StopWatch_Measure("drawContour: done contour definition %d", contourLineIndex);
    lineMask = lineMask + lineMask;
    contourLineIndex++;
  }

  StopWatch_Measure("[/drawContour]");
}
