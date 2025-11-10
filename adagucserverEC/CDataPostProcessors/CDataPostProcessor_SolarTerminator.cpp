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
  if ((isApplicable(proc, dataSource, mode) & mode) == false) {
    return -1;
  }

  double currentOffset = 1;

  if (!dataSource->srvParams->requestDims.empty()) {
    CT::string timestampStr = dataSource->srvParams->requestDims[0]->value.c_str();
    currentOffset = CTime::getEpochTimeFromDateString(dataSource->srvParams->requestDims[0]->value);
  }

  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    // Copy bounding box of screen
    dataSource->setGeo(dataSource->srvParams->Geo);

    // Width and height of the dataSource need to be at least 2 in this case.
    if (dataSource->dWidth < 2) dataSource->dWidth = 2;
    if (dataSource->dHeight < 2) dataSource->dHeight = 2;
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
    CDBDebug("%f %f", dataSource->srvParams->dX, dataSource->srvParams->dY);
    int dX = int(dataSource->srvParams->dX);
    int dY = int(dataSource->srvParams->dY);
    double lonRange = dataSource->dfBBOX[2] - dataSource->dfBBOX[0];
    double latRange = dataSource->dfBBOX[1] - dataSource->dfBBOX[3];

    float gfiValue;
    if (dataSource->srvParams->requestType == REQUEST_WMS_GETFEATUREINFO) {
      double geox = (lonRange / dataSource->dWidth) * dX + dataSource->dfBBOX[0];
      double geoy = (latRange / dataSource->dHeight) * dY + dataSource->dfBBOX[3];

      // Transform EPG:3857 coordinates into latlon
      imageWarper.reprojToLatLon(geox, geoy);
      gfiValue = static_cast<float>(getDayTimeCategory(getSolarZenithAngle(geoy, geox, currentOffset)));
    }

    for (size_t j = 0; j < l; j++) {
      int px = j % dataSource->dWidth;
      int py = j / dataSource->dWidth;

      if (dataSource->srvParams->requestType == REQUEST_WMS_GETFEATUREINFO) {

        // Select final value based on solar zenith angle
        result[j] = gfiValue;
      } else {
        // Projection coordinates (works in EPSG 4326)
        double geox = (lonRange / dataSource->dWidth) * px + dataSource->dfBBOX[0];
        double geoy = (latRange / dataSource->dHeight) * py + dataSource->dfBBOX[3];

        // Transform EPG:3857 coordinates into latlon
        imageWarper.reprojToLatLon(geox, geoy);

        // Select final value based on solar zenith angle
        result[j] = static_cast<float>(getDayTimeCategory(getSolarZenithAngle(geoy, geox, currentOffset)));
      }
    }
  }
  return 0;
}