#include "trajectorySampling.h"

#include "CDataReader.h"
#include "CDataSource.h"
#include "CImageWarper.h"
#include "CDFObjectStore.h"
#include "CServerError.h"
#include "utils/projectionUtils.h"
#include <algorithm>
#include <cmath>
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
  if (source->formatConverterActive || source->getNumDataObjects() != 1) {
    CDBError("Trajectory requires a regular grid with one source variable per layer");
    throw(__LINE__);
  }
  auto axis = [](CDF::Variable *var) {
    if (var == nullptr || var->dimensionlinks.size() != 1) {
      CDBError("Trajectory requires regular one-dimensional grid axes with at least two cells");
      throw(__LINE__);
    }
    return regularTrajectoryAxis(static_cast<double *>(var->data), var->getSize(), var->getNativeType() == CDF_FLOAT);
  };
  const auto [x0, sx] = axis(source->varX);
  const auto [y0, sy] = axis(source->varY);
  // The legacy header reader treats the final two variable dimensions as spatial.
  for (const auto &dim: source->requiredDims) {
    if (dim.netCDFDimName == source->varX->name || dim.netCDFDimName == source->varY->name) {
      CDBError("Trajectory requires spatial dimensions in the final two variable positions");
      throw(__LINE__);
    }
  }
  const int width = source->varX->getSize(), height = source->varY->getSize();
  GeoParameters geo;
  geo.crs = "EPSG:4326";
  if (warper->initreproj(source, geo, &source->srvParams->cfg->Projection) != 0) {
    CDBError("Could not initialize trajectory projection");
    throw(__LINE__);
  }
  // Release projection state even if sampling throws.
  struct Close {
    CImageWarper *warper;
    ~Close() { warper->closereproj(); }
  } close{warper};
  TrajectorySampling result;
  result.referenceGrid = snapshotTrajectoryGrid(source);
  auto project = [&](f8point p) {
    if (warper->reprojpoint(p.x, p.y) != 0 || !std::isfinite(p.x) || !std::isfinite(p.y)) {
      CDBError("Could not project trajectory coordinate");
      throw(__LINE__);
    }
    // Place cell boundaries at integers and centres at i + 0.5.
    p.x = (p.x - x0) / sx + 0.5;
    p.y = (p.y - y0) / sy + 0.5;
    return p;
  };
  const double xPeriod = isLonLatProjection(source->nativeProj4) ? 360 / std::abs(sx) : 0;
  std::vector<i4point> cells;
  // Budget extra subdivision work separately from the accepted input vertices.
  size_t subdivisions = 0;
  std::function<void(f8point, f8point, f8point, f8point, int)> segment;
  segment = [&](f8point a, f8point b, f8point ga, f8point gb, int depth) {
    if (depth > 0 && ++subdivisions > MAX_TRAJECTORY_SAMPLES * 4) {
      CDBError("Trajectory is too complex");
      throw(__LINE__);
    }
    const f8point mid{(a.x + b.x) / 2, (a.y + b.y) / 2};
    const auto gm = project(mid);
    // Split curved or long segments before tracing their crossed cells.
    const double error = std::hypot(gm.x - (ga.x + gb.x) / 2, gm.y - (ga.y + gb.y) / 2);
    const double length = std::hypot(gb.x - ga.x, gb.y - ga.y);
    if (error > 0.001 || length > 32) {
      if (depth >= 24) {
        CDBError("Trajectory projection did not converge");
        throw(__LINE__);
      }
      segment(a, mid, ga, gm, depth + 1);
      segment(mid, b, gm, gb, depth + 1);
    } else {
      traverseTrajectoryCells(ga, gb, width, height, cells, xPeriod);
    }
  };
  // Repeated vertices must not introduce corner-only samples.
  auto vertices = source->srvParams->trajectoryCoords;
  vertices.erase(std::unique(vertices.begin(), vertices.end()), vertices.end());
  // A stationary trajectory is one zero-length segment for the same walker.
  if (vertices.size() == 1) vertices.push_back(vertices.front());
  for (size_t i = 1; i < vertices.size(); ++i) segment(vertices[i - 1], vertices[i], project(vertices[i - 1]), project(vertices[i]), 0);
  // Cache each centre once; (y,x) keys also group unique cells into row runs.
  std::map<std::pair<int, int>, f8point> cellCentres;
  for (const auto &cell: cells) {
    auto [entry, firstVisit] = cellCentres.try_emplace(std::pair{cell.y, cell.x});
    if (firstVisit) {
      auto &centre = entry->second;
      centre = {result.referenceGrid.x[cell.x], result.referenceGrid.y[cell.y]};
      if (warper->reprojpoint_inv(centre.x, centre.y) != 0 || !std::isfinite(centre.x) || !std::isfinite(centre.y)) {
        CDBError("Could not project trajectory cell centre");
        throw(__LINE__);
      }
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
  if (result.samples.size() > size_t(10000000) / std::max(1, source->getNumTimeSteps())) {
    CDBError("Trajectory selects too many values");
    throw(__LINE__);
  }
  return result;
}

void prepareTrajectoryFile(CDataReader &reader, CDataSource *source, int representativeStep, const TrajectorySampling &sampling) {
  // Header reads use the filename and dimensions of the current selection.
  source->setTimeStep(representativeStep);
  if (reader.open(source, CNETCDFREADER_MODE_OPEN_HEADER) != 0) {
    CDBError("Could not read trajectory grid");
    throw(__LINE__);
  }
  if (!(snapshotTrajectoryGrid(source) == sampling.referenceGrid)) {
    CDBError("Trajectory requires a consistent spatial grid across source files");
    throw(__LINE__);
  }
}
