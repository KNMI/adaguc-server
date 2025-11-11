#include "GenericDataWarper/gdwDrawFunction.h"
#include "CImageOperators/smoothRasterField.h"
#include <sys/types.h>

static inline int nfast_mod(const int input, const int ceil) { return input >= ceil ? input % ceil : input; }

void setPixelInDrawImage(int x, int y, double val, GDWDrawFunctionSettings *settings) {
  bool isNodata = false;
  if (settings->hasNodataValue) {
    if (val == settings->dfNodataValue) isNodata = true;
  }
  if (std::isnan(val)) isNodata = true;

  if (!isNodata)
    if (settings->legendValueRange)
      if (val < settings->legendLowerRange || val > settings->legendUpperRange) isNodata = true;
  if (!isNodata) {
    if (settings->isUsingShadeIntervals && settings->shadeInterval == 0) {
      bool pixelSet = false; // Remember if a pixel was set. If not set and bgColorDefined is defined, draw the background color.
      if (settings->intervals.size() > 0) {
        for (size_t j = 0; (j < settings->intervals.size() && pixelSet == false); j += 1) {
          if (val >= settings->intervals[j].min && val < settings->intervals[j].max) {
            settings->drawImage->setPixel(x, y, settings->intervals[j].color);
            pixelSet = true;
          }
        }
      }

      if (pixelSet == false && settings->bgColorDefined) {
        settings->drawImage->setPixel(x, y, settings->bgColor);
      }

    } else {
      // val = floor(val / 2) * 2;
      if (settings->legendLog != 0) {

        if (val > 0) {
          val = (log10(val) / settings->legendLogAsLog);
        } else
          val = (-settings->legendOffset);
      }
      if (settings->shadeInterval > 0) {
        val = floor(val / settings->shadeInterval);
        val = val * settings->shadeInterval;
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

double gdwGetValueFromSourceFunction(int x, int y, GDWDrawFunctionSettings *drawSettings) {
  if (drawSettings == nullptr) {
    return x + y;
  }

  return 0;
};

void gdwDrawFunction(GDWState *_drawSettings) {
  // GDWDrawFunctionSettings *drawSettings = (GDWDrawFunctionSettings *)_drawSettings;
  // int x = drawSettings->destX;
  // int y = drawSettings->destY;
  // if (x < 0 || y < 0 || x >= drawSettings->destDataWidth || y >= drawSettings->destDataHeight) return;

  // double val = gdwGetValueFromSourceFunction(drawSettings->sourceDataPX, drawSettings->sourceDataPY, drawSettings);
  // bool isNodata = false;
  // if (drawSettings->hasNodataValue) {
  //   if (val == drawSettings->dfNodataValue) isNodata = true;
  // }
  // if (!(val == val)) isNodata = true;
  // if (!isNodata) {
  //   int sourceDataPX = drawSettings->sourceDataPX;
  //   int sourceDataPY = drawSettings->sourceDataPY;
  //   int sourceDataWidth = drawSettings->sourceDataWidth;
  //   int sourceDataHeight = drawSettings->sourceDataHeight;

  //   if (sourceDataPX < 0 || sourceDataPY < 0 || sourceDataPY > sourceDataHeight - 1 || sourceDataPX > sourceDataWidth - 1) return;

  //   bool bilinear = drawSettings->drawInImage == DrawInImageBilinear || drawSettings->drawInDataGrid == DrawInDataGridBilinear;
  //   bool nearest = drawSettings->drawInImage == DrawInImageNearest || drawSettings->drawInDataGrid == DrawInDataGridNearest;

  //   if (drawSettings->smoothingFiter == 0) {

  //     // Bilinear
  //     if (bilinear && sourceDataPY <= sourceDataHeight - 2 && sourceDataPX <= sourceDataWidth - 2) {
  //       int xL = (sourceDataPX + 0);
  //       int yT = (sourceDataPY + 0);
  //       int xR = (sourceDataPX + 1);
  //       int yB = (sourceDataPY + 1);
  //       double values[2][2];
  //       values[0][0] = gdwGetValueFromSourceFunction(xL, yT, drawSettings);
  //       values[1][0] = gdwGetValueFromSourceFunction(xR, yT, drawSettings);
  //       values[0][1] = gdwGetValueFromSourceFunction(xL, yB, drawSettings);
  //       values[1][1] = gdwGetValueFromSourceFunction(xR, yB, drawSettings);

  //       double dx = drawSettings->tileDx;
  //       double dy = drawSettings->tileDy;
  //       double gx1 = (1 - dx) * values[0][0] + dx * values[1][0];
  //       double gx2 = (1 - dx) * values[0][1] + dx * values[1][1];
  //       double billValue = (1 - dy) * gx1 + dy * gx2;
  //       // Draw in drawImage pixels, interpolated with bilinear method
  //       if (drawSettings->drawInImage == DrawInImageBilinear) {
  //         setPixelInDrawImage(x, y, billValue, drawSettings);
  //       }

  //       if (drawSettings->drawInDataGrid == DrawInDataGridBilinear && drawSettings->destinationGrid != nullptr) {
  //         ((double *)drawSettings->destinationGrid)[x + y * drawSettings->drawImage->Geo->dWidth] = billValue;
  //       }
  //     }

  //     // Nearest
  //     if (nearest) {
  //       int xL = (sourceDataPX + 0);
  //       int yT = (sourceDataPY + 0);

  //       double value = gdwGetValueFromSourceFunction(xL, yT, drawSettings);

  //       // Draw in drawImage pixels, interpolated with nearest method
  //       if (drawSettings->drawInImage == DrawInImageNearest) {
  //         setPixelInDrawImage(x, y, value, drawSettings);
  //       }

  //       if (drawSettings->drawInDataGrid == DrawInDataGridNearest && drawSettings->destinationGrid != nullptr) {
  //         ((double *)drawSettings->destinationGrid)[x + y * drawSettings->drawImage->Geo->dWidth] = value; // TODO
  //       }
  //     }

  //   } else {
  //     // values[0][0] = smoothingAtLocation((float *)sourceData, drawSettings->smoothingDistanceMatrix, drawSettings->smoothingFiter, (float)drawSettings->dfNodataValue, sourceDataPX, sourceDataPY,
  //     //                                    sourceDataWidth, sourceDataHeight);
  //     // values[1][0] = smoothingAtLocation((float *)sourceData, drawSettings->smoothingDistanceMatrix, drawSettings->smoothingFiter, (float)drawSettings->dfNodataValue, sourceDataPX + 1,
  //     // sourceDataPY,
  //     //                                    sourceDataWidth, sourceDataHeight);
  //     // values[0][1] = smoothingAtLocation((float *)sourceData, drawSettings->smoothingDistanceMatrix, drawSettings->smoothingFiter, (float)drawSettings->dfNodataValue, sourceDataPX, sourceDataPY
  //     +
  //     // 1,
  //     //                                    sourceDataWidth, sourceDataHeight);
  //     // values[1][1] = smoothingAtLocation((float *)sourceData, drawSettings->smoothingDistanceMatrix, drawSettings->smoothingFiter, (float)drawSettings->dfNodataValue, sourceDataPX + 1,
  //     //                                    sourceDataPY + 1, sourceDataWidth, sourceDataHeight);
  //   }
  // }
};
