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
    <RenderMethod>barbpoint</RenderMethod>
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

int CDPPointsFromGrid::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode != CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    return 0;
  }

  CDBDebug(CDATAPOSTPROCESSOR_PointsFromGrid_ID);

  CImageWarper warper;
  if (dataSource->srvParams->Geo->CRS.empty()) {
    dataSource->srvParams->Geo->CRS = "EPSG:4236";
  }
  warper.initreproj(dataSource, dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);

  CDBDebug("%f %f %f %f", dataSource->dfBBOX[0], dataSource->dfBBOX[1], dataSource->dfBBOX[2], dataSource->dfBBOX[3]);

  std::vector<f8point> pointListInModelCoords;
  std::vector<size_t> pointers;
  for (size_t y = 0; y < size_t(dataSource->dHeight); y = y + 10) {
    for (size_t x = 0; x < size_t(dataSource->dWidth); x = x + 10) {
      size_t p = x + y * dataSource->dWidth;
      double modelX = dataSource->dfCellSizeX * x + dataSource->dfBBOX[0] + dataSource->dfCellSizeX / 2;
      double modelY = dataSource->dfCellSizeY * y + dataSource->dfBBOX[3] + dataSource->dfCellSizeY / 2;
      pointListInModelCoords.push_back({.x = modelX, .y = modelY});
      pointers.push_back(p);
    }
  }

  auto pointsInLatLon = pointListInModelCoords;
  warper.reprojModelToLatLon(pointsInLatLon);
  auto pointsInScreen = pointsInLatLon;
  warper.reprojfromLatLon(pointsInScreen);

  int id = 0;
  for (auto con : proc->attr.select.splitToStack(",")) {
    for (size_t index = 0; index < pointListInModelCoords.size(); index++) {
      auto pixelCoord = pointsInScreen[index];
      pixelCoord.x = ((pixelCoord.x - dataSource->srvParams->Geo->dfBBOX[0]) / (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0])) * dataSource->srvParams->Geo->dWidth;
      pixelCoord.y = ((pixelCoord.y - dataSource->srvParams->Geo->dfBBOX[1]) / (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1])) * dataSource->srvParams->Geo->dHeight;

      auto ob = dataSource->getDataObjectByName(con.c_str());
      auto destob = dataSource->getDataObject(id);

      if (ob->cdfVariable->getType() != CDF_FLOAT) {
        CDBError("Can only work with CDF_FLOAT");
        throw __LINE__;
      }
      float *data = (float *)ob->cdfVariable->data;
      int px = pixelCoord.x;
      int py = pixelCoord.y;
      auto newPoint = PointDVWithLatLon(px, py, pointsInLatLon[index].x, pointsInLatLon[index].y, data[pointers[index]]);
      destob->points.push_back(newPoint);
    }
    id++;
  }

  return 0;
}
