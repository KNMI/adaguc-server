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

#ifndef CFILLTRIANGLE_H
#define CFILLTRIANGLE_H
#include <cmath>
#include <cstdlib>
/**
 * Fills a triangle in a Gouraud way
 * @param data the data field to fill
 * @param values 3 values for the corners
 * @param W width of the field
 * @param W height of the field
 * @param xP 3 X corners
 * @param xP 3 Y corners
 */
void fillTriangleGouraud(float *data, float *values, int W, int H, int *xP, int *yP);

/**
 * Fills a quad in a Gouraud way
 * @param data the data field to fill
 * @param values 4 values for the corners
 * @param W width of the field
 * @param W height of the field
 * @param xP 4 X corners
 * @param xP 4 Y corners
 */
void fillQuadGouraud(float *data, float *values, int W, int H, int *xP, int *yP);

void drawCircle(float *data, float value, int W, int H, int orgx, int orgy, int radius);

/**
 * Fills a triangle flat
 * @param data the data field to fill
 * @param value value for the triangle
 * @param W width of the field
 * @param W height of the field
 * @param xP 3 X corners
 * @param xP 3 Y corners
 */
void fillTriangleFlat(float *data, float value, int W, int H, int *xP, int *yP);

#endif
