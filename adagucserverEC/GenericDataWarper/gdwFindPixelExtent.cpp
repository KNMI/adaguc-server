#include <array>
#include <CGeoParams.h>
#include <CImageWarper.h>
int gdwFindPixelExtent(int *PXExtentBasedOnSource, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, CImageWarper *warper) {
  int sourceDataWidth = sourceGeoParams->dWidth;
  int sourceDataHeight = sourceGeoParams->dHeight;

  PXExtentBasedOnSource[0] = 0;
  PXExtentBasedOnSource[1] = 0;
  PXExtentBasedOnSource[2] = sourceDataWidth;
  PXExtentBasedOnSource[3] = sourceDataHeight;

  PXExtentBasedOnSource[0] = -1;
  PXExtentBasedOnSource[1] = -1;
  PXExtentBasedOnSource[2] = -1;
  PXExtentBasedOnSource[3] = -1;

  double dfSourceW = double(sourceGeoParams->dWidth);
  double dfSourceH = double(sourceGeoParams->dHeight);

  int imageHeight = destGeoParams->dHeight;
  int imageWidth = destGeoParams->dWidth;
  double dfDestW = double(destGeoParams->dWidth);
  double dfDestH = double(destGeoParams->dHeight);

  int lowerIndex = 1, higherIndex = 3;

  double dfSourceExtW = (sourceGeoParams->dfBBOX[2] - sourceGeoParams->dfBBOX[0]);
  double dfSourceOrigX = sourceGeoParams->dfBBOX[0];
  double dfSourceExtH = (sourceGeoParams->dfBBOX[lowerIndex] - sourceGeoParams->dfBBOX[higherIndex]);
  double dfSourceOrigY = sourceGeoParams->dfBBOX[higherIndex];

  double dfDestExtW = destGeoParams->dfBBOX[2] - destGeoParams->dfBBOX[0];
  double dfDestExtH = destGeoParams->dfBBOX[1] - destGeoParams->dfBBOX[3];
  double dfDestOrigX = destGeoParams->dfBBOX[0];
  double dfDestOrigY = destGeoParams->dfBBOX[3];

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
