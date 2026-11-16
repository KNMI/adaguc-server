#include "trajectory.h"
#include "CDebugger.h"

#include <algorithm>
#include <cmath>
#include <limits>

std::pair<double, double> regularTrajectoryAxis(const double *values, size_t count, bool floatAxis) {
  if (values == nullptr || count < 2) {
    CDBError("Trajectory requires grid axes with at least two cells");
    throw(__LINE__);
  }
  // Estimate spacing across the whole axis to tolerate float32 rounding.
  const double step = (values[count - 1] - values[0]) / double(count - 1);
  if (!std::isfinite(step) || step == 0) {
    CDBError("Invalid trajectory grid spacing");
    throw(__LINE__);
  }
  // Reading as double retains the source axis's float32 rounding.
  const double magnitude = std::max(std::abs(values[0]), std::abs(values[count - 1]));
  const double precision = floatAxis ? std::numeric_limits<float>::epsilon() : std::numeric_limits<double>::epsilon();
  const double tolerance = std::max(std::abs(step) * 1e-4, 2 * precision * magnitude);
  for (size_t i = 0; i < count; ++i) {
    if (!std::isfinite(values[i]) || std::abs(values[i] - (values[0] + i * step)) > tolerance) {
      CDBError("Trajectory requires regularly spaced grid axes");
      throw(__LINE__);
    }
  }
  return {values[0], step};
}

// Edges belong to the positive-index side; corner-only touches select no extra cells.
void traverseTrajectoryCells(f8point a, f8point b, int width, int height, std::vector<i4point> &cells, double xPeriod) {
  if (xPeriod > 0) {
    // Visit shifted copies in path order so a longitude seam never becomes a long segment.
    const int first = int(std::ceil(-std::max(a.x, b.x) / xPeriod));
    const int last = int(std::floor((width - std::min(a.x, b.x)) / xPeriod));
    for (int i = first; i <= last; ++i) {
      const double shift = (b.x >= a.x ? last - (i - first) : i) * xPeriod;
      traverseTrajectoryCells({a.x + shift, a.y}, {b.x + shift, b.y}, width, height, cells);
    }
    return;
  }
  double enter = 0, leave = 1;
  const double dx = b.x - a.x, dy = b.y - a.y;
  auto clip = [&](double origin, double delta, double extent) {
    if (delta == 0) return origin >= 0 && origin < extent;
    double lo = -origin / delta, hi = (extent - origin) / delta;
    if (lo > hi) std::swap(lo, hi);
    enter = std::max(enter, lo);
    leave = std::min(leave, hi);
    return enter <= leave;
  };
  // Clip first so traversal only visits the part inside the grid.
  if (!clip(a.x, dx, width) || !clip(a.y, dy, height)) return;
  auto add = [&](double t) {
    const double x = a.x + t * dx, y = a.y + t * dy;
    if (!(x >= 0 && x < width && y >= 0 && y < height)) return;
    const i4point cell{int(std::floor(x)), int(std::floor(y))};
    // Suppress consecutive duplicates, but keep later revisits.
    if (cells.empty() || cells.back() != cell) cells.push_back(cell);
    if (cells.size() > MAX_TRAJECTORY_SAMPLES) {
      CDBError("Too many trajectory samples");
      throw(__LINE__);
    }
  };
  const int stepX = (dx > 0) - (dx < 0), stepY = (dy > 0) - (dy < 0);
  auto firstEdge = [&](double origin, double delta, int extent) {
    const double start = origin + enter * delta;
    return delta > 0 ? std::max(1, int(std::floor(start)) + 1) : std::min(extent - 1, int(std::ceil(start)) - 1);
  };
  int edgeX = firstEdge(a.x, dx, width), edgeY = firstEdge(a.y, dy, height);
  auto crossing = [](double origin, double delta, int edge, int extent) { return delta != 0 && edge > 0 && edge < extent ? (edge - origin) / delta : std::numeric_limits<double>::infinity(); };
  // Walk directly to the next cell boundary; no crossing list or sorting is needed.
  while (enter < leave) {
    const double tx = crossing(a.x, dx, edgeX, width), ty = crossing(a.y, dy, edgeY, height);
    const double next = std::min({tx, ty, leave});
    if (next - enter > 1e-12) add((enter + next) / 2);
    // Both axes advance at an exact corner; rounding slivers emit no extra cell.
    if (tx <= next) edgeX += stepX;
    if (ty <= next) edgeY += stepY;
    enter = std::max(enter, next);
  }
}
