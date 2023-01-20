/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

/*
 *
 * http://datagenetics.com/blog/august32013/index.html
 *
 */

#include "CAreaMapper.h"
#include "CImageDataWriter.h"

const char *CAreaMapper::className = "CAreaMapper";

void CAreaMapper::init(CDataSource *dataSource, CDrawImage *drawImage, int tileWidth, int tileHeight) {
  this->dataSource = dataSource;
  this->drawImage = drawImage;
  dfTileWidth = double(tileWidth);
  dfTileHeight = double(tileHeight);
  for (int k = 0; k < 4; k++) {
    dfSourceBBOX[k] = dataSource->dfBBOX[k];
    dfImageBBOX[k] = dataSource->dfBBOX[k];
  }

  /* Look whether BBOX was swapped in y dir */
  if (dataSource->dfBBOX[3] < dataSource->dfBBOX[1]) {
    dfSourceBBOX[1] = dataSource->dfBBOX[3];
    dfSourceBBOX[3] = dataSource->dfBBOX[1];
  }
  /* Look whether BBOX was swapped in x dir */
  if (dataSource->dfBBOX[2] < dataSource->dfBBOX[0]) {
    dfSourceBBOX[0] = dataSource->dfBBOX[2];
    dfSourceBBOX[2] = dataSource->dfBBOX[0];
  }

  debug = false;

  if (dataSource->cfgLayer->TileSettings.size() == 1) {
    if (dataSource->cfgLayer->TileSettings[0]->attr.debug.equals("true")) {
      debug = true;
    }
  }

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();

  dfNodataValue = dataSource->getDataObject(0)->dfNodataValue;
  legendValueRange = styleConfiguration->hasLegendValueRange;
  legendLowerRange = styleConfiguration->legendLowerRange;
  legendUpperRange = styleConfiguration->legendUpperRange;
  hasNodataValue = dataSource->getDataObject(0)->hasNodataValue;
  width = dataSource->dWidth;
  height = dataSource->dHeight;
  legendLog = styleConfiguration->legendLog;
  if (legendLog > 0) {
    legendLogAsLog = log10(legendLog);
  } else {
    legendLogAsLog = 0;
  }
  legendScale = styleConfiguration->legendScale;
  legendOffset = styleConfiguration->legendOffset;

  internalWidth = width;
  internalHeight = height;

  /* Check the if we want to use discrete type */
  if (styleConfiguration != NULL && styleConfiguration->styleConfig != NULL && styleConfiguration->styleConfig->RenderSettings.size() == 1) {
    CT::string renderHint = styleConfiguration->styleConfig->RenderSettings[0]->attr.renderhint;
    if (renderHint.equals(RENDERHINT_DISCRETECLASSES)) {
      isUsingShadeIntervals = true;
    }
  }

  /* Make a shorthand vector from the shadeInterval configuration*/
  if (isUsingShadeIntervals) {
    int numShadeDefs = (int)styleConfiguration->shadeIntervals->size();
    intervals.reserve(numShadeDefs);
    for (int j = 0; j < numShadeDefs; j++) {
      CServerConfig::XMLE_ShadeInterval *shadeInterVal = ((*styleConfiguration->shadeIntervals)[j]);
      intervals.push_back(Interval(shadeInterVal->attr.min.toFloat(), shadeInterVal->attr.max.toFloat(), CColor(shadeInterVal->attr.fillcolor.c_str())));
      /* Check for bgcolor */
      if (j == 0) {
        if (shadeInterVal->attr.bgcolor.empty() == false) {
          bgColorDefined = true;
          bgColor = CColor(shadeInterVal->attr.bgcolor.c_str());
        }
      }
    }
  }
}

int CAreaMapper::drawTile(double *x_corners, double *y_corners, int &dDestX, int &dDestY) {
  CDFType dataType = dataSource->getDataObject(0)->cdfVariable->getType();
  void *data = dataSource->getDataObject(0)->cdfVariable->data;
  switch (dataType) {
  case CDF_CHAR:
    return myDrawRawTile((const char *)data, x_corners, y_corners, dDestX, dDestY);
    break;
  case CDF_BYTE:
    return myDrawRawTile((const char *)data, x_corners, y_corners, dDestX, dDestY);
    break;
  case CDF_UBYTE:
    return myDrawRawTile((const unsigned char *)data, x_corners, y_corners, dDestX, dDestY);
    break;
  case CDF_SHORT:
    return myDrawRawTile((const short *)data, x_corners, y_corners, dDestX, dDestY);
    break;
  case CDF_USHORT:
    return myDrawRawTile((const ushort *)data, x_corners, y_corners, dDestX, dDestY);
    break;
  case CDF_INT:
    return myDrawRawTile((const int *)data, x_corners, y_corners, dDestX, dDestY);
    break;
  case CDF_UINT:
    return myDrawRawTile((const uint *)data, x_corners, y_corners, dDestX, dDestY);
    break;
  case CDF_FLOAT:
    return myDrawRawTile((const float *)data, x_corners, y_corners, dDestX, dDestY);
    break;
  case CDF_DOUBLE:
    return myDrawRawTile((const double *)data, x_corners, y_corners, dDestX, dDestY);
    break;
  }
  return 1;
}

template <class T> int CAreaMapper::myDrawRawTile(const T *data, double *x_corners, double *y_corners, int &dDestX, int &dDestY) {
  int imageWidth = drawImage->getWidth();
  int imageHeight = drawImage->getHeight();
  int k;
  for (k = 0; k < 4; k++) {
    if (fabs(x_corners[k] - x_corners[0]) >= fabs(dfSourceBBOX[2] - dfSourceBBOX[0])) break;
  }
  if (k == 4) {
    for (k = 0; k < 4; k++) {
      if (x_corners[k] > dfSourceBBOX[0] && x_corners[k] < dfSourceBBOX[2]) break;
    }
    if (k == 4) {
      return __LINE__;
    }
  }
  for (k = 0; k < 4; k++) {
    if (fabs(y_corners[k] - y_corners[0]) >= fabs(dfSourceBBOX[3] - dfSourceBBOX[1])) break;
  }
  if (k == 4) {
    for (k = 0; k < 4; k++) {
      if (y_corners[k] > dfSourceBBOX[1] && y_corners[k] < dfSourceBBOX[3]) break;
    }
    if (k == 4) {
      return __LINE__;
    }
  }

  bool isNodata = false;
  T val;
  T nodataValue = (T)dfNodataValue;
  size_t imgpointer;
  double destBoxTL[2] = {x_corners[3], y_corners[3]};
  double destBoxTR[2] = {x_corners[0], y_corners[0]};
  double destBoxBL[2] = {x_corners[2], y_corners[2]};
  double destBoxBR[2] = {x_corners[1], y_corners[1]};

  double sourceBBOXWidth = dfImageBBOX[2] - dfImageBBOX[0];
  double sourceBBOXHeight = dfImageBBOX[1] - dfImageBBOX[3];
  double sourceWidth = double(width);
  double sourceHeight = double(height);
  double destPXTL[2] = {((destBoxTL[0] - dfImageBBOX[0]) / sourceBBOXWidth) * sourceWidth, ((destBoxTL[1] - dfImageBBOX[3]) / sourceBBOXHeight) * sourceHeight};
  double destPXTR[2] = {((destBoxTR[0] - dfImageBBOX[0]) / sourceBBOXWidth) * sourceWidth, ((destBoxTR[1] - dfImageBBOX[3]) / sourceBBOXHeight) * sourceHeight};
  double destPXBL[2] = {((destBoxBL[0] - dfImageBBOX[0]) / sourceBBOXWidth) * sourceWidth, ((destBoxBL[1] - dfImageBBOX[3]) / sourceBBOXHeight) * sourceHeight};
  double destPXBR[2] = {((destBoxBR[0] - dfImageBBOX[0]) / sourceBBOXWidth) * sourceWidth, ((destBoxBR[1] - dfImageBBOX[3]) / sourceBBOXHeight) * sourceHeight};

  for (double dstpixel_y = 0.5; dstpixel_y < dfTileHeight + 0.5; dstpixel_y++) {
    /* Calculate left and right ribs of source image, leftLineY  and rightLineY follow the ribs according to dstpixel_y */
    double leftLineA = (destPXBL[0] - destPXTL[0]) / (destPXBL[1] - destPXTL[1]);                 /* RC coefficient of left rib, (X2-X1)/(Y2-Y1) by (X = AY + B) */
    double leftLineB = destPXTL[0] - destPXTL[1] * leftLineA;                                     /* X1 - Y1 * RC */
    double leftLineY = (dstpixel_y / (dfTileHeight)) * (destPXBL[1] - destPXTL[1]) + destPXTL[1]; /* 0 - dfTileHeight --> 0 - 1 --> destPXTL[1] - destPXBL[1]*/
    double leftLineX = leftLineY * leftLineA + leftLineB;

    double rightLineA = (destPXBR[0] - destPXTR[0]) / (destPXBR[1] - destPXTR[1]);                 /* RC coefficient of right rib, (X2-X1)/(Y2-Y1) */
    double rightLineB = destPXTR[0] - destPXTR[1] * rightLineA;                                    /* X1 - Y1 * RC */
    double rightLineY = (dstpixel_y / (dfTileHeight)) * (destPXBR[1] - destPXTR[1]) + destPXTR[1]; /* 0 - dfTileHeight --> 0 - 1 --> destPXTL[1] - destPXBL[1] */
    double rightLineX = rightLineY * rightLineA + rightLineB;

    /* Now calculate points from leftLineX, leftLineY to rightLineX, rightLineY */
    double lineA = (rightLineY - leftLineY) / (rightLineX - leftLineX); /* RC coefficient of top rib (Y= AX + B)*/
    double lineB = leftLineY - leftLineX * lineA;

    for (double dstpixel_x = 0.5; dstpixel_x < dfTileWidth + 0.5; dstpixel_x++) {

      double dfSourceSampleX = (dstpixel_x / (dfTileWidth)) * (rightLineX - leftLineX) + leftLineX;
      double dfSourceSampleY = dfSourceSampleX * lineA + lineB;
      int sourceSampleX = floor(dfSourceSampleX);
      int sourceSampleY = floor(dfSourceSampleY);
      int destPixelX = dstpixel_x + dDestX;
      int destPixelY = dstpixel_y + dDestY;

      if (sourceSampleX >= 0 && sourceSampleX < width && sourceSampleY >= 0 && sourceSampleY < height && destPixelX >= 0 && destPixelY >= 0 && destPixelX < imageWidth && destPixelY < imageHeight) {
        imgpointer = sourceSampleX + (sourceSampleY)*width;
        val = data[imgpointer];
        isNodata = false;
        if (hasNodataValue) {
          if (val == nodataValue) isNodata = true;
        }
        if (!(val == val)) isNodata = true;
        if (!isNodata)
          if (legendValueRange)
            if (val < legendLowerRange || val > legendUpperRange) isNodata = true;
        if (!isNodata) {
          if (isUsingShadeIntervals) {
            bool pixelSet = false; // Remember if a pixel was set. If not set and bgColorDefined is defined, draw the background color.
            for (size_t j = 0; (j < intervals.size() && pixelSet == false); j += 1) {
              if (val >= intervals[j].min && val < intervals[j].max) {
                drawImage->setPixel(destPixelX, dstpixel_y + dDestY, intervals[j].color);
                pixelSet = true;
              }
            }
            if (bgColorDefined && pixelSet == false) {
              drawImage->setPixel(destPixelX, dstpixel_y + dDestY, bgColor);
            }
          } else {
            if (legendLog != 0) {
              if (val > 0) {
                val = (T)(log10(val) / legendLogAsLog);
              } else
                val = (T)(-legendOffset);
            }
            int pcolorind = (int)(val * legendScale + legendOffset);
            if (pcolorind >= 239)
              pcolorind = 239;
            else if (pcolorind <= 0)
              pcolorind = 0;
            drawImage->setPixelIndexed(destPixelX, dstpixel_y + dDestY, pcolorind);
          }
        }

        if (debug) {
          bool draw = false;
          bool draw2 = false;
          if (sourceSampleX == 0 || sourceSampleX == width - 1 || sourceSampleY == 0 || sourceSampleY == height - 1) {
            draw = true;
          }
          if ((sourceSampleX == 10 || sourceSampleX == width - 10) && sourceSampleY > 10 && sourceSampleY < height - 10) {
            draw2 = true;
          }
          if ((sourceSampleY == 10 || sourceSampleY == width - 10) && sourceSampleX > 10 && sourceSampleX < width - 10) {
            draw2 = true;
          }
          if (draw) {
            drawImage->setPixelIndexed(destPixelX, destPixelY, 249);
          }
          if (draw2) {
            drawImage->setPixelIndexed(destPixelX, destPixelY, 244);
          }
        }
      }
    }
  }

  return 0;
}
