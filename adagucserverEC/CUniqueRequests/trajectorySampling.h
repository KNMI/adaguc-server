#ifndef ADAGUC_TRAJECTORY_SAMPLING_H
#define ADAGUC_TRAJECTORY_SAMPLING_H

#include "trajectory.h"
#include "CDataReader.h"
#include "CImageWarper.h"
#include <functional>
#include <set>

namespace trajectory {
  struct Sample {
    Cell cell;
    Point lonlat;
  };
  struct Run {
    int x, y, count;
  };
  struct Sampling {
    std::vector<Sample> samples;
    std::vector<Run> runs;
  };

  struct Grid {
    std::string projection;
    std::vector<double> x, y;
    bool operator==(const Grid &) const = default;
  };

  inline Grid grid(CDataSource *source) {
    const auto x = static_cast<double *>(source->varX->data), y = static_cast<double *>(source->varY->data);
    return {source->nativeProj4, {x, x + source->varX->getSize()}, {y, y + source->varY->getSize()}};
  }

  inline Sampling sampleGrid(CDataSource *source, CImageWarper *warper) {
    auto axis = [](CDF::Variable *var) {
      if (var == nullptr || var->data == nullptr || var->getSize() < 2 || var->dimensionlinks.size() != 1)
        throw UnsupportedRequest("Trajectory requires regular one-dimensional grid axes with at least two cells");
      return regularAxis(static_cast<double *>(var->data), var->getSize());
    };
    const auto [x0, sx] = axis(source->varX);
    const auto [y0, sy] = axis(source->varY);
    // The legacy header reader treats the final two variable dimensions as spatial.
    for (const auto &dim: source->requiredDims) {
      if (dim.netCDFDimName == source->varX->name || dim.netCDFDimName == source->varY->name)
        throw UnsupportedRequest("Trajectory requires spatial dimensions in the final two variable positions");
    }
    const int width = source->varX->getSize(), height = source->varY->getSize();
    GeoParameters geo;
    geo.crs = "EPSG:4326";
    if (warper->initreproj(source, geo, &source->srvParams->cfg->Projection) != 0)
      throw UnsupportedRequest("Could not initialize trajectory projection");
    struct Close {
      CImageWarper *warper;
      ~Close() { warper->closereproj(); }
    } close{warper};
    auto project = [&](Point p) {
      if (warper->reprojpoint(p.first, p.second) != 0 || !std::isfinite(p.first) || !std::isfinite(p.second))
        throw UnsupportedRequest("Could not project trajectory coordinate");
      p.first = (p.first - x0) / sx + 0.5;
      p.second = (p.second - y0) / sy + 0.5;
      return p;
    };
    std::vector<Cell> cells;
    size_t subdivisions = 0;
    std::function<void(Point, Point, Point, Point, int)> segment;
    segment = [&](Point a, Point b, Point ga, Point gb, int depth) {
      if (++subdivisions > maxSamples * 4)
        throw UnsupportedRequest("Trajectory is too complex");
      const Point mid{(a.first + b.first) / 2, (a.second + b.second) / 2};
      const auto gm = project(mid);
      // Bound curvature error in units of native cells. Also subdivide long
      // segments so a midpoint alone cannot hide a curved projected path.
      const double error = std::hypot(gm.first - (ga.first + gb.first) / 2, gm.second - (ga.second + gb.second) / 2);
      const double length = std::hypot(gb.first - ga.first, gb.second - ga.second);
      if (error > 0.001 || length > 32) {
        if (depth >= 24)
          throw UnsupportedRequest("Trajectory projection did not converge");
        segment(a, mid, ga, gm, depth + 1);
        segment(mid, b, gm, gb, depth + 1);
      } else {
        traverse(ga, gb, width, height, cells);
      }
    };
    auto vertices = parse(source->srvParams->trajectoryCoords);
    vertices.erase(std::unique(vertices.begin(), vertices.end()), vertices.end());
    if (vertices.size() == 1) {
      const auto point = project(vertices.front());
      traverse(point, point, width, height, cells);
    }
    for (size_t i = 1; i < vertices.size(); ++i)
      segment(vertices[i - 1], vertices[i], project(vertices[i - 1]), project(vertices[i]), 0);
    Sampling result;
    std::set<std::pair<int, int>> unique; // y first, for contiguous reads along x
    for (const auto &cell: cells) {
      Point centre{static_cast<double *>(source->varX->data)[cell.first], static_cast<double *>(source->varY->data)[cell.second]};
      if (warper->reprojpoint_inv(centre.first, centre.second) != 0 || !std::isfinite(centre.first) || !std::isfinite(centre.second))
        throw UnsupportedRequest("Could not project trajectory cell centre");
      result.samples.push_back({cell, centre});
      unique.emplace(cell.second, cell.first);
    }
    for (const auto &[y, x]: unique) {
      if (!result.runs.empty() && result.runs.back().y == y && result.runs.back().x + result.runs.back().count == x)
        ++result.runs.back().count;
      else
        result.runs.push_back({x, y, 1});
    }
    return result;
  }
} // namespace trajectory
#endif
