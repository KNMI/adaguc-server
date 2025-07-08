#include "CDebugger.h"
#include "maketiles.h"
#include "CTString.h"
#include "CDataReader.h"
#include "CNetCDFDataWriter.h"
#include "CRequest.h"

DEF_ERRORMAIN()

struct DestinationGrids {
  int level, x, y;
  double bbox[4];
};

int main() {
  CT::string filename = "/data/adaguc-data/SAT/satcomp/MTG-FCI-FD_eur_atlantic_1km_true_color_202507031320.png";

  CDataReader reader;

  auto layerCOnfig = new CServerConfig::XMLE_Layer();
  auto filePath = new CServerConfig::XMLE_FilePath();
  auto variable = new CServerConfig::XMLE_Variable();
  auto db = new CServerConfig::XMLE_DataBase();
  auto cfg = new CServerConfig::XMLE_Configuration();
  auto tmp = new CServerConfig::XMLE_TempDir();

  tmp->value = "/tmp";
  filePath->value = filename;
  variable->value = "pngdata";
  layerCOnfig->FilePath.push_back(filePath);
  layerCOnfig->Variable.push_back(variable);
  db->attr.parameters = "user=adaguc password=adaguc host=localhost dbname=adaguc";
  cfg->DataBase.push_back(db);
  cfg->TempDir.push_back(tmp);

  CServerParams srvParams;
  srvParams.cfg = cfg;

  CDataSource dataSource;
  dataSource.setCFGLayer(&srvParams, cfg, layerCOnfig, "test", 0);

  CCDFDims dims;
  dims.addDimension("time", "2025-07-03T13:20:00Z", 0);
  dataSource.addStep(filename.c_str(), &dims);

  CDBDebug("dataSource filename %s", dataSource.getFileName());

  reader.open(&dataSource, CNETCDFREADER_MODE_OPEN_HEADER);

  CDBDebug("dataSource filename %s", dataSource.getFileName());

  COGCDims *ogcDim = new COGCDims();
  dataSource.requiredDims.push_back(ogcDim);
  ogcDim->name.copy("time");
  ogcDim->netCDFDimName = "time";
  ogcDim->uniqueValues = {dataSource.getDimensionValue(0)};

  //   CDBDebug("%s", CDF::dump(dataSource.getFirstAvailableDataObject()->cdfObject).c_str());

  dataSource.setTimeStep(0);
  CDBDebug("dataSource filename %s", dataSource.getFileName());
  srvParams.Geo->BBOX_CRS = " +proj=geos +lon_0=0 +h=35785831 +x_0=0 +y_0=0 +a=6378169 +rf=295.488065897001 +units=m +no_defs +type=crs";
  srvParams.Geo->CRS = srvParams.Geo->BBOX_CRS;
  srvParams.WCS_GoNative = false;

  std::vector<DestinationGrids> destinationGrids;
  int desiredWidth = 1024;
  int desiredHeight = 1024;
  double spanX = dataSource.dfBBOX[2] - dataSource.dfBBOX[0];
  double spanY = dataSource.dfBBOX[3] - dataSource.dfBBOX[1];
  double cellSizeX = fabs(dataSource.dfCellSizeX);
  double cellSizeY = fabs(dataSource.dfCellSizeY);
  for (int level = 1; level < 5; level++) {
    int inc = pow(2, level - 1);
    double newSpanX = desiredWidth * cellSizeX * inc;
    double newSpanY = desiredHeight * cellSizeY * inc;
    // CDBDebug("%d %d %f %f %f %f", level, inc, spanX, newSpanX, dataSource.dfBBOX[0], dataSource.dfBBOX[0] + newSpanX);
    for (double x = dataSource.dfBBOX[0]; x < dataSource.dfBBOX[2]; x += newSpanX) {
      for (double y = dataSource.dfBBOX[1]; y < dataSource.dfBBOX[3]; y += newSpanY) {

        int dx = round((x - dataSource.dfBBOX[0]) / newSpanX);
        int dy = round((y - dataSource.dfBBOX[1]) / newSpanY);
        CDBDebug("%d %d %d %f %f", level, dx, dy, x, y);
        destinationGrids.push_back({.level = level, .x = dx, .y = dy, .bbox = {x, y, x + newSpanX, y + newSpanY}});
      }
    }
  }
  for (auto destGrid : destinationGrids) {
    for (size_t b = 0; b < 4; b++) {
      srvParams.Geo->dfBBOX[b] = destGrid.bbox[b];
    }
    srvParams.Geo->dWidth = desiredWidth;
    srvParams.Geo->dHeight = desiredHeight;

    CNetCDFDataWriter wcsWriter;
    wcsWriter.init(&srvParams, &dataSource, dataSource.getNumTimeSteps());
    std::vector<CDataSource *> dataSources = {&dataSource};
    wcsWriter.addData(dataSources);
    CT::string destFileName;
    destFileName.print("/data/adaguc-autowms/tiles/tile%0.3d_%0.3d_%0.3d.nc", destGrid.level, destGrid.y, destGrid.x);
    CDBDebug("Writing %s", destFileName.c_str());
    wcsWriter.writeFile(destFileName.c_str(), destGrid.level, false);
  }
}