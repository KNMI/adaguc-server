#ifndef GDW_DRAWTRIANGLE_UTILS_H
#define GDW_DRAWTRIANGLE_UTILS_H

#include "CGenericDataWarper.h"

template <typename T> int gdwDrawTriangle(double *_xP, double *_yP, T value, bool aOrB, GDWState &warperState, const std::function<void(int, int, T, GDWState &warperState)> &drawFunction);

#endif
