#include "CDataPostProcessor_ToKnots.h"

/************************/
/*      CDPPToKnots     */
/************************/
const char *CDPPToKnots::className = "CDPPToToKnots";

const char *CDPPToKnots::getId() { return "toknots"; }
int CDPPToKnots::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int) {
  if (proc->attr.algorithm.equals("toknots")) {
    if (CDATAPOSTPROCESSOR_RUNAFTERREADING) {
      if (dataSource->getNumDataObjects() != 1 && dataSource->getNumDataObjects() != 2) {
        CDBError("1 or 2 variables are needed for toknots, found %d", dataSource->getNumDataObjects());
        return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
      }
      return CDATAPOSTPROCESSOR_RUNAFTERREADING;
    }
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPToKnots::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  CDBDebug("Applying toknots %d", mode == CDATAPOSTPROCESSOR_RUNAFTERREADING);
  float factor = 1;
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    if (dataSource->getNumDataObjects() == 1) {
      CDBDebug("Applying toknots for 1 element");
      if (dataSource->getDataObject(0)->getUnits().equals("m/s") || dataSource->getDataObject(0)->getUnits().equals("m s-1")) {
        factor = 3600 / 1852.;
        CDBDebug("Applying toknots for 1 element with factor %f to grid", factor);
        dataSource->getDataObject(0)->setUnits("kts");
        size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
        float *src = (float *)dataSource->getDataObject(0)->cdfVariable->data;
        float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
        for (size_t cnt = 0; cnt < l; cnt++) {
          float speed = *src;
          if (speed == speed) {
            if (speed != noDataValue) {
              *src = factor * speed;
            }
          }
          src++;
        }

        // Convert point data if needed
        size_t nrPoints = dataSource->getDataObject(0)->points.size();
        CDBDebug("(1): %d points", nrPoints);
        for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
          float speed = (float)dataSource->getDataObject(0)->points[pointNo].v;
          if (speed == speed) {
            if (speed != noDataValue) {
              dataSource->getDataObject(0)->points[pointNo].v = factor * speed;
            }
          }
        }
      }
    }
    if (dataSource->getNumDataObjects() == 2) {
      CDBDebug("Applying toknots for 2 elements %s %s", dataSource->getDataObject(0)->getUnits().c_str(), dataSource->getDataObject(1)->getUnits().c_str());
      if ((dataSource->getDataObject(0)->getUnits().equals("m/s") || dataSource->getDataObject(0)->getUnits().equals("m s-1")) && dataSource->getDataObject(1)->getUnits().equals("degree")) {
        factor = 3600 / 1852.;
        // This is a (wind speed,direction) pair
        dataSource->getDataObject(0)->setUnits("kts");
        size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
        float *src = (float *)dataSource->getDataObject(0)->cdfVariable->data;
        float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
        for (size_t cnt = 0; cnt < l; cnt++) {
          float speed = *src;
          if (speed == speed) {
            if (speed != noDataValue) {
              *src = factor * speed;
            }
          }
          src++;
        }
        // Convert point data if needed
        size_t nrPoints = dataSource->getDataObject(0)->points.size();
        CDBDebug("(2): %d points", nrPoints);
        for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
          float speed = dataSource->getDataObject(0)->points[pointNo].v;
          if (speed == speed) {
            if (speed != noDataValue) {
              dataSource->getDataObject(0)->points[pointNo].v = factor * speed;
            }
          }
        }
      }
      if ((dataSource->getDataObject(0)->getUnits().equals("m/s") || dataSource->getDataObject(0)->getUnits().equals("m s-1")) &&
          (dataSource->getDataObject(1)->getUnits().equals("m/s") || dataSource->getDataObject(1)->getUnits().equals("m s-1"))) {
        // This is a (u,v) pair
        dataSource->getDataObject(0)->setUnits("kts");

        size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
        float *srcu = (float *)dataSource->getDataObject(0)->cdfVariable->data;
        float *srcv = (float *)dataSource->getDataObject(1)->cdfVariable->data;
        float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
        float speed;
        float speedu;
        float speedv;
        for (size_t cnt = 0; cnt < l; cnt++) {
          speedu = *srcu;
          speedv = *srcv;
          if ((speedu == speedu) && (speedv == speedv)) {
            if ((speedu != noDataValue) && (speedv != noDataValue)) {
              speed = factor * hypot(speedu, speedv);
              *srcu = speed;
            } else {
              *srcu = noDataValue;
            }
          }
          srcu++;
          srcv++;
        }
        // Convert point data if needed
        size_t nrPoints = dataSource->getDataObject(0)->points.size();
        CDBDebug("(2): %d points", nrPoints);
        for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
          speedu = dataSource->getDataObject(0)->points[pointNo].v;
          speedv = dataSource->getDataObject(1)->points[pointNo].v;
          if ((speedu == speedu) && (speedv == speedv)) {
            if ((speedu != noDataValue) && (speedv != noDataValue)) {
              speed = factor * hypot(speedu, speedv);
              dataSource->getDataObject(0)->points[pointNo].v = speed;
            } else {
              dataSource->getDataObject(0)->points[pointNo].v = noDataValue;
            }
          }
        }
        CDBDebug("Deleting dataObject(1))");
        delete (dataSource->getDataObject(1));
        dataSource->getDataObjectsVector()->erase(dataSource->getDataObjectsVector()->begin() + 1); // Remove second element
      }
    }
  }
  return 0;
}

int CDPPToKnots::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numDataPoints) {
  if (isApplicable(proc, dataSource, mode) == CDATAPOSTPROCESSOR_NOTAPPLICABLE) {
    return -1;
  }
  float factor = 1;
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    if (dataSource->getNumDataObjects() == 1) {
      CDBDebug("Applying toknots for 1 element");
      if (dataSource->getDataObject(0)->getUnits().equals("m/s") || dataSource->getDataObject(0)->getUnits().equals("m s-1")) {
        factor = 3600 / 1852.;
        CDBDebug("Applying toknots for 1 element with factor %f to timeseries", factor);
        dataSource->getDataObject(0)->setUnits("kts");
        double noDataValue = dataSource->getDataObject(0)->dfNodataValue;
        for (size_t cnt = 0; cnt < numDataPoints; cnt++) {
          double speed = data[cnt];
          if (speed == speed) {
            if (speed != noDataValue) {
              data[cnt] = factor * speed;
            }
          }
        }
      } else {
        CDBDebug("Units not recognized (should be m/s or m s-1)");
      }
    }
  }
  return 0;
}