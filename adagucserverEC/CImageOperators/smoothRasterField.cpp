#include "smoothRasterField.h"
#include <cstddef>
#include <math.h>

float *smoothingMakeDistanceMatrix(int smoothWindowSize) {
  if (smoothWindowSize == 0) {
    return nullptr;
  }
  float *distanceMatrix = new float((smoothWindowSize + 1) * 2 * (smoothWindowSize + 1) * 2);
  float distanceAmmount = 0;
  int dWinP = 0;
  for (int y1 = -smoothWindowSize; y1 < smoothWindowSize + 1; y1++) {
    for (int x1 = -smoothWindowSize; x1 < smoothWindowSize + 1; x1++) {
      float d = sqrt(x1 * x1 + y1 * y1);
      d = 1 / (d + 1);
      distanceMatrix[dWinP++] = d;
      distanceAmmount += d;
    }
  }
  return distanceMatrix;
}

float smoothingAtLocation(float *inputGrid, float *distanceMatrix, int smoothWindowSize, float fNodataValue, int gridLocationX, int gridLocationY, int gridWidth, int gridHeight) {
  if (smoothWindowSize == 0 || distanceMatrix == nullptr) {
    return fNodataValue;
  }
  size_t p = size_t(gridLocationX + gridLocationY * gridWidth);
  if (inputGrid[p] == fNodataValue) {
    return fNodataValue;
  }
  float distanceAmmount = 0;
  int dWinP = 0;
  float resultValue = 0;
  for (int y1 = -smoothWindowSize; y1 < smoothWindowSize + 1; y1++) {
    size_t yp = y1 * gridWidth;
    for (int x1 = -smoothWindowSize; x1 < smoothWindowSize + 1; x1++) {
      if (x1 + gridLocationX < gridWidth && y1 + gridLocationY < gridHeight && x1 + gridLocationX >= 0 && y1 + gridLocationY >= 0) {
        float val = inputGrid[p + x1 + yp];
        if (val != fNodataValue) {
          float d = distanceMatrix[dWinP];
          distanceAmmount += d;
          resultValue += val * d;
        }
      }
      dWinP++;
    }
  }
  if (distanceAmmount > 0) resultValue /= distanceAmmount;
  return resultValue;
}

void smoothRasterField(float *inputGrid, float fNodataValue, int smoothWindowSize, int W, int H) {
  if (smoothWindowSize == 0) return; // No smoothing.
  size_t drawImageSize = W * H;
  float *resultGrid = new float[W * H];
  float *distanceMatrix = smoothingMakeDistanceMatrix(smoothWindowSize);
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      resultGrid[x + y * W] = smoothingAtLocation(inputGrid, distanceMatrix, smoothWindowSize, fNodataValue, x, y, W, H);
    }
  }
  for (size_t p = 0; p < drawImageSize; p++) {
    inputGrid[p] = resultGrid[p];
  }
  delete[] distanceMatrix;
  delete[] resultGrid;
}
