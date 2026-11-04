#ifndef ADAGUC_TRAJECTORY_H
#define ADAGUC_TRAJECTORY_H

#include <cstddef>
#include <utility>
#include <vector>

using TrajectoryPoint = std::pair<double, double>;
using TrajectoryCell = std::pair<int, int>;
constexpr std::size_t MAX_TRAJECTORY_SAMPLES = 10000;

// Return the origin and spacing of a regular axis, tolerating float32 rounding.
TrajectoryPoint regularTrajectoryAxis(const double *values, std::size_t count);

// Append crossed cells in path order, suppressing consecutive duplicates.
void traverseTrajectoryCells(TrajectoryPoint a, TrajectoryPoint b, int width, int height, std::vector<TrajectoryCell> &cells);
#endif
