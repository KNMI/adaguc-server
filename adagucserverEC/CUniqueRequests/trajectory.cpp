#include "trajectory.h"

#include <algorithm>
#include <cmath>
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
  std::vector<double> cuts{enter, leave};
  auto crossings = [&](double origin, double delta, int extent) {
    if (delta == 0) return;
    const double lo = std::min(origin + enter * delta, origin + leave * delta);
    const double hi = std::max(origin + enter * delta, origin + leave * delta);
    for (int edge = std::max(1, int(std::ceil(lo))); edge <= std::min(extent - 1, int(std::floor(hi))); ++edge) {
      const double t = (edge - origin) / delta;
      if (t > enter && t < leave) cuts.push_back(t);
    }
  };
  crossings(a.first, dx, width);
  crossings(a.second, dy, height);
  std::sort(cuts.begin(), cuts.end());
  auto add = [&](double t) {
    const double x = a.first + t * dx, y = a.second + t * dy;
    if (!(x >= 0 && x < width && y >= 0 && y < height)) return;
    const TrajectoryCell cell{int(std::floor(x)), int(std::floor(y))};
    // Suppress consecutive duplicates, but keep later revisits.
    if (cells.empty() || cells.back() != cell) cells.push_back(cell);
    if (cells.size() > MAX_TRAJECTORY_SAMPLES) throw std::runtime_error("Too many trajectory samples");
  };
  if (dx == 0 && dy == 0) {
    add(enter);
    return;
  }
  // Each interval between crossings lies in one cell.
  for (size_t i = 1; i < cuts.size(); ++i) {
    // Ignore rounding slivers at corners.
    if (cuts[i] - cuts[i - 1] > 1e-12) add((cuts[i] + cuts[i - 1]) / 2);
  }
}
