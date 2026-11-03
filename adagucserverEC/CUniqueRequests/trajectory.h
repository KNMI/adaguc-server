#ifndef ADAGUC_TRAJECTORY_H
#define ADAGUC_TRAJECTORY_H

#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace trajectory {
  // Expected unsupported-input failures are distinct from I/O and programming errors.
  class UnsupportedRequest : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };

  using Point = std::pair<double, double>;
  using Cell = std::pair<int, int>;
  constexpr size_t maxSamples = 100000;

  inline Point regularAxis(const double *values, size_t count) {
    if (values == nullptr || count < 2)
      throw UnsupportedRequest("Trajectory requires grid axes with at least two cells");
    // Coordinates stored as float32 have rounded intervals. Using just the
    // first interval accumulates its rounding error over the entire axis.
    const double step = (values[count - 1] - values[0]) / double(count - 1);
    if (!std::isfinite(step) || step == 0)
      throw UnsupportedRequest("Invalid trajectory grid spacing");
    for (size_t i = 0; i < count; ++i) {
      if (!std::isfinite(values[i]) || std::abs(values[i] - (values[0] + i * step)) > std::abs(step) * 1e-4)
        throw UnsupportedRequest("Trajectory requires regularly spaced grid axes");
    }
    return Point(values[0], step);
  }

  inline std::vector<Point> parse(const std::string &wkt) {
    const auto begin = wkt.find('('), end = wkt.find(')');
    if (begin == std::string::npos || end == std::string::npos)
      throw UnsupportedRequest("Expected a 2D LINESTRING");
    std::string kind = wkt.substr(0, begin);
    kind.erase(std::remove_if(kind.begin(), kind.end(), [](unsigned char c) { return std::isspace(c); }), kind.end());
    std::transform(kind.begin(), kind.end(), kind.begin(), [](unsigned char c) { return std::toupper(c); });
    if (kind != "LINESTRING" || wkt.find_first_not_of(" \t\r\n", end + 1) != std::string::npos)
      throw UnsupportedRequest("Expected a 2D LINESTRING");
    std::stringstream stream(wkt.substr(begin + 1, end - begin - 1));
    std::string vertex;
    std::vector<Point> points;
    while (std::getline(stream, vertex, ',')) {
      std::stringstream values(vertex);
      double x, y;
      if (!(values >> x) || !std::isspace(values.peek()) || !(values >> y) || !std::isfinite(x) || !std::isfinite(y))
        throw UnsupportedRequest("Invalid trajectory coordinate");
      values >> std::ws;
      if (!values.eof())
        throw UnsupportedRequest("Expected two ordinates per trajectory coordinate");
      points.emplace_back(x, y);
      if (points.size() > maxSamples)
        throw UnsupportedRequest("Too many trajectory vertices");
    }
    if (points.size() < 2 || wkt[end - 1] == ',')
      throw UnsupportedRequest("A trajectory needs at least two coordinates");
    return points;
  }

  inline void append(std::vector<Cell> &cells, int x, int y) {
    if (cells.empty() || cells.back() != Cell(x, y))
      cells.emplace_back(x, y);
    if (cells.size() > maxSamples)
      throw UnsupportedRequest("Too many trajectory samples");
  }

  // Cell coordinates have integer boundaries and centres at i + 0.5. Cells are
  // half-open: an edge belongs to its positive side; corner-only touches do not
  // select additional cells. Clip first so work depends on crossed cells, not on
  // the length of portions outside the dataset.
  inline void traverse(Point a, Point b, int width, int height, std::vector<Cell> &cells) {
    double enter = 0, leave = 1;
    const double dx = b.first - a.first, dy = b.second - a.second;
    auto clip = [&](double origin, double delta, double extent) {
      if (delta == 0)
        return origin >= 0 && origin < extent;
      double lo = -origin / delta, hi = (extent - origin) / delta;
      if (lo > hi)
        std::swap(lo, hi);
      enter = std::max(enter, lo);
      leave = std::min(leave, hi);
      return enter <= leave;
    };
    if (!clip(a.first, dx, width) || !clip(a.second, dy, height))
      return;
    std::vector<double> cuts{enter, leave};
    auto crossings = [&](double origin, double delta, int extent) {
      if (delta == 0)
        return;
      const double lo = std::min(origin + enter * delta, origin + leave * delta);
      const double hi = std::max(origin + enter * delta, origin + leave * delta);
      for (int edge = std::max(1, int(std::ceil(lo))); edge <= std::min(extent - 1, int(std::floor(hi))); ++edge) {
        const double t = (edge - origin) / delta;
        if (t > enter && t < leave)
          cuts.push_back(t);
      }
    };
    crossings(a.first, dx, width);
    crossings(a.second, dy, height);
    std::sort(cuts.begin(), cuts.end());
    auto add = [&](double t) {
      const double x = a.first + t * dx, y = a.second + t * dy;
      if (x >= 0 && x < width && y >= 0 && y < height)
        append(cells, int(std::floor(x)), int(std::floor(y)));
    };
    if (dx == 0 && dy == 0) {
      add(enter);
      return;
    }
    for (size_t i = 1; i < cuts.size(); ++i) {
      // Coincident x/y crossings can differ by rounding. Do not turn that
      // numerical sliver into an additional cell at a corner.
      if (cuts[i] - cuts[i - 1] > 1e-12)
        add((cuts[i] + cuts[i - 1]) / 2);
    }
  }
} // namespace trajectory
#endif
