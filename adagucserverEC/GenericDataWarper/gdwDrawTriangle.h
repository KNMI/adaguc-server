#ifndef GDW_DRAWTRIANGLE_UTILS_H
#define GDW_DRAWTRIANGLE_UTILS_H

#include "CGenericDataWarper.h"

template <typename T>
int gdwDrawTriangle(const double xCoords[3], const double yCoords[3], const T &value, bool tUp, GDWState &warperState, const std::function<void(int, int, T, GDWState &warperState)> &drawFunction);

#endif
