#ifndef ADAGUC_TRAJECTORY_H
#define ADAGUC_TRAJECTORY_H

#include "../Types/GeoParameters.h"
#include <cstddef>
#include <utility>
#include <vector>

constexpr std::size_t MAX_TRAJECTORY_SAMPLES = 10000;

// Return the origin and spacing of a regular axis, tolerating float32 rounding.
std::pair<double, double> regularTrajectoryAxis(const double *values, std::size_t count);

// Append crossed cells in path order, suppressing consecutive duplicates.
void traverseTrajectoryCells(f8point a, f8point b, int width, int height, std::vector<i4point> &cells);
#endif
