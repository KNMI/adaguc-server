#include "CDataPostProcessor_SolarTerminator.h"
#include "solar/solar_terminator.h"
#include <chrono>
#include <ctime>
#include "CTime.h"
#include "CImageWarper.h"

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <cxxabi.h>
#include <dlfcn.h>

#include <time.h>

/************************/
/*      CDPPSolarTerminator  */
/************************/
const char *CDPPSolarTerminator::className = "CDPPSolarTerminator";

const char *CDPPSolarTerminator::getId() { return "solarterminator"; }

#define SIZE 255
void CDPPSolarTerminator::print_trace() {
  void *buffer[SIZE];
  char cmd[256];
  int nptrs;

  nptrs = backtrace(buffer, SIZE);
  backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO); // prints out raw stack trace

  for (int j = 0; j < nptrs; j++) {
    sprintf(cmd, "addr2line -e myprogram -f -C -i %p", buffer[j]); // replace 'myprogram' with your binary name
    system(cmd);
  }

  return;
}

int CDPPSolarTerminator::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("solarterminator")) {
    if (dataSource->getNumDataObjects() < 1 && mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
      CDBError("1 variable is needed for solarterminator, found %d", dataSource->getNumDataObjects());
      return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
    }
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPSolarTerminator::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  CDBDebug("Calling CDPPSolarTerminator::execute without a timestamp");
  return CDPPSolarTerminator::execute(proc, dataSource, mode, 0);
}

time_t strToEpochTimestamp(const char *timestampStr) {
  struct tm tmStruct;
  strptime(timestampStr, "%Y-%m-%dT%H:%M:%SZ", &tmStruct);
  return mktime(&tmStruct);
}

int CDPPSolarTerminator::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double timestamp) {
  if ((isApplicable(proc, dataSource, mode) & mode) == false) {
    return -1;
  }
  CDBDebug("Value of requested dim 0 is: %s", dataSource->srvParams->requestDims[0]->value.c_str());

  CT::string timestampStr = dataSource->srvParams->requestDims[0]->value.c_str();
  double currentOffset = strToEpochTimestamp(timestampStr);

  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    CT::string newVariableName = "SolT";

    CDataSource::DataObject *newDataObject = dataSource->getDataObject(0);
    newDataObject->variableName.copy(newVariableName.c_str());

    // Copy bounding box of screen
    auto *geo = dataSource->srvParams->Geo;
    double dfBBOX[] = {geo->dfBBOX[0], geo->dfBBOX[1], geo->dfBBOX[2], geo->dfBBOX[3]};
    size_t width = geo->dWidth;
    size_t height = geo->dHeight;

    dataSource->nativeProj4 = geo->CRS;
    dataSource->dWidth = geo->dWidth;
    dataSource->dHeight = geo->dHeight;
    dataSource->dfBBOX[0] = geo->dfBBOX[0];
    dataSource->dfBBOX[1] = geo->dfBBOX[1];
    dataSource->dfBBOX[2] = geo->dfBBOX[2];
    dataSource->dfBBOX[3] = geo->dfBBOX[3];

    // Create new dimensions and variables (X,Y,T)

    CDF::Dimension *dimX = new CDF::Dimension();
    dimX->name = "xet";
    dimX->setSize(width);
    newDataObject->cdfObject->addDimension(dimX);

    // Define the X variable using the X dimension
    CDF::Variable *varX = new CDF::Variable();
    varX->setType(CDF_DOUBLE);
    varX->name.copy("xet");
    varX->isDimension = true;
    varX->dimensionlinks.push_back(dimX);
    newDataObject->cdfObject->addVariable(varX);
    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);

    // Set the bbox in the data, since the virtual grid is 2x2 pixels we can directly apply the bbox
    ((double *)varX->data)[0] = dfBBOX[0];
    ((double *)varX->data)[1] = dfBBOX[2];

    // For y dimension
    CDF::Dimension *dimY = new CDF::Dimension();
    dimY->name = "yet";
    dimY->setSize(height);
    newDataObject->cdfObject->addDimension(dimY);

    // Define the Y variable using the X dimension
    CDF::Variable *varY = new CDF::Variable();
    varY->setType(CDF_DOUBLE);
    varY->name.copy("yet");
    varY->isDimension = true;
    varY->dimensionlinks.push_back(dimY);
    newDataObject->cdfObject->addVariable(varY);
    CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

    ((double *)varY->data)[0] = dfBBOX[1];
    ((double *)varY->data)[1] = dfBBOX[3];

    // For time dimension
    CDF::Dimension *dimTime = new CDF::Dimension();
    dimTime->name = "time";
    dimTime->setSize(10); // 24 * 6); // Last day every 10 minutes
    newDataObject->cdfObject->addDimension(dimTime);

    // Define the Y variable using the X dimension
    CDF::Variable *varTime = new CDF::Variable();
    varTime->setType(CDF_DOUBLE);
    varTime->name.copy("time");
    varTime->isDimension = true;
    varTime->dimensionlinks.push_back(dimTime);
    newDataObject->cdfObject->addVariable(varTime);
    varTime->setAttributeText("units", "seconds since 1970");
    CTime epochCTime;
    epochCTime.init("seconds since 1970-01-01 0:0:0", NULL);
    CDF::allocateData(CDF_DOUBLE, &varTime->data, dimTime->length);

    for (int off = 0; off < 10; off++) {
      // Every 10 minutes for a day
      double timestep = epochCTime.quantizeTimeToISO8601(currentOffset - off * 60 * 10, "PT30M", "low");
      ((double *)varTime->data)[off] = timestep; // timestep;
    }

    dataSource->getDataObject(0)->cdfVariable->dimensionlinks.push_back(dimTime);
    // Define the Solar Terminator variable using the defined dimensions, and set the right attributes
    CDF::Variable *solTVar = new CDF::Variable();
    newDataObject->cdfObject->addVariable(solTVar);
    solTVar->setType(CDF_FLOAT);
    float fillValue[] = {-1};
    solTVar->setAttribute("_FillValue", solTVar->getType(), fillValue, 1);
    // solTVar->dimensionlinks.push_back(dimTime);
    solTVar->dimensionlinks.push_back(dimY);
    solTVar->dimensionlinks.push_back(dimX);
    solTVar->setType(CDF_FLOAT);
    solTVar->name = "SolT";
    solTVar->addAttribute(new CDF::Attribute("units", "FL (ft*100)"));
    solTVar->setAttributeText("grid_mapping", "projection");
    newDataObject->cdfVariable = solTVar;

    newDataObject->cdfVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);

    newDataObject->cdfVariable->setSize(dataSource->dWidth * dataSource->dHeight);

    // Make the width and height of the new 2D adaguc field the same as the viewing window
    dataSource->dWidth = dataSource->srvParams->Geo->dWidth;
    dataSource->dHeight = dataSource->srvParams->Geo->dHeight;

    // Width and height of the dataSource need to be at least 2 in this case.
    if (dataSource->dWidth < 2) dataSource->dWidth = 2;
    if (dataSource->dHeight < 2) dataSource->dHeight = 2;

    // Get the X and Y dimensions previousely defined and adjust them to the new settings and new grid (Grid in screenview space)
    dimX->setSize(dataSource->dWidth);
    dimY->setSize(dataSource->dHeight);

    // Re-allocate data for these coordinate variables with the new grid size
    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);
    CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

    // Calculate the gridsize, allocate data and fill the data with a fillvalue
    size_t fieldSize = dimX->length * dimY->length;
    newDataObject->cdfVariable->setSize(fieldSize);
    CDF::allocateData(newDataObject->cdfVariable->getType(), &(newDataObject->cdfVariable->data), fieldSize);
    CDF::fill(newDataObject->cdfVariable->data, newDataObject->cdfVariable->getType(), fillValue[0], fieldSize);

    // Calculate cellsize and offset of the echo toppen (ET) 2D virtual grid, using the same grid as the screenspace
    double cellSizeX = (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0]) / double(dataSource->dWidth);
    double cellSizeY = (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1]) / double(dataSource->dHeight);
    double offsetX = dataSource->srvParams->Geo->dfBBOX[0];
    double offsetY = dataSource->srvParams->Geo->dfBBOX[1];

    // Fill in the X and Y dimensions with the array of coordinates
    for (size_t j = 0; j < dimX->length; j++) {
      double x = offsetX + double(j) * cellSizeX + cellSizeX / 2;
      ((double *)varX->data)[j] = x;
    }
    for (size_t j = 0; j < dimY->length; j++) {
      double y = offsetY + double(j) * cellSizeY + cellSizeY / 2;
      ((double *)varY->data)[j] = y;
    }
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    CDBDebug("CDATAPOSTPROCESSOR_RUNAFTERREADING::Applying SOLARTERMINATOR");
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    CDF::allocateData(dataSource->getDataObject(0)->cdfVariable->getType(), &dataSource->getDataObject(0)->cdfVariable->data, l);

    float *result = (float *)dataSource->getDataObject(0)->cdfVariable->data;

    CImageWarper imageWarper;
    int status = imageWarper.initreproj(dataSource, dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);
    if (status != 0) {
      CDBError("Unable to init projection");
      return 1;
    }

    for (size_t j = 0; j < l; j++) {
      int px = j % dataSource->dWidth;
      int py = j / dataSource->dWidth;

      double lonRange = dataSource->dfBBOX[2] - dataSource->dfBBOX[0];
      double latRange = dataSource->dfBBOX[1] - dataSource->dfBBOX[3];

      // Projection coordinates (works in EPSG 4326)
      double geox = (lonRange / dataSource->dWidth) * px + dataSource->dfBBOX[0];
      double geoy = (latRange / dataSource->dHeight) * py + dataSource->dfBBOX[3];

      // Transform EPG:3857 coordinates into latlon
      imageWarper.reprojToLatLon(geox, geoy);

      // Select final value based on solar zenith angle
      result[j] = getDayTimeCategory(getSolarZenithAngle(geoy, geox, currentOffset));
    }
  }
  return 0;
}