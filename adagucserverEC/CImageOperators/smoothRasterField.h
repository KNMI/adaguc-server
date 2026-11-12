
#ifndef SMOOTHRASTERFIELD_H
#define SMOOTHRASTERFIELD_H

#include "GenericDataWarper/GDWDrawFunctionSettings.h"
#include "GenericDataWarper/GDWState.h"
#include <vector>

/** Average a grid region with a centered radius, skipping missing cells.
 * Edges are clipped vertically. wrapWidth is zero for regional grids, or the
 * number of unique longitude columns for a global grid. Empty windows give NaN.
 */
template <typename T> std::vector<double> smoothRasterRegion(const T *sourceGrid, int width, int height, i4box region, int radius, int wrapWidth, bool hasNodataValue, double nodataValue);

template <typename T> T smoothingAtLocation(int gridLocationX, int gridLocationY, T *inputGrid, GDWState &warperState, GDWDrawFunctionSettings &settings);

#endif // !SMOOTHRASTERFIELD_H
