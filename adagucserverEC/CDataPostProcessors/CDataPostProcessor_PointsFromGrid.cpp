#include <type_traits>
#include "CDataPostProcessor_PointsFromGrid.h"
#include <CImageWarper.h>

/************************/
/*      CDPPointsFromGrid     */
/************************/
const char *CDPPointsFromGrid::className = "CDPPointsFromGrid";

/**Example:
 *
  <Style name="windbarbs_kts_shaded_withbarbs">
    <Point symbol="adaguc_windbarb"  min="0" max="700" label="0-7 (&lt;3 bft)" fillcolor="#FFFFB4" outline="#FFFFFF"/>
    <DataPostProc algorithm="pointsfromgrid" select="speed_component,direction_component"/>
    <RenderMethod>barb</RenderMethod>
  </Style>


  <Layer type="database">
    <Name>wind__at_10m</Name>
    <Title>Wind 10m flags</Title>
    <Variable>x_wind__at_10m</Variable>
    <Variable>y_wind__at_10m</Variable>
    <FilePath filter="">{ADAGUC_PATH}data/datasets/harmonie_wind_grid_30x30.nc</FilePath>
    <Dimension name="time" units="ISO8601" default="min">time</Dimension>
    <Dimension name="forecast_reference_time" units="ISO8601">reference_time</Dimension>
    <Styles>windbarbs_kts_shaded_withbarbs</Styles>
    <DataPostProc algorithm="convert_uv_components"/>
  </Layer>
 */

const char *CDPPointsFromGrid::getId() { return CDATAPOSTPROCESSOR_PointsFromGrid_ID; }
int CDPPointsFromGrid::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_PointsFromGrid_ID)) {
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPointsFromGrid::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *, size_t) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  CDBError("Not implemented yet");
  return 1;
}

f8point getPixelCoordinateFromGetMapCoordinate(f8point in, CDataSource &dataSource) {
  f8point pixelCoord;
  pixelCoord.x =
      ((in.x - dataSource.srvParams->geoParams.bbox.left) / (dataSource.srvParams->geoParams.bbox.right - dataSource.srvParams->geoParams.bbox.left)) * dataSource.srvParams->geoParams.width;
  pixelCoord.y =
      ((in.y - dataSource.srvParams->geoParams.bbox.bottom) / (dataSource.srvParams->geoParams.bbox.top - dataSource.srvParams->geoParams.bbox.bottom)) * dataSource.srvParams->geoParams.height;
  return pixelCoord;
}

void getPixelCoordinateListFromGetMapCoordinateListInPlace(std::vector<f8point> &in, CDataSource &dataSource) {
  for (auto &p : in) {
    p = getPixelCoordinateFromGetMapCoordinate(p, dataSource);
  }
}

f8point getGetMapCoordinateFromPixelCoordinate(f8point in, CDataSource &dataSource) {
  f8point getmapCoord;
  getmapCoord.x = (in.x / dataSource.srvParams->geoParams.width) * (dataSource.srvParams->geoParams.bbox.right - dataSource.srvParams->geoParams.bbox.left) + dataSource.srvParams->geoParams.bbox.left;
  getmapCoord.y =
      (in.y / dataSource.srvParams->geoParams.height) * (dataSource.srvParams->geoParams.bbox.top - dataSource.srvParams->geoParams.bbox.bottom) + dataSource.srvParams->geoParams.bbox.bottom;
  return getmapCoord;
}

f8point getStrideFromGetMapLocation(CDataSource &dataSource, CImageWarper &warper, f8point pixelOffset) {

  // Calculate center of modelgrid
  f8point middlePointModelData = {.x = (dataSource.dfBBOX[0] + dataSource.dfBBOX[2]) / 2, .y = (dataSource.dfBBOX[1] + dataSource.dfBBOX[3]) / 2};
  f8point middlePoint = middlePointModelData;

  //  Convert to GetMap CRS coords
  warper.reprojpoint_inv(middlePointModelData);

  //  Convert to GetMap Pixel Coords
  f8point middleBoxInPixelCoords = getPixelCoordinateFromGetMapCoordinate(middlePointModelData, dataSource);

  f8point middleBoxInPixelCoordsX = middleBoxInPixelCoords;
  f8point middleBoxInPixelCoordsY = middleBoxInPixelCoords;

  // Add the desired pixel offset
  middleBoxInPixelCoordsX.x += pixelOffset.x;
  middleBoxInPixelCoordsY.y += pixelOffset.y;

  // Back to GetMap CRS coords
  auto getMapMiddleX = getGetMapCoordinateFromPixelCoordinate(middleBoxInPixelCoordsX, dataSource);
  auto getMapMiddleY = getGetMapCoordinateFromPixelCoordinate(middleBoxInPixelCoordsY, dataSource);

  // Back to model coords
  warper.reprojpoint(getMapMiddleX);
  warper.reprojpoint(getMapMiddleY);

  // Substract from model center, make it absolute and round up.
  return {.x = ceil(fabs((getMapMiddleX.x - middlePoint.x) / dataSource.dfCellSizeX)), .y = ceil(fabs((getMapMiddleY.y - middlePoint.y) / dataSource.dfCellSizeY))};
}

int CDPPointsFromGrid::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode != CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    return 0;
  }

  CDBDebug(CDATAPOSTPROCESSOR_PointsFromGrid_ID);

  CImageWarper warper;
  if (dataSource->srvParams->geoParams.crs.empty()) {
    dataSource->srvParams->geoParams.crs = "EPSG:4236";
  }
  warper.initreproj(dataSource, dataSource->srvParams->geoParams, &dataSource->srvParams->cfg->Projection);

  std::vector<f8point> pointListInModelCoords;
  std::vector<size_t> pointers;

  f8point pixelOffset = {.x = 1, .y = 1};
  if (!proc->attr.a.empty()) {
    auto value = proc->attr.a.toDouble();
    if (value < 1) value = 1;
    pixelOffset.x = value;
    pixelOffset.y = value;
  }

  auto striding = getStrideFromGetMapLocation((*dataSource), warper, pixelOffset);
  CDBDebug("striding %f %f", striding.x, striding.y);

  // TODO: Calculate the start/end indices which are inside the getmap request. E.g. prevent looping the whole modelfield.
  for (size_t y = 0; y < size_t(dataSource->dHeight); y = y + striding.y) {
    for (size_t x = 0; x < size_t(dataSource->dWidth); x = x + striding.x) {
      size_t p = x + y * dataSource->dWidth;
      double modelX = dataSource->dfCellSizeX * x + dataSource->dfBBOX[0] + dataSource->dfCellSizeX / 2;
      double modelY = dataSource->dfCellSizeY * y + dataSource->dfBBOX[3] + dataSource->dfCellSizeY / 2;
      pointListInModelCoords.push_back({.x = modelX, .y = modelY});
      pointers.push_back(p);
    }
  }

  // From model coordinates to lat/lon coordinates
  auto pointsInLatLon = pointListInModelCoords;
  warper.reprojModelToLatLon(pointsInLatLon);

  // From latlon coordinates to getmap projection coordinates
  auto pointsInScreen = pointsInLatLon;
  warper.reprojfromLatLon(pointsInScreen);

  // From getmap coordinates to pixel coordinates
  auto pointsInPixel = pointsInScreen;
  getPixelCoordinateListFromGetMapCoordinateListInPlace(pointsInPixel, (*dataSource));

  int id = 0; // TODO check if not grows besided number of available objects
  for (auto con : proc->attr.select.splitToStack(",")) {
    auto ob = dataSource->getDataObjectByName(con.c_str());
    if (ob == nullptr) {
      CDBWarning("Cannot select variable %s in datapostproc %s, skipping point rendering", con.c_str(), CDPPointsFromGrid::className);
      continue;
    }

    auto destob = dataSource->getDataObject(id);
    if (ob->cdfVariable->getType() != CDF_FLOAT) {
      CDBError("Can only work with CDF_FLOAT");
      throw __LINE__;
    }
    for (size_t index = 0; index < pointsInPixel.size(); index++) {
      auto pixelCoord = pointsInPixel[index];
      float *data = (float *)ob->cdfVariable->data;
      int px = pixelCoord.x;
      int py = pixelCoord.y;
      if (px < 0 || py < 0 || px > dataSource->srvParams->geoParams.width || py > dataSource->srvParams->geoParams.height) continue;
      auto newPoint = PointDVWithLatLon(px, py, pointsInLatLon[index].x, pointsInLatLon[index].y, data[pointers[index]]);
      destob->points.push_back(newPoint);
    }
    id++;
  }

  return 0;
}
