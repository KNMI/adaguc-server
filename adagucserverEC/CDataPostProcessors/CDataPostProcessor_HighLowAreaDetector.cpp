#include "CDataPostProcessor_HighLowAreaDetector.h"
#include <CImageWarper.h>
#include <utils/GeometryUtils.h>
#include <algorithm>
#include <array>
#include <cmath>

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

// Search window for xdir and ydir:
const i4point searchWindow[9] = {{.x = 0, .y = 0},  {.x = -1, .y = 1}, {.x = 0, .y = 1},   {.x = 1, .y = 1}, {.x = 1, .y = 0},
                                 {.x = 1, .y = -1}, {.x = 0, .y = -1}, {.x = -1, .y = -1}, {.x = -1, .y = 0}};

int getGridPointer(int x, int y, size_t w, size_t h) {
  int mx = x % (int)w;
  int my = y % (int)h;
  x = mx < 0 ? mx + (int)w : mx;
  y = my < 0 ? my + (int)h : my;
  return x + y * w;
}

PointDVWithLatLon projectGridPointToScreen(i4point p, CDataSource *dataSource, CImageWarper &warper, float pointValue) {
  f8point model = {.x = dataSource->dfCellSizeX * p.x + dataSource->dfBBOX[0] + dataSource->dfCellSizeX / 2, .y = dataSource->dfCellSizeY * p.y + dataSource->dfBBOX[3] + dataSource->dfCellSizeY / 2};

  f8point lalo = model;
  warper.reprojModelToLatLon(lalo);

  auto pointsInScreen = lalo;
  warper.reprojfromLatLon(pointsInScreen);

  // From getmap coordinates to pixel coordinates
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
  float value;
};

// Walked points that converged to the same low/high typically end up within
// a few grid cells of each other. Greedily merge points into clusters based on
// proximity to the cluster's location, keeping the location fixed at whichever
// walked point has the most extreme value instead of averaging positions.
std::vector<PointCluster> clusterPoints(const std::vector<i4point> &walkedPoints, float *data, CDataSource *dataSource, int clusterRadius, bool preferLower) {
  std::vector<PointCluster> pointClusters;
  for (const auto &p: walkedPoints) {
    float value = data[getGridPointer(p.x, p.y, dataSource->dWidth, dataSource->dHeight)];
    bool merged = false;
    for (auto &pointCluster: pointClusters) {
      if (std::abs(p.x - pointCluster.location.x) <= clusterRadius && std::abs(p.y - pointCluster.location.y) <= clusterRadius) {
        bool isBetter = preferLower ? (value < pointCluster.value) : (value > pointCluster.value);
        if (isBetter) {
          pointCluster.location = p;
          pointCluster.value = value;
        }
        merged = true;
        break;
      }
    }
    if (!merged) {
      pointClusters.push_back({.location = p, .value = value});
    }
  }
  return pointClusters;
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

  std::vector<i4point> gridIndexPoint;

  i4point striding = {.x = 10, .y = 10};
  if (!proc->attr.a.empty()) {
    auto value = atof(proc->attr.a.c_str());
    if (value < 1) value = 1;
    striding.x = value;
    striding.y = value;
  }

  // TODO: Calculate the start/end indices which are inside the getmap request. E.g. prevent looping the whole modelfield.
  for (int y = 0; y < (dataSource->dHeight); y = y + striding.y) {
    for (int x = 0; x < (dataSource->dWidth); x = x + striding.x) {

      gridIndexPoint.push_back({.x = x, .y = y});
    }
  }

  // gridIndexPoint.push_back({.x = 100, .y = 100});
  // // From model coordinates to lat/lon coordinates
  // auto pointsInLatLon = pointListInModelCoords;
  // warper.reprojModelToLatLon(pointsInLatLon);

  // // From latlon coordinates to getmap projection coordinates
  // auto pointsInScreen = pointsInLatLon;
  // warper.reprojfromLatLon(pointsInScreen);

  // // From getmap coordinates to pixel coordinates
  // auto pointsInPixel = pointsInScreen;
  // getPixelCoordinateListFromGetMapCoordinateListInPlace(pointsInPixel, (*dataSource));

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

    float *data = new float[dataSource->dWidth * dataSource->dHeight];
    std::fill_n(data, dataSource->dWidth * dataSource->dHeight, NAN);

    const int smoothWindow = 32;
    const int clusterRadius = 48;
    const int smoothHalf = smoothWindow / 2;
    const int w = (int)dataSource->dWidth;
    const int h = (int)dataSource->dHeight;

    // Separable box blur: a horizontal sliding-window sum followed by a vertical
    // sliding-window sum produces the same result as the naive W*H*window*window
    // convolution, but each pass only touches every cell a constant number of times,
    // i.e. O(W*H) instead of O(W*H*window*window).
    std::vector<double> rowSum(size_t(w) * size_t(h), 0.0);
    for (int y = 0; y < h; y++) {
      const float *rowData = orgVardata + size_t(y) * w;
      double sum = 0;
      for (int wx = -smoothHalf; wx < smoothHalf; wx++) {
        sum += rowData[smoothHalf + wx];
      }
      rowSum[size_t(smoothHalf) + size_t(y) * w] = sum;
      for (int x = smoothHalf + 1; x < w - smoothHalf; x++) {
        sum += rowData[x + smoothHalf - 1] - rowData[x - smoothHalf - 1];
        rowSum[size_t(x) + size_t(y) * w] = sum;
      }
    }

    for (int x = smoothHalf; x < w - smoothHalf; x++) {
      double sum = 0;
      for (int wy = -smoothHalf; wy < smoothHalf; wy++) {
        sum += rowSum[size_t(x) + size_t(smoothHalf + wy) * w];
      }
      data[size_t(x) + size_t(smoothHalf) * w] = sum / (smoothWindow * smoothWindow);
      for (int y = smoothHalf + 1; y < h - smoothHalf; y++) {
        sum += rowSum[size_t(x) + size_t(y + smoothHalf - 1) * w] - rowSum[size_t(x) + size_t(y - smoothHalf - 1) * w];
        data[size_t(x) + size_t(y) * w] = sum / (smoothWindow * smoothWindow);
      }
    }
    // size_t numCells = dataSource->dWidth * dataSource->dHeight;
    // for (size_t i = 0; i < numCells; i++) {
    //   orgVardata[i] = data[i];
    // }
    std::vector<i4point> walkedLowPoints;
    std::vector<i4point> walkedHighPoints;
    for (size_t index = 0; index < gridIndexPoint.size(); index++) {
      bool addLowest = false;
      bool addHighest = false;
      auto &pLowest = gridIndexPoint[index];
      i4point pHighest = pLowest; // Walk towards the local maximum from the same seed position

      for (size_t i = 0; i < 100; i++) {
        std::array<float, 9> pointWindowValues;

        for (size_t j = 0; j < 9; j++) {
          i4point newp = {.x = pLowest.x + searchWindow[j].x, .y = pLowest.y + searchWindow[j].y};
          pointWindowValues[j] = data[getGridPointer(newp.x, newp.y, dataSource->dWidth, dataSource->dHeight)];
        }
        if (pointWindowValues[0] < 1010)
          addLowest = true;
        else
          break;

        // find index of lowest value in pointWindowValues
        size_t lowestIndex = std::distance(pointWindowValues.begin(), std::min_element(pointWindowValues.begin(), pointWindowValues.end()));
        if (lowestIndex == 0) {
          break;
        }

        pLowest.x += searchWindow[lowestIndex].x;
        pLowest.y += searchWindow[lowestIndex].y;
      }

      for (size_t i = 0; i < 100; i++) {
        std::array<float, 9> pointWindowValues;

        for (size_t j = 0; j < 9; j++) {
          i4point newp = {.x = pHighest.x + searchWindow[j].x, .y = pHighest.y + searchWindow[j].y};
          pointWindowValues[j] = data[getGridPointer(newp.x, newp.y, dataSource->dWidth, dataSource->dHeight)];
        }
        if (pointWindowValues[0] > 1030)
          addHighest = true;
        else
          break;

        // find index of highest value in pointWindowValues
        size_t highestIndex = std::distance(pointWindowValues.begin(), std::max_element(pointWindowValues.begin(), pointWindowValues.end()));
        if (highestIndex == 0) {
          break;
        }

        pHighest.x += searchWindow[highestIndex].x;
        pHighest.y += searchWindow[highestIndex].y;
      }

      if (addLowest) {
        walkedLowPoints.push_back(pLowest);
      }
      if (addHighest) {
        walkedHighPoints.push_back(pHighest);
      }
    }
    // Points are now clustered, we need to find the main clusters.
    auto lowPointClusters = clusterPoints(walkedLowPoints, data, dataSource, clusterRadius, true);
    auto highPointClusters = clusterPoints(walkedHighPoints, data, dataSource, clusterRadius, false);

    std::vector<i4point> lowClusters;
    for (auto &pointCluster: lowPointClusters) {
      lowClusters.push_back(pointCluster.location);
    }

    std::vector<i4point> highClusters;
    for (auto &pointCluster: highPointClusters) {
      highClusters.push_back(pointCluster.location);
    }

    CDBDebug("Found %lu low clusters and %lu high clusters", lowClusters.size(), highClusters.size());
    // Add points to the dataobject

    for (const auto &p: lowClusters) {
      destob->points.push_back(projectGridPointToScreen(p, dataSource, warper, 0));
    }
    for (const auto &p: highClusters) {
      destob->points.push_back(projectGridPointToScreen(p, dataSource, warper, 2));
    }
    delete[] data;
    id++;
  }

  return 0;
}
