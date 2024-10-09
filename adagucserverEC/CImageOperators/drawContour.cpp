
#include "drawContour.h"
#include <CDrawImage.h>
#include <Types/DrawPoint.h>
#include <set>
#include <Types/DrawShadeDefinition.h>

#define CONTOURDEFINITIONLOOKUPLENGTH 32
#define DISTANCEFIELDTYPE unsigned int

bool IsTextTooClose(std::vector<Point> *textLocations, int x, int y) {

  for (size_t j = 0; j < textLocations->size(); j++) {
    int dx = x - (*textLocations)[j].x;
    int dy = y - (*textLocations)[j].y;
    if ((dx * dx + dy * dy) < 10 * 10) {
      return true;
    }
  }
  return false;
}

void traverseLine(CDrawImage *drawImage, DISTANCEFIELDTYPE *distance, float *valueField, int lineX, int lineY, int dImageWidth, int dImageHeight, float lineWidth, CColor lineColor, CColor textColor,
                  CColor textStrokeColor, CServerConfig::XMLE_ContourLine *contourDefinition, DISTANCEFIELDTYPE lineMask, bool, std::vector<Point> *textLocations, double scaling,
                  const char *fontLocation, float fontSize, float textStrokeWidth, std::vector<double> dashesVector) {
  size_t p = lineX + lineY * dImageWidth; /* Starting pointer */
  bool foundLine = true;                  /* This function starts at the beginning of a line segment */
  int maxLineDistance = 5;                /* Maximum length of each line segment */
  int currentLineDistance = maxLineDistance;
  int lineSegmentsX[MAX_LINE_SEGMENTS + 1];
  int lineSegmentsY[MAX_LINE_SEGMENTS + 1];

  int lineSegmentCounter = 0;

  size_t numDashes = dashesVector.size();
  double *dashes = &dashesVector[0];

  /* Push the beginning of this line*/
  lineSegmentsX[lineSegmentCounter] = lineX;
  lineSegmentsY[lineSegmentCounter] = lineY;
  float lineSegmentsValue = valueField[p];
  float binnedLineSegmentsValue;

  if (!contourDefinition->attr.classes.empty()) {
    float closestValue;
    int definedIntervalIndex = 0;
    auto classes = contourDefinition->attr.classes.splitToStack(",");
    int j = 0;
    for (auto c : classes) {
      float d = fabs(lineSegmentsValue - c.toDouble());
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
    binnedLineSegmentsValue = classes[definedIntervalIndex].toDouble();
  } else {
    double interval = contourDefinition->attr.interval.toDouble();
    binnedLineSegmentsValue = convertValueToClass(lineSegmentsValue + interval / 2, interval);
  }

  lineSegmentCounter++;

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
    // if (!foundLine){
    //   drawImage->rectangle(lineX-5, lineY-5, lineX+5,lineY+5, 240);
    // }
    lineX = nextLineX;
    lineY = nextLineY;
    /* Decrease the max currentLineDist counter,
       when zero, the max line distance is reached
       and we should add a line segment */
    currentLineDistance--;
    if (currentLineDistance <= 0 || foundLine == false) {
      currentLineDistance = maxLineDistance;
      if (lineSegmentCounter < MAX_LINE_SEGMENTS) {
        lineSegmentsX[lineSegmentCounter] = lineX;
        lineSegmentsY[lineSegmentCounter] = lineY;
        lineSegmentCounter++;
        /* If we have reached max line segments, stop it */
        if (lineSegmentCounter >= MAX_LINE_SEGMENTS) {
          foundLine = false;
        }
      }
    }
  }

  // textLocations->clear();
  /* Now draw this line */
  drawImage->moveTo(lineSegmentsX[0], lineSegmentsY[0]);

  bool doDrawText = !contourDefinition->attr.textformatting.empty();

  bool textSkip = false;
  bool textOn = false;

  int drawTextAtEveryNPixels = 50 * int(scaling);
  int drawTextAngleNSteps = 0;
  int drawTextAngleNSteps5 = 5 * int(scaling) * (fontSize / 8);

  float scaledLineWidth = lineWidth * scaling;

  for (int j = 0; j < lineSegmentCounter; j++) {
    // if (doDrawText) {
    //   if (j % drawTextAtEveryNPixels == drawTextAngleNSteps && j + drawTextAngleNSteps5 < lineSegmentCounter) {
    //     textOn = false;
    //     if (IsTextTooClose(textLocations, lineSegmentsX[j], lineSegmentsY[j]) == false) {
    //       textSkip = false;
    //       textLocations->push_back(Point(lineSegmentsX[j], lineSegmentsY[j]));
    //       float startX = lineSegmentsX[j];
    //       float startY = lineSegmentsY[j];
    //       float endX = lineSegmentsX[j + drawTextAngleNSteps5];
    //       float endY = lineSegmentsY[j + drawTextAngleNSteps5];
    //       //   this->drawTextForContourLines(drawImage, contourDefinition, startX, startY, endX, endY, textLocations, binnedLineSegmentsValue, textColor, textStrokeColor, fontLocation, fontSize *
    //       //   scaling,
    //       //                                 textStrokeWidth * scaling);
    //       textOn = true;
    //     } else {
    //       textSkip = true;
    //     }
    //   }
    //   if (j % drawTextAtEveryNPixels == drawTextAngleNSteps && textOn && !textSkip) {
    //     drawImage->endLine(dashes, numDashes);
    //   }
    //   if (j % drawTextAtEveryNPixels > drawTextAngleNSteps5 || textSkip) {
    //     drawImage->lineTo(lineSegmentsX[j], lineSegmentsY[j], scaledLineWidth, lineColor);
    //   }
    // } else {
    drawImage->lineTo(lineSegmentsX[j], lineSegmentsY[j], scaledLineWidth, lineColor);
    // }
  }

  drawImage->endLine(dashes, numDashes);
}

void drawContour(float *sourceGrid, CDataSource *dataSource, CDrawImage *drawImage) {
  CStyleConfiguration *styleConfiguration = dataSource->getStyle(); // TODO SLOW
  CDBDebug("drawContour");
  double scaling = dataSource->getContourScaling();
  const char *fontLocation = dataSource->srvParams->cfg->WMS[0]->ContourFont[0]->attr.location.c_str();

  int dImageWidth = drawImage->Geo->dWidth;
  int dImageHeight = drawImage->Geo->dHeight;
  size_t imageSize = (dImageHeight + 0) * (dImageWidth + 1);

  // Create a distance field, this is where the line information will be put in.
  DISTANCEFIELDTYPE *distance = new DISTANCEFIELDTYPE[imageSize];

  // Determine contour lines
  memset(distance, 0, imageSize * sizeof(DISTANCEFIELDTYPE));

  float fNodataValue = dataSource->getDataObject(0)->dfNodataValue;
  bool drawText = true;
  for (int y = 0; y < dImageHeight - 1; y++) {
    for (int x = 0; x < dImageWidth - 1; x++) {
      size_t p1 = size_t(x + y * dImageWidth);
      float val[4] = {sourceGrid[p1], sourceGrid[p1 + 1], sourceGrid[p1 + dImageWidth], sourceGrid[p1 + dImageWidth + 1]};

      // Check if all pixels have values...
      if (val[0] != fNodataValue && val[1] != fNodataValue && val[2] != fNodataValue && val[3] != fNodataValue && val[0] == val[0] && val[1] == val[1] && val[2] == val[2] && val[3] == val[3]) {

        int mask = 1;
        for (auto contourLine : (*styleConfiguration->contourLines)) {
          // Check for lines at specified classes
          if (contourLine->attr.classes.empty() == false) {
            auto classes = contourLine->attr.classes.splitToStack(",");
            for (size_t c = 0; c < classes.size(); c++) {
              double cc = classes[c].toDouble();
              // Check for classes

              if ((val[0] >= cc && val[1] < cc) || (val[0] > cc && val[1] <= cc) || (val[0] < cc && val[1] >= cc) || (val[0] <= cc && val[1] > cc) || (val[0] > cc && val[2] <= cc) ||
                  (val[0] >= cc && val[2] < cc) || (val[0] <= cc && val[2] > cc) || (val[0] < cc && val[2] >= cc)

              ) {
                distance[p1] |= mask;
                break;
              }
            }

          } else if (contourLine->attr.interval.empty() == false) {
            // Check for lines at continous interval
            double contourinterval = contourLine->attr.interval.toDouble();
            float min, max;
            min = val[0];
            max = val[0];
            for (int j = 1; j < 4; j++) {
              if (val[j] < min) min = val[j];
              if (val[j] > max) max = val[j];
            }

            float classStart = round(val[0] / contourinterval) * contourinterval;
            {
              {
                if ((val[0] > classStart && val[1] <= classStart) || // TL => TR
                    (val[0] > classStart && val[2] <= classStart) || // TL => BL
                    (val[1] > classStart && val[0] <= classStart) || // TR => TL
                    (val[2] > classStart && val[0] <= classStart)    //  BL => TL
                ) {
                  distance[p1] |= mask;
                }
              }
            }
          }

          mask = mask + mask;
        }
      }
    }
  }

  std::vector<Point> textLocations;

  DISTANCEFIELDTYPE lineMask = 1;

  double defaultLineWidth = 4;
  double defaultStrokeWidth = 0.75;
  double defaultFontSize = dataSource->srvParams->cfg->WMS[0]->ContourFont[0]->attr.size.toDouble();
  CColor defaultLineColor = CColor(0, 0, 0, 255);
  CColor defaultTextColor = CColor(0, 0, 0, 255);
  CColor defaultTextStrokeColor = CColor(0, 0, 0, 0);

  CDBDebug("B %d", styleConfiguration->contourLines->size());
  for (auto contourLine : (*styleConfiguration->contourLines)) {

    CColor lineColor = contourLine->attr.linecolor.empty() ? defaultLineColor : CColor(contourLine->attr.linecolor);
    CColor textColor = contourLine->attr.textcolor.empty() ? defaultTextColor : CColor(contourLine->attr.textcolor);
    CColor textstrokecolor = contourLine->attr.textstrokecolor.empty() ? defaultTextStrokeColor : CColor(contourLine->attr.textstrokecolor);
    double lineWidth = contourLine->attr.width.empty() ? defaultLineWidth : contourLine->attr.width.toDouble();
    double fontSize = contourLine->attr.textsize.empty() ? defaultFontSize : contourLine->attr.textsize.toDouble();
    double textStrokeWidth = contourLine->attr.textstrokewidth.empty() ? defaultStrokeWidth : contourLine->attr.textstrokewidth.toDouble();
    auto dashesStrings = contourLine->attr.dashing.splitToStack(",");

    CDBDebug("C %f %d", lineWidth, lineMask);
    std::vector<double> dashes;
    for (auto dashString : dashesStrings) {
      dashes.push_back(dashString.toDouble());
    }

    /* Everywhere */
    for (int y = 0; y < dImageHeight; y++) {
      for (int x = 0; x < dImageWidth; x++) {
        size_t p = x + y * dImageWidth;
        // size_t p1 = size_t(x + y * dImageWidth);
        // auto d = sourceGrid[p1];

        if (distance[p] & lineMask) {
          //   if (d == d && d != NAN && distance[p] & 1) {
          //     auto c = CColor((d + 14) * 2, (d + 14) * 2, (d + 14) * 2, 255);
          //     drawImage->setPixel(x, y, c);
          //   }

          traverseLine(drawImage, distance, sourceGrid, x, y, dImageWidth, dImageHeight, lineWidth, lineColor, textColor, textstrokecolor, contourLine, lineMask, drawText, &textLocations, scaling,
                       fontLocation, fontSize, textStrokeWidth, dashes);
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