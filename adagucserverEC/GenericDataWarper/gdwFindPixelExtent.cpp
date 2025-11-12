#include <array>
#include <limits>
#include <Types/GeoParameters.h>
#include <CImageWarper.h>
int gdwFindPixelExtent(int *PXExtentBasedOnSource, GeoParameters &sourceGeoParams, GeoParameters &destGeoParams, CImageWarper *warper) {
  int sourceDataWidth = sourceGeoParams.width;
  int sourceDataHeight = sourceGeoParams.height;

  PXExtentBasedOnSource[0] = 0;
  PXExtentBasedOnSource[1] = 0;
  PXExtentBasedOnSource[2] = sourceDataWidth;
  PXExtentBasedOnSource[3] = sourceDataHeight;

  PXExtentBasedOnSource[0] = -1;
  PXExtentBasedOnSource[1] = -1;
  PXExtentBasedOnSource[2] = -1;
  PXExtentBasedOnSource[3] = -1;

  double dfSourceW = double(sourceGeoParams.width);
  double dfSourceH = double(sourceGeoParams.height);

  int imageHeight = destGeoParams.height;
  int imageWidth = destGeoParams.width;
  double dfDestW = double(destGeoParams.width);
  double dfDestH = double(destGeoParams.height);

  int lowerIndex = 1, higherIndex = 3;

  double dfSourceExtW = sourceGeoParams.bbox.span().x;
  ;
  double dfSourceOrigX = sourceGeoParams.bbox.left;
  double dfSourceExtH = (sourceGeoParams.bbox.get(lowerIndex) - sourceGeoParams.bbox.get(higherIndex));
  double dfSourceOrigY = sourceGeoParams.bbox.get(higherIndex);

  double dfDestExtW = destGeoParams.bbox.right - destGeoParams.bbox.left;
  double dfDestExtH = destGeoParams.bbox.bottom - destGeoParams.bbox.top;
  double dfDestOrigX = destGeoParams.bbox.left;
  double dfDestOrigY = destGeoParams.bbox.top;

  int startX = 0;
  int startY = 0;
  int stopX = imageWidth;
  int stopY = imageHeight;
  bool firstExtent = true;
  bool needsProjection = warper->isProjectionRequired();
  bool OK = false;
  bool transFormationRequired = false;
  bool fullScan = false;

  while (OK == false) {
    OK = true;

    bool attemptToContintue = true;

    int incY = double(imageHeight) / 16 + 0.5;
    int incX = double(imageWidth) / 16 + 0.5;
    if (incY < 1) incY = 1;
    if (incX < 1) incX = 1;

    if (attemptToContintue) {
      for (int y = startY; y < stopY + incY && OK; y = y + incY) {
        for (int x = startX; x < stopX + incX && OK; x = x + incX) {
          if (x == startX || y == startY || x == stopX || y == stopY || fullScan == true || true) {
            double destCoordX, destCoordY;
            destCoordX = (double(x) / dfDestW) * dfDestExtW + dfDestOrigX;
            destCoordY = (double(y) / dfDestH) * dfDestExtH + dfDestOrigY;

            bool skip = false;
            double px = destCoordX, py = destCoordY;
            if (needsProjection) {
              if (warper->isProjectionRequired()) {
                if (proj_trans_generic(warper->projSourceToDest, PJ_INV, &px, sizeof(double), 1, &py, sizeof(double), 1, nullptr, 0, 0, nullptr, 0, 0) != 1) {
                  skip = true;
                  CDBDebug("skip %f %f", px, py);
                }
              }
            }

            double sourcePixelX = ((px - dfSourceOrigX) / dfSourceExtW) * dfSourceW;
            double sourcePixelY = ((py - dfSourceOrigY) / dfSourceExtH) * dfSourceH;

            if (!skip && px == px && py == py && px != -INFINITY && px != INFINITY && py != -INFINITY && py != INFINITY) {
              transFormationRequired = true;

              if (sourcePixelX == sourcePixelX && sourcePixelY == sourcePixelY && sourcePixelX != -INFINITY && sourcePixelX != INFINITY && sourcePixelY != -INFINITY && sourcePixelY != INFINITY) {

                if (firstExtent) {
                  PXExtentBasedOnSource[0] = int(sourcePixelX);
                  PXExtentBasedOnSource[1] = int(sourcePixelY);
                  PXExtentBasedOnSource[2] = int(sourcePixelX);
                  PXExtentBasedOnSource[3] = int(sourcePixelY);
                  firstExtent = false;
                } else {
                  if (sourcePixelX < PXExtentBasedOnSource[0]) PXExtentBasedOnSource[0] = sourcePixelX;
                  if (sourcePixelX > PXExtentBasedOnSource[2]) PXExtentBasedOnSource[2] = sourcePixelX;
                  if (sourcePixelY < PXExtentBasedOnSource[1]) {
                    PXExtentBasedOnSource[1] = sourcePixelY;
                  }
                  if (sourcePixelY > PXExtentBasedOnSource[3]) {
                    PXExtentBasedOnSource[3] = sourcePixelY;
                  }
                }
              }

            } else {

              if (OK == true && fullScan == false) {
                OK = false;
                fullScan = true;
              }
            }
          }
        }
      }

      if (OK == true && fullScan == true) break;
    }
  }

#ifdef GenericDataWarper_DEBUG
  CDBDebug("PXExtentBasedOnSource = [%d,%d,%d,%d]", PXExtentBasedOnSource[0], PXExtentBasedOnSource[1], PXExtentBasedOnSource[2], PXExtentBasedOnSource[3]);

#endif
  if (PXExtentBasedOnSource[1] > PXExtentBasedOnSource[3]) {
    std::swap(PXExtentBasedOnSource[1], PXExtentBasedOnSource[3]);
  }
  if (PXExtentBasedOnSource[0] > PXExtentBasedOnSource[2]) {
    std::swap(PXExtentBasedOnSource[0], PXExtentBasedOnSource[2]);
  }
  PXExtentBasedOnSource[2] += 1;
  PXExtentBasedOnSource[3] += 1;

#ifdef GenericDataWarper_DEBUG
  CDBDebug("PXExtentBasedOnSource = [%d,%d,%d,%d]", PXExtentBasedOnSource[0], PXExtentBasedOnSource[1], PXExtentBasedOnSource[2], PXExtentBasedOnSource[3]);
#endif

  if (PXExtentBasedOnSource[0] < 0) {
    PXExtentBasedOnSource[0] = 0;
  }
  if (PXExtentBasedOnSource[0] >= sourceDataWidth) {
    PXExtentBasedOnSource[0] = sourceDataWidth;
  }
  if (PXExtentBasedOnSource[2] < 0) {
    PXExtentBasedOnSource[2] = 0;
  }
  if (PXExtentBasedOnSource[2] >= sourceDataWidth) {
    PXExtentBasedOnSource[2] = sourceDataWidth;
  }
  if (PXExtentBasedOnSource[1] < 0) {
    PXExtentBasedOnSource[1] = 0;
  }
  if (PXExtentBasedOnSource[1] >= sourceDataHeight) {
    PXExtentBasedOnSource[1] = sourceDataHeight;
  }
  if (PXExtentBasedOnSource[3] < 0) {
    PXExtentBasedOnSource[3] = 0;
  }
  if (PXExtentBasedOnSource[3] >= sourceDataHeight) {
    PXExtentBasedOnSource[3] = sourceDataHeight;
  }
  if (transFormationRequired == false) {
    PXExtentBasedOnSource[0] = -1;
    PXExtentBasedOnSource[1] = -1;
    PXExtentBasedOnSource[2] = -1;
    PXExtentBasedOnSource[3] = -1;
  }

  return 0;
}
