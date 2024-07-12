#ifndef CDRAWFUNCTION
#define CDRAWFUNCTION

#include <cmath>
#include <float.h>
#include <pthread.h>
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CGenericDataWarper.h"
#include "CStyleConfiguration.h"

class CDrawFunctionSettings {
public:
  class Interval {
  public:
    float min;
    float max;
    CColor color;
    Interval(float min, float max, CColor color) {
      this->min = min;
      this->max = max;
      this->color = color;
    }
  };
  bool isUsingShadeIntervals = false;
  std::vector<Interval> intervals;
  CColor bgColor;
  bool bgColorDefined = false;
  double dfNodataValue;
  double legendValueRange;
  double legendLowerRange;
  double legendUpperRange;
  bool hasNodataValue;
  float legendLog;
  float legendLogAsLog;
  float legendScale;
  float legendOffset;
  CDrawImage *drawImage;
  float *smoothingDistanceMatrix = nullptr;
  int smoothingFiter = 0;
};

CDrawFunctionSettings getDrawFunctionSettings(CDataSource *dataSource, CDrawImage *drawImage, const CStyleConfiguration *styleConfiguration);

template <class T> void setPixelInDrawImage(int x, int y, T val, CDrawFunctionSettings *settings) {
  bool isNodata = false;

  if (settings->hasNodataValue) {
    /*
     * Casting the double dfNodataValue back to the precision of the data itself,
     * to do a correct comparison to check if this value is a nodatavalue.
     */
    T noDataValue = (T)settings->dfNodataValue;
    if (val == noDataValue) isNodata = true;
  }

  if (std::isnan(val)) isNodata = true;

  if (!isNodata)
    if (settings->legendValueRange)
      if (val < settings->legendLowerRange || val > settings->legendUpperRange) isNodata = true;
  if (!isNodata) {
    if (settings->isUsingShadeIntervals) {
      bool pixelSet = false; // Remember if a pixel was set. If not set and bgColorDefined is defined, draw the background color.
      for (size_t j = 0; (j < settings->intervals.size() && pixelSet == false); j += 1) {
        if (val >= settings->intervals[j].min && val < settings->intervals[j].max) {
          settings->drawImage->setPixel(x, y, settings->intervals[j].color);
          pixelSet = true;
        }
      }
      if (settings->bgColorDefined && pixelSet == false) {
        settings->drawImage->setPixel(x, y, settings->bgColor);
      }
    } else {
      if (settings->legendLog != 0) {
        if (val > 0) {
          val = (T)(log10(val) / settings->legendLogAsLog);
        } else
          val = (T)(-settings->legendOffset);
      }
      int pcolorind = (int)(val * settings->legendScale + settings->legendOffset);
      if (pcolorind >= 239)
        pcolorind = 239;
      else if (pcolorind <= 0)
        pcolorind = 0;
      settings->drawImage->setPixelIndexed(x, y, pcolorind);
    }
  }
}
#endif