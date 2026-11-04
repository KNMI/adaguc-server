#include "trajectory.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

TrajectoryPoint regularTrajectoryAxis(const double *values, size_t count) {
  if (values == nullptr || count < 2) throw std::runtime_error("Trajectory requires grid axes with at least two cells");
  // Estimate spacing across the whole axis to tolerate float32 rounding.
  const double step = (values[count - 1] - values[0]) / double(count - 1);
  if (!std::isfinite(step) || step == 0) throw std::runtime_error("Invalid trajectory grid spacing");
  for (size_t i = 0; i < count; ++i) {
    if (!std::isfinite(values[i]) || std::abs(values[i] - (values[0] + i * step)) > std::abs(step) * 1e-4) throw std::runtime_error("Trajectory requires regularly spaced grid axes");
  }
  return TrajectoryPoint(values[0], step);
}

// Edges belong to the positive-index side; corner-only touches select no extra cells.
void traverseTrajectoryCells(TrajectoryPoint a, TrajectoryPoint b, int width, int height, std::vector<TrajectoryCell> &cells) {
  double enter = 0, leave = 1;
  const double dx = b.first - a.first, dy = b.second - a.second;
  auto clip = [&](double origin, double delta, double extent) {
    if (delta == 0) return origin >= 0 && origin < extent;
    double lo = -origin / delta, hi = (extent - origin) / delta;
    if (lo > hi) std::swap(lo, hi);
    enter = std::max(enter, lo);
    leave = std::min(leave, hi);
    return enter <= leave;
  };
  // Clip first so traversal only visits the part inside the grid.
  if (!clip(a.first, dx, width) || !clip(a.second, dy, height)) return;
  auto add = [&](double t) {
    const double x = a.first + t * dx, y = a.second + t * dy;
    if (!(x >= 0 && x < width && y >= 0 && y < height)) return;
    const TrajectoryCell cell{int(std::floor(x)), int(std::floor(y))};
    // Suppress consecutive duplicates, but keep later revisits.
    if (cells.empty() || cells.back() != cell) cells.push_back(cell);
    if (cells.size() > MAX_TRAJECTORY_SAMPLES) throw std::runtime_error("Too many trajectory samples");
  };
  const int stepX = (dx > 0) - (dx < 0), stepY = (dy > 0) - (dy < 0);
  auto firstEdge = [&](double origin, double delta, int extent) {
    const double start = origin + enter * delta;
    return delta > 0 ? std::max(1, int(std::floor(start)) + 1) : std::min(extent - 1, int(std::ceil(start)) - 1);
  };
  int edgeX = firstEdge(a.first, dx, width), edgeY = firstEdge(a.second, dy, height);
  auto crossing = [](double origin, double delta, int edge, int extent) { return delta != 0 && edge > 0 && edge < extent ? (edge - origin) / delta : std::numeric_limits<double>::infinity(); };
  // Walk directly to the next cell boundary; no crossing list or sorting is needed.
  while (enter < leave) {
    const double tx = crossing(a.first, dx, edgeX, width), ty = crossing(a.second, dy, edgeY, height);
    const double next = std::min({tx, ty, leave});
    if (next - enter > 1e-12) add((enter + next) / 2);
    // Both axes advance at an exact corner; rounding slivers emit no extra cell.
    if (tx <= next) edgeX += stepX;
    if (ty <= next) edgeY += stepY;
    enter = std::max(enter, next);
  }
}
