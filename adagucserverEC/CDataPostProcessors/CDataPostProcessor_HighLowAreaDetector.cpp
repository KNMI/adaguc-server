#include "CDataPostProcessor_HighLowAreaDetector.h"
#include <CImageWarper.h>
#include <utils/GeometryUtils.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <CImageOperators/smoothRasterField.h>
#include <CDebugger.h>

/************************/
/*  CDPPHighLowAreaDetector  */
/************************/

/**Example:
 *
  <Style name="highlow_areas">
    <DataPostProc algorithm="highlowareadetector"/>
  </Style>


  <Layer type="database">
    <Name>air_pressure_at_sea_level</Name>
    <Title>High/Low pressure areas</Title>
    <Variable>air_pressure_at_sea_level</Variable>
    <FilePath filter="">{ADAGUC_PATH}data/datasets/pressure-for-h-and-l-detection.nc</FilePath>
    <Dimension name="time" units="ISO8601" default="min">time</Dimension>
    <Styles>highlow_areas</Styles>
  </Layer>
 */

const char *CDPPHighLowAreaDetector::getId() { return CDATAPOSTPROCESSOR_HighLowAreaDetector_ID; }
int CDPPHighLowAreaDetector::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm == (CDATAPOSTPROCESSOR_HighLowAreaDetector_ID)) {
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPHighLowAreaDetector::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *, size_t) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  CDBError("Not implemented yet");
  return 1;
}

constexpr int smoothingRadius = 16;
constexpr int clusterRadius = 48;

// Check the center first so equal values do not cause a move.
const std::array<i4point, 9> searchWindow = {
    {{.x = 0, .y = 0}, {.x = -1, .y = 1}, {.x = 0, .y = 1}, {.x = 1, .y = 1}, {.x = 1, .y = 0}, {.x = 1, .y = -1}, {.x = 0, .y = -1}, {.x = -1, .y = -1}, {.x = -1, .y = 0}}};

/** Find a cell index. Only connected longitude edges wrap; other outside cells give -1. */
int getGridPointer(int x, int y, int w, int h, int wrapWidth) {
  if (y < 0 || y >= h) return -1;
  if (wrapWidth > 0) {
    x = (x % wrapWidth + wrapWidth) % wrapWidth;
  } else if (x < 0 || x >= w) {
    return -1;
  }
  return x + y * w;
}

/** Convert a cell center to screen pixels and latitude/longitude. */
PointDVWithLatLon projectGridPointToScreen(i4point p, CDataSource *dataSource, CImageWarper &warper, float pointValue) {
  f8point model = {.x = dataSource->dfCellSizeX * p.x + dataSource->dfBBOX[0] + dataSource->dfCellSizeX / 2, .y = dataSource->dfCellSizeY * p.y + dataSource->dfBBOX[3] + dataSource->dfCellSizeY / 2};

  // Go through latitude/longitude to reach the requested map projection.
  f8point lalo = model;
  warper.reprojModelToLatLon(lalo);

  auto pointsInScreen = lalo;
  warper.reprojfromLatLon(pointsInScreen);

  // Convert map coordinates to pixels.
  std::vector<f8point> pointsInPixel = {pointsInScreen};
  getPixelCoordinateListFromGetMapCoordinateListInPlace(pointsInPixel, (*dataSource));

  int px = pointsInPixel[0].x;
  int py = pointsInPixel[0].y;
  double lon = lalo.x;
  double lat = lalo.y;

  return PointDVWithLatLon(px, py, lon, lat, pointValue);
}

struct PointCluster {
  i4point location;
  double value;
};

/** Keep the strongest point in each nearby group. */
std::vector<PointCluster> clusterPoints(const std::vector<i4point> &walkedPoints, const std::vector<double> &data, int w, int h, int wrapWidth, int clusterRadius, bool preferLower) {
  std::vector<PointCluster> candidates;
  for (const auto &p: walkedPoints) {
    double value = data[getGridPointer(p.x, p.y, w, h, wrapWidth)];
    if (std::isfinite(value)) candidates.push_back({.location = p, .value = value});
  }
  // Check stronger points first; use position to break ties.
  std::ranges::sort(candidates, [preferLower](const PointCluster &a, const PointCluster &b) {
    if (a.value != b.value) return preferLower ? a.value < b.value : a.value > b.value;
    if (a.location.y != b.location.y) return a.location.y < b.location.y;
    return a.location.x < b.location.x;
  });

  // Store kept points in small areas to make nearby checks quick.
  const int bucketSize = clusterRadius + 1;
  auto bucketCoordinate = [bucketSize](int coordinate) { return coordinate / bucketSize - (coordinate % bucketSize < 0); };
  std::map<std::pair<int, int>, std::vector<i4point>> buckets;
  std::vector<PointCluster> pointClusters;
  for (const auto &candidate: candidates) {
    const auto &p = candidate.location;
    int bx = bucketCoordinate(p.x);
    int by = bucketCoordinate(p.y);
    // Only this area and its eight neighbors can contain a close point.
    auto hasNearbyPoint = [&](int candidateX) {
      int candidateBucket = bucketCoordinate(candidateX);
      for (int y = by - 1; y <= by + 1; y++) {
        for (int x = candidateBucket - 1; x <= candidateBucket + 1; x++) {
          auto bucket = buckets.find({x, y});
          if (bucket == buckets.end()) continue;
          if (std::ranges::any_of(bucket->second, [&](const i4point &accepted) { return std::abs(candidateX - accepted.x) <= clusterRadius && std::abs(p.y - accepted.y) <= clusterRadius; }))
            return true;
        }
      }
      return false;
    };
    bool nearby = hasNearbyPoint(p.x);
    // Also check across the longitude seam on global grids.
    if (wrapWidth > 0 && !nearby) nearby = hasNearbyPoint(p.x - wrapWidth) || hasNearbyPoint(p.x + wrapWidth);
    if (!nearby) {
      pointClusters.push_back(candidate);
      buckets[{bx, by}].push_back(p);
    }
  }
  return pointClusters;
}

/** Place starting points at regular grid intervals. */
std::vector<i4point> createGridSeeds(int w, int h, int stride) {
  std::vector<i4point> seeds;
  // TODO: Restrict seeds to the requested map extent.
  for (int y = 0; y < h; y += stride) {
    for (int x = 0; x < w; x += stride) {
      seeds.push_back({.x = x, .y = y});
    }
  }
  return seeds;
}

/** Move toward lower or higher values for up to 100 steps.
 * Return true if a value passed the pressure threshold, even if the walk hit the limit.
 * Pressure thresholds use hPa.
 */
bool walkToExtremum(i4point &point, const std::vector<double> &data, int w, int h, int wrapWidth, float threshold, bool preferLower) {
  // Keep positions inside the grid, including grids with a repeated last longitude.
  int start = getGridPointer(point.x, point.y, w, h, wrapWidth);
  point = {.x = start % w, .y = start / w};
  bool accepted = false;
  for (size_t i = 0; i < 100; i++) {
    std::array<double, 9> values;
    std::array<int, 9> indices;
    for (size_t j = 0; j < searchWindow.size(); j++) {
      indices[j] = getGridPointer(point.x + searchWindow[j].x, point.y + searchWindow[j].y, w, h, wrapWidth);
      // Outside neighbors cannot win the search.
      values[j] = indices[j] < 0 ? (preferLower ? INFINITY : -INFINITY) : data[indices[j]];
    }
    // Stop if the current value is outside the requested pressure range.
    if (!(preferLower ? values[0] < threshold : values[0] > threshold)) break;
    accepted = true;
    auto best = preferLower ? std::ranges::min_element(values) : std::ranges::max_element(values);
    size_t bestIndex = std::distance(values.begin(), best);
    // Stop when no neighbor is better than the current cell.
    if (bestIndex == 0) break;
    point.x = indices[bestIndex] % w;
    point.y = indices[bestIndex] / w;
  }
  return accepted;
}

/** Add each kept point to the map using its style value. */
void appendClusterPoints(const std::vector<PointCluster> &clusters, DataObject *destination, CDataSource *dataSource, CImageWarper &warper, float category) {
  for (const auto &cluster: clusters) {
    destination->points.push_back(projectGridPointToScreen(cluster.location, dataSource, warper, category));
  }
}

int CDPPHighLowAreaDetector::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode != CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    return 0;
  }

  CDBDebug(CDATAPOSTPROCESSOR_HighLowAreaDetector_ID);

  CImageWarper warper;
  if (dataSource->srvParams->geoParams.crs.empty()) {
    dataSource->srvParams->geoParams.crs = "EPSG:4236";
  }
  warper.initreproj(dataSource, dataSource->srvParams->geoParams, &dataSource->srvParams->cfg->Projection);

  // Attribute 'a' sets the spacing between starting points, in grid cells.
  int stride = 10;
  if (!proc->attr.a.empty()) {
    auto value = atof(proc->attr.a.c_str());
    if (value < 1) value = 1;
    stride = value;
  }
  const int wrapWidth = getLongitudeWrapWidth(*dataSource);
  auto gridIndexPoint = createGridSeeds((int)dataSource->dWidth, (int)dataSource->dHeight, stride);

  int id = 0; // TODO check if not grows besided number of available objects
  const auto selectItems = CT::split(proc->attr.select, ",");
  for (auto &con: selectItems) {
    auto ob = dataSource->getDataObjectByName(con.c_str());
    if (ob == nullptr) {
      CDBWarning("Cannot select variable %s in datapostproc PointsFromGrid, skipping point rendering", con.c_str());
      continue;
    }
    if (ob->cdfVariable == NULL) {
      CDBError("Dataobject %s has no variable", con.c_str());
      throw __LINE__;
    }

    auto destob = dataSource->getDataObject(id);
    if (ob->cdfVariable->getType() != CDF_FLOAT) {
      CDBError("Can only work with CDF_FLOAT");
      throw __LINE__;
    }
    float *orgVardata = (float *)ob->cdfVariable->data;

    const int w = (int)dataSource->dWidth;
    const int h = (int)dataSource->dHeight;
    // Smooth the field before looking for highs and lows.
    auto data = smoothRasterRegion<float>(orgVardata, w, h, {0, 0, w, h}, smoothingRadius, wrapWidth, ob->hasNodataValue, ob->hasNodataValue ? ob->dfNodataValue : NAN);
    std::vector<i4point> walkedLowPoints;
    std::vector<i4point> walkedHighPoints;
    for (const auto &seed: gridIndexPoint) {
      i4point pLowest = seed;
      i4point pHighest = seed;
      if (walkToExtremum(pLowest, data, w, h, wrapWidth, 1010, true)) walkedLowPoints.push_back(pLowest);
      if (walkToExtremum(pHighest, data, w, h, wrapWidth, 1030, false)) walkedHighPoints.push_back(pHighest);
    }
    // Several walks may reach the same high or low; keep nearby results together.
    auto lowClusters = clusterPoints(walkedLowPoints, data, w, h, wrapWidth, clusterRadius, true);
    auto highClusters = clusterPoints(walkedHighPoints, data, w, h, wrapWidth, clusterRadius, false);
    CDBDebug("Found %lu low clusters and %lu high clusters", lowClusters.size(), highClusters.size());

    // Styles use category 0 for lows and 2 for highs.
    appendClusterPoints(lowClusters, destob, dataSource, warper, 0);
    appendClusterPoints(highClusters, destob, dataSource, warper, 2);
    id++;
  }

  return 0;
}
