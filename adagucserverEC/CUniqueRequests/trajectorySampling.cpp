#include "trajectorySampling.h"

#include "CDataReader.h"
#include "CDataSource.h"
#include "CImageWarper.h"
#include "CDFObjectStore.h"
#include "CServerError.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <functional>
#include <set>
#include <map>

// Check request-wide trajectory limits before any layer starts sampling.
int validateTrajectorySources(const std::vector<CDataSource *> &dataSources) {
  // The request-owned store retains these handles. Reject requests exceeding
  // its capacity rather than evicting and reopening files between layers.
  std::set<std::string> files;
  for (auto source: dataSources) {
    if (source == nullptr) return 1;
    for (const auto &variable: source->cfgLayer->Variable) {
      if (!variable.attr.orgname.empty()) {
        CDBError("Trajectory does not support per-layer source variable renaming");
        setExceptionType(ServiceExceptionType::UnprocessableEntity);
        return 1;
      }
    }
    for (const auto &step: source->timeSteps) files.insert(step.fileName);
    source->setTimeStep(0);
  }
  if (files.size() > size_t(CDFObjectStore::getCDFObjectStore()->getMaxNumberOfOpenObjects())) {
    CDBError("Trajectory selects too many source files");
    setExceptionType(ServiceExceptionType::UnprocessableEntity);
    return 1;
  }
  return 0;
}

static TrajectoryGrid snapshotTrajectoryGrid(CDataSource *source) {
  const auto x = static_cast<double *>(source->varX->data), y = static_cast<double *>(source->varY->data);
  return {source->nativeProj4, {x, x + source->varX->getSize()}, {y, y + source->varY->getSize()}};
}

TrajectorySampling sampleTrajectoryGrid(CDataSource *source, CImageWarper *warper) {
  if (source->formatConverterActive || source->getNumDataObjects() != 1) throw std::runtime_error("Trajectory requires a regular grid with one source variable per layer");
  auto axis = [](CDF::Variable *var) {
    if (var == nullptr || var->dimensionlinks.size() != 1) throw std::runtime_error("Trajectory requires regular one-dimensional grid axes with at least two cells");
    return regularTrajectoryAxis(static_cast<double *>(var->data), var->getSize());
  };
  const auto [x0, sx] = axis(source->varX);
  const auto [y0, sy] = axis(source->varY);
  // The legacy header reader treats the final two variable dimensions as spatial.
  for (const auto &dim: source->requiredDims) {
    if (dim.netCDFDimName == source->varX->name || dim.netCDFDimName == source->varY->name) throw std::runtime_error("Trajectory requires spatial dimensions in the final two variable positions");
  }
  const int width = source->varX->getSize(), height = source->varY->getSize();
  GeoParameters geo;
  geo.crs = "EPSG:4326";
  if (warper->initreproj(source, geo, &source->srvParams->cfg->Projection) != 0) throw std::runtime_error("Could not initialize trajectory projection");
  // Release projection state even if sampling throws.
  struct Close {
    CImageWarper *warper;
    ~Close() { warper->closereproj(); }
  } close{warper};
  TrajectorySampling result;
  result.referenceGrid = snapshotTrajectoryGrid(source);
  auto project = [&](TrajectoryPoint p) {
    if (warper->reprojpoint(p.first, p.second) != 0 || !std::isfinite(p.first) || !std::isfinite(p.second)) throw std::runtime_error("Could not project trajectory coordinate");
    // Place cell boundaries at integers and centres at i + 0.5.
    p.first = (p.first - x0) / sx + 0.5;
    p.second = (p.second - y0) / sy + 0.5;
    return p;
  };
  std::vector<TrajectoryCell> cells;
  size_t subdivisions = 0;
  std::function<void(TrajectoryPoint, TrajectoryPoint, TrajectoryPoint, TrajectoryPoint, int)> segment;
  segment = [&](TrajectoryPoint a, TrajectoryPoint b, TrajectoryPoint ga, TrajectoryPoint gb, int depth) {
    if (++subdivisions > MAX_TRAJECTORY_SAMPLES * 4) throw std::runtime_error("Trajectory is too complex");
    const TrajectoryPoint mid{(a.first + b.first) / 2, (a.second + b.second) / 2};
    const auto gm = project(mid);
    // Split curved or long segments before tracing their crossed cells.
    const double error = std::hypot(gm.first - (ga.first + gb.first) / 2, gm.second - (ga.second + gb.second) / 2);
    const double length = std::hypot(gb.first - ga.first, gb.second - ga.second);
    if (error > 0.001 || length > 32) {
      if (depth >= 24) throw std::runtime_error("Trajectory projection did not converge");
      segment(a, mid, ga, gm, depth + 1);
      segment(mid, b, gm, gb, depth + 1);
    } else {
      traverseTrajectoryCells(ga, gb, width, height, cells);
    }
  };
  // Repeated vertices must not introduce corner-only samples.
  auto vertices = source->srvParams->trajectoryCoords;
  vertices.erase(std::unique(vertices.begin(), vertices.end()), vertices.end());
  // A stationary trajectory is one zero-length segment for the same walker.
  if (vertices.size() == 1) vertices.push_back(vertices.front());
  for (size_t i = 1; i < vertices.size(); ++i) segment(vertices[i - 1], vertices[i], project(vertices[i - 1]), project(vertices[i]), 0);
  // Cache each centre once; (y,x) keys also group unique cells into row runs.
  std::map<std::pair<int, int>, TrajectoryPoint> cellCentres;
  for (const auto &cell: cells) {
    auto [entry, firstVisit] = cellCentres.try_emplace(std::pair{cell.second, cell.first});
    if (firstVisit) {
      auto &centre = entry->second;
      centre = {result.referenceGrid.x[cell.first], result.referenceGrid.y[cell.second]};
      if (warper->reprojpoint_inv(centre.first, centre.second) != 0 || !std::isfinite(centre.first) || !std::isfinite(centre.second))
        throw std::runtime_error("Could not project trajectory cell centre");
    }
    result.samples.push_back({cell, entry->second});
  }
  for (const auto &entry: cellCentres) {
    const auto [y, x] = entry.first;
    if (!result.runs.empty() && result.runs.back().y == y && result.runs.back().x + result.runs.back().count == x)
      ++result.runs.back().count;
    else
      result.runs.push_back({x, y, 1});
  }
  // Bound output size across all selected times and levels.
  if (result.samples.size() > size_t(10000000) / std::max(1, source->getNumTimeSteps())) throw std::runtime_error("Trajectory selects too many values");
  return result;
}

void prepareTrajectoryFile(CDataReader &reader, CDataSource *source, int representativeStep, const TrajectorySampling &sampling) {
  // Header reads use the filename and dimensions of the current selection.
  source->setTimeStep(representativeStep);
  if (reader.open(source, CNETCDFREADER_MODE_OPEN_HEADER) != 0) throw std::runtime_error("Could not read trajectory grid");
  if (!(snapshotTrajectoryGrid(source) == sampling.referenceGrid)) throw std::runtime_error("Trajectory requires a consistent spatial grid across source files");
}
