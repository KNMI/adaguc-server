#include "CDataPostProcessor_SolarTerminator.h"
#include "solar/solar_terminator.h"
#include <chrono>
#include <ctime>
#include "CTime.h"
#include "CImageWarper.h"

#include <execinfo.h>
#include <cstdio>
#include <cstdlib>
#include <cxxabi.h>
#include <dlfcn.h>

#include <ctime>

/************************/
/*      CDPPSolarTerminator  */
/************************/

#define MAX_SOLT_WIDTH 1024
#define MAX_SOLT_HEIGHT 1024

const char *CDPPSolarTerminator::getId() { return "solarterminator"; }

int CDPPSolarTerminator::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("solarterminator")) {
    if (dataSource->getNumDataObjects() < 1 && mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
      CDBError("1 variable is needed for solarterminator, found %lu", dataSource->getNumDataObjects());
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
    dataSource->setGeo(dataSource->srvParams->geoParams);

    if (dataSource->srvParams->requestType != REQUEST_WMS_GETFEATUREINFO) {
      dataSource->dWidth = dataSource->srvParams->geoParams.width / 16;
      dataSource->dHeight = dataSource->srvParams->geoParams.height / 16;
      if (proc->attr.stride.empty() == false) {
        float stride = proc->attr.stride.toFloat();
        dataSource->dWidth = dataSource->srvParams->geoParams.width / stride;
        dataSource->dHeight = dataSource->srvParams->geoParams.height / stride;
      }
    }
    // Width and height of the dataSource need to be at least 2 in this case.
    if (dataSource->dWidth < 2) dataSource->dWidth = 2;
    if (dataSource->dHeight < 2) dataSource->dHeight = 2;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    // CDBDebug("CDATAPOSTPROCESSOR_RUNAFTERREADING::Applying SOLARTERMINATOR");
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    dataSource->getDataObject(0)->cdfVariable->allocateData(l);

    float *result = (float *)dataSource->getDataObject(0)->cdfVariable->data;

    // CDBDebug("Initializing image warper for reprojection");
    CImageWarper imageWarper;
    int status = imageWarper.initreproj(dataSource, dataSource->srvParams->geoParams, &dataSource->srvParams->cfg->Projection);
    if (status != 0) {
      CDBError("Unable to init projection");
      return 1;
    }
    // CDBDebug("Finished initializing image warper for reprojection %d", imageWarper.isProjectionRequired());
    int dX = int(dataSource->srvParams->dX);
    int dY = int(dataSource->srvParams->dY);

    double lonRange = dataSource->dfBBOX[2] - dataSource->dfBBOX[0];
    double latRange = dataSource->dfBBOX[1] - dataSource->dfBBOX[3];

    bool singlePointRequest = (lonRange == 0);

    float gfiValue;

    if (singlePointRequest) {
      // Compute for one point (with lat/lon)
      double geox, geoy;
      geox = dataSource->srvParams->dX;
      geoy = dataSource->srvParams->dY;

      float gfiValue = static_cast<float>(getSolarZenithAngle(geoy, geox, currentOffset));

      // fill the (possibly 1x1) raster
      for (size_t j = 0; j < l; j++) {
        result[j] = gfiValue;
      }
      return 0;
    }

    if (dataSource->srvParams->requestType == REQUEST_WMS_GETFEATUREINFO) {
      double geox = (lonRange / dataSource->dWidth) * dX + dataSource->dfBBOX[0];
      double geoy = (latRange / dataSource->dHeight) * dY + dataSource->dfBBOX[3];

      // Transform EPSG:3857 coordinates into latlon (only if this is latlon!)
      imageWarper.reprojToLatLon(geox, geoy);
      gfiValue = static_cast<float>(getSolarZenithAngle(geoy, geox, currentOffset));

      // Calculate the only value required and assign it around the pixel in question (3x3 cell)
      for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
          int px = dX + dx;
          int py = dY + dy;
          if (px < 0 || py < 0 || px >= dataSource->dWidth || py >= dataSource->dHeight) continue;

          size_t idx = static_cast<size_t>(py) * dataSource->dWidth + static_cast<size_t>(px);
          result[idx] = gfiValue;
        }
      }

    } else { // Assuming REQUEST_WMS_GETMAP
      // CDBDebug("Calculating solar zenith angle for each pixel in the raster");
      for (size_t j = 0; j < l; j++) {
        int px = j % dataSource->dWidth;
        int py = j / dataSource->dWidth;

        // Projection coordinates (works in EPSG 4326)
        double geox = (lonRange / dataSource->dWidth) * px + dataSource->dfBBOX[0];
        double geoy = (latRange / dataSource->dHeight) * py + dataSource->dfBBOX[3];

        // Transform EPG:3857 coordinates into latlon
        imageWarper.reprojToLatLon(geox, geoy);

        // Select final value based on solar zenith angle
        result[j] = static_cast<float>(getSolarZenithAngle(geoy, geox, currentOffset));
      }
      // CDBDebug("Finished calculating solar zenith angle for each pixel in the raster");
    }
  }
  return 0;
}