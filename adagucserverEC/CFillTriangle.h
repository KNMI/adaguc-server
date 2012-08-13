#ifndef CFILLTRIANGLE_H
#define CFILLTRIANGLE_H
#include <math.h>
#include <stdlib.h>
/**
 * Fills a triangle in a Gouraud way
 * @param data the data field to fill
 * @param values 3 values for the corners
 * @param W width of the field
 * @param W height of the field
 * @param xP 3 X corners
 * @param xP 3 Y corners
 */
void fillTriangleGouraud(float  *data, float  *values, int W,int H, int *xP,int *yP);

/**
 * Fills a quad in a Gouraud way
 * @param data the data field to fill
 * @param values 4 values for the corners
 * @param W width of the field
 * @param W height of the field
 * @param xP 4 X corners
 * @param xP 4 Y corners
 */
void fillQuadGouraud(float  *data, float  *values, int W,int H, int *xP,int *yP);

void drawCircle(float *data,float value,int W,int H,int orgx,int orgy,int radius);

#endif
