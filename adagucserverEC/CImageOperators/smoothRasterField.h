
#ifndef SMOOTHRASTERFIELD_H

float *smoothingMakeDistanceMatrix(int smoothWindowSize);
float smoothingAtLocation(float *inputGrid, float *distanceMatrix, int smoothWindowSize, float fNodataValue, int gridLocationX, int gridLocationY, int gridWidth, int gridHeight);
void smoothRasterField(float *valueData, float fNodataValue, int smoothWindow, int W, int H);

#endif // !SMOOTHRASTERFIELD_H
