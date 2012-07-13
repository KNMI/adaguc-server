#ifndef CFILLTRIANGLE_H
#define CFILLTRIANGLE_H
#include <math.h>

/**
 * Fills a triangle in a Gouraud way
 * @param data the data field to fill
 * @param values 3 values for the corners
 * @param W width of the field
 * @param W height of the field
 * @param xP 3 X corners
 * @param xP 3 Y corners
 */
void fillTriangle(float  *data, float  *values, int W,int H, int *xP,int *yP);
#endif
