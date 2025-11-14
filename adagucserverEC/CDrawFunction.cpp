#include "CDrawFunction.h"

template <class T> void drawIndexPixel(int &x, int &y, T &val, GDWDrawFunctionSettings &settings) {
  if (settings.legendLog != 0) {
    if (val > 0) {
      val = (T)(log10(val) / settings.legendLogAsLog);
    } else
      val = (T)(-settings.legendOffset);
  }
  int pcolorind = (int)(val * settings.legendScale + settings.legendOffset);
  if (pcolorind >= 239)
    pcolorind = 239;
  else if (pcolorind <= 0)
    pcolorind = 0;
  settings.drawImage->setPixelIndexed(x, y, pcolorind);
}

template <class T> void setPixelInDrawImage(int x, int y, T val, GDWDrawFunctionSettings *settings) {
  bool isNodata = false;

  if (settings->hasNodataValue) {
    /*
     * Casting the double dfNodataValue back to the precision of the data itself,
     * to do a correct comparison to check if this value is a nodatavalue.
     */
    T noDataValue = (T)settings->dfNodataValue;
    if (val == noDataValue) isNodata = true;
  }

  if (std::isnan(val)) return;

  if (!isNodata)
    if (settings->legendValueRange)
      if (val < settings->legendLowerRange || val > settings->legendUpperRange) isNodata = true;
  if (!isNodata) {
    if (settings->isUsingShadeIntervals) {
      bool pixelSet = false; // Remember if a pixel was set. If not set and bgColorDefined is defined, draw the background color.
      if (settings->shadeInterval > 0) {
        float f = floor(val / settings->shadeInterval) * settings->shadeInterval;
        drawIndexPixel(x, y, f, *settings);
      } else {
        for (size_t j = 0; (j < settings->intervals.size() && pixelSet == false); j += 1) {
          if (val >= settings->intervals[j].min && val < settings->intervals[j].max) {
            settings->drawImage->setPixel(x, y, settings->intervals[j].color);
            pixelSet = true;
          }
        }
      }
      if (settings->bgColorDefined && pixelSet == false) {
        settings->drawImage->setPixel(x, y, settings->bgColor);
      }
    } else {
      drawIndexPixel(x, y, val, *settings);
    }
  }
}

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE) template void setPixelInDrawImage<CPPTYPE>(int x, int y, CPPTYPE val, GDWDrawFunctionSettings *settings);
ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE
