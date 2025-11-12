
#include "drawContour.h"
#include <CDrawImage.h>
#include <set>

#define CONTOURDEFINITIONLOOKUPLENGTH 32
#define DISTANCEFIELDTYPE unsigned int

struct ContourLineStructure {
  std::vector<double> classes;
  double interval = 0;
  CT::string textformatting = "%0.1f";
  CColor lineColor = CColor(0, 0, 0, 255);
  CColor textColor = CColor(0, 0, 0, 255);
  CColor textstrokecolor = CColor(0, 0, 0, 0);
  double lineWidth = 4;
  double fontSize = 10;
  double textStrokeWidth = 0.75;
  std::vector<double> dashes;
};

bool IsTextTooClose(std::vector<i4point> &textLocations, int x, int y) {
  for (auto &textLocation : textLocations) {
    int dx = x - textLocation.x;
    int dy = y - textLocation.y;
    if ((dx * dx + dy * dy) < 10 * 10) {
      return true;
    }
  }
  return false;
}

void drawTextForContourLines(CDrawImage *drawImage, ContourLineStructure &contourDefinition, int lineX, int lineY, int endX, int endY, std::vector<i4point> &, float value, const char *fontLocation,
                             float scaling) {

  /* Draw text */
  CT::string text;
  contourDefinition.textformatting.empty() ? text.print(contourDefinition.textformatting.c_str(), value) : text.print("%g", value);
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
                  DISTANCEFIELDTYPE lineMask, std::vector<i4point> &textLocations, double scaling, const char *fontLocation) {
  size_t p = lineX + lineY * dImageWidth; /* Starting pointer */
  bool foundLine = true;                  /* This function starts at the beginning of a line segment */
  int maxLineDistance = 5;                /* Maximum length of each line segment */
  int currentLineDistance = maxLineDistance;
  std::vector<i4point> lineSegments;

  size_t numDashes = contourDefinition.dashes.size();
  double *dashes = &contourDefinition.dashes[0];

  /* Push the beginning of this line*/
  lineSegments.push_back({.x = lineX, .y = lineY});

  float lineSegmentsValue = valueField[p];
  float binnedLineSegmentsValue;

  if (!contourDefinition.classes.size() == 0) {
    float closestValue;
    int definedIntervalIndex = 0;
    int j = 0;
    for (auto c : contourDefinition.classes) {
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
    binnedLineSegmentsValue = convertValueToClass(lineSegmentsValue + interval / 2, interval);
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
        distance[p] &= ~lineMask; /* Indicate found, set to false */
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
            break;
          }
        }
      }
    }
    if (!foundLine) {
      // drawImage->rectangle(lineX - 5, lineY - 5, lineX + 5, lineY + 5, 240);
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

  // textLocations.clear();
  /* Now draw this line */
  drawImage->moveTo(lineSegments[0].x, lineSegments[0].y);

  bool doDrawText = !contourDefinition.textformatting.empty();

  bool textSkip = false;
  bool textOn = false;

  int drawTextAtEveryNPixels = 50 * int(scaling);
  int drawTextAngleNSteps = 0;
  int drawTextAngleNSteps5 = 5 * int(scaling) * (contourDefinition.fontSize / 8);

  float scaledLineWidth = contourDefinition.lineWidth * scaling;
  int j = -1;
  for (auto &lineSegment : lineSegments) {
    j++;
    if (doDrawText) {
      if (j % drawTextAtEveryNPixels == drawTextAngleNSteps) {
        textOn = false;
        if (IsTextTooClose(textLocations, lineSegment.x, lineSegment.y) == false) {
          textSkip = false;
          textLocations.push_back(lineSegment);

          float endX = lineSegments[j + drawTextAngleNSteps5].x;
          float endY = lineSegments[j + drawTextAngleNSteps5].y;
          drawTextForContourLines(drawImage, contourDefinition, lineSegment.x, lineSegment.y, endX, endY, textLocations, binnedLineSegmentsValue, fontLocation, scaling);

          textOn = true;
        } else {
          textSkip = true;
        }
      }
      if (j % drawTextAtEveryNPixels == drawTextAngleNSteps && textOn && !textSkip) {
        drawImage->endLine(dashes, numDashes);
      }
      if (j % drawTextAtEveryNPixels > drawTextAngleNSteps5 || textSkip) {
        drawImage->lineTo(lineSegment.x, lineSegment.y, scaledLineWidth, contourDefinition.lineColor);
      }
    } else {
      drawImage->lineTo(lineSegment.x, lineSegment.y, scaledLineWidth, contourDefinition.lineColor);
    }
  }

  drawImage->endLine(dashes, numDashes);
}

void drawContour(float *sourceGrid, CDataSource *dataSource, CDrawImage *drawImage) {
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  CDBDebug("drawContour");
  double scaling = dataSource->getContourScaling();
  const char *fontLocation = dataSource->srvParams->cfg->WMS[0]->ContourFont[0]->attr.location.c_str();

  double defaultLineWidth = 4;
  double defaultStrokeWidth = 0.75;
  double defaultFontSize = dataSource->srvParams->cfg->WMS[0]->ContourFont[0]->attr.size.toDouble();
  CColor defaultLineColor = CColor(0, 0, 0, 255);
  CColor defaultTextColor = CColor(0, 0, 0, 255);
  CColor defaultTextStrokeColor = CColor(0, 0, 0, 0);

  int dImageWidth = drawImage->geoParams.width;
  int dImageHeight = drawImage->geoParams.height;
  size_t imageSize = (dImageHeight + 0) * (dImageWidth + 1);

  // Create a distance field, this is where the line information will be put in.
  DISTANCEFIELDTYPE *distance = new DISTANCEFIELDTYPE[imageSize];

  // Determine contour lines
  memset(distance, 0, imageSize * sizeof(DISTANCEFIELDTYPE));

  std::vector<ContourLineStructure> contourlineList;

  for (auto contourLine : (styleConfiguration->contourLines)) {
    auto &attr = contourLine->attr;
    std::vector<double> classes;
    double interval = 0;
    if (attr.classes.empty() == false) {
      auto classesString = attr.classes.splitToStack(",");
      for (auto classString : classesString) {
        classes.push_back(classString.toDouble());
      }
    } else if (attr.interval.empty() == false) {
      interval = attr.interval.toDouble();
    }
    std::vector<double> dashes;
    auto dashesStrings = attr.dashing.splitToStack(",");
    for (auto dashString : dashesStrings) {
      dashes.push_back(dashString.toDouble());
    }

    // Check for lines at specified classes

    contourlineList.push_back({.classes = classes,
                               .interval = interval,
                               .textformatting = attr.textformatting,
                               .lineColor = attr.linecolor.empty() ? defaultLineColor : CColor(attr.linecolor),
                               .textColor = attr.textcolor.empty() ? defaultTextColor : CColor(attr.textcolor),
                               .textstrokecolor = attr.textstrokecolor.empty() ? defaultTextStrokeColor : CColor(attr.textstrokecolor),
                               .lineWidth = attr.width.empty() ? defaultLineWidth : attr.width.toDouble(),
                               .fontSize = attr.textsize.empty() ? defaultFontSize : attr.textsize.toDouble(),
                               .textStrokeWidth = attr.textstrokewidth.empty() ? defaultStrokeWidth : attr.textstrokewidth.toDouble(),
                               .dashes = dashes});
  }

  float fNodataValue = dataSource->getDataObject(0)->dfNodataValue;
  for (int y = 0; y < dImageHeight - 1; y++) {
    for (int x = 0; x < dImageWidth - 1; x++) {
      size_t p1 = size_t(x + y * dImageWidth);
      const double val[4] = {sourceGrid[p1], sourceGrid[p1 + 1], sourceGrid[p1 + dImageWidth], sourceGrid[p1 + dImageWidth + 1]};
      // Check if all pixels have values...
      if (val[0] != fNodataValue && val[1] != fNodataValue && val[2] != fNodataValue && val[3] != fNodataValue && val[0] == val[0] && val[1] == val[1] && val[2] == val[2] && val[3] == val[3]) {
        int mask = 1;
        for (auto contourLine : contourlineList) {
          // Check for lines at specified classes

          float min = std::min(val[0], std::min(val[1], std::min(val[2], val[3])));
          float max = std::max(val[0], std::max(val[1], std::max(val[2], val[3])));
          for (double cc : contourLine.classes) {
            if (cc >= min && cc < max) {
              distance[p1] |= mask;
              break;
            }
          }
          if (contourLine.interval > 0) {
            // Check for lines at continous interval
            float cc = round(min / contourLine.interval) * contourLine.interval;
            if (cc >= min && cc < max) {
              distance[p1] |= mask;
            }
          }

          mask = mask + mask;
        }
      }
    }
  }

  std::vector<i4point> textLocations;

  DISTANCEFIELDTYPE lineMask = 1;

  // CDBDebug("B %d", styleConfiguration->contourLines.size());
  for (auto contourLine : contourlineList) {

    /* Everywhere */
    for (int y = 0; y < dImageHeight; y++) {
      for (int x = 0; x < dImageWidth; x++) {
        size_t p = x + y * dImageWidth;
        if (distance[p] & lineMask) {
          traverseLine(drawImage, distance, sourceGrid, x, y, dImageWidth, dImageHeight, contourLine, lineMask, textLocations, scaling, fontLocation);
        }
      }
    }
    lineMask = lineMask + lineMask;
  }

#ifdef CImgWarpBilinear_DEBUG
  CDBDebug("Deleting distance[]");
#endif

  delete[] distance;

#ifdef CImgWarpBilinear_DEBUG
  CDBDebug("Finished drawing lines and text");
#endif
}