
#include "CDataPostProcessor_Beaufort.h"

const char *CDPPBeaufort::className = "CDPPBeaufort";

const char *CDPPBeaufort::getId() { return "beaufort"; }
int CDPPBeaufort::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("beaufort")) {
    if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
      if (dataSource->getNumDataObjects() != 1 && dataSource->getNumDataObjects() != 2) {
        CDBError("1 or 2 variables are needed for beaufort, found %d", dataSource->getNumDataObjects());
        return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
      }
      return CDATAPOSTPROCESSOR_RUNAFTERREADING;
    }
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

float CDPPBeaufort::getBeaufort(float speed) {
  int bft;
  if (speed < 0.3) {
    bft = 0;
  } else if (speed < 1.6) {
    bft = 1;
  } else if (speed < 3.4) {
    bft = 2;
  } else if (speed < 5.5) {
    bft = 3;
  } else if (speed < 8.0) {
    bft = 4;
  } else if (speed < 10.8) {
    bft = 5;
  } else if (speed < 13.9) {
    bft = 6;
  } else if (speed < 17.2) {
    bft = 7;
  } else if (speed < 20.8) {
    bft = 8;
  } else if (speed < 24.5) {
    bft = 9;
  } else if (speed < 28.5) {
    bft = 10;
  } else if (speed < 32.6) {
    bft = 11;
  } else {
    bft = 12;
  }
  //  CDBDebug("bft(%f)=%d", speed, bft);
  return bft;
}
int CDPPBeaufort::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == CDATAPOSTPROCESSOR_NOTAPPLICABLE) {
    return -1;
  }
  CDBDebug("Applying beaufort %d", mode == CDATAPOSTPROCESSOR_RUNAFTERREADING);
  float factor = 1;
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    if (dataSource->getNumDataObjects() == 1) {
      CDBDebug("Applying beaufort for 1 element");
      if (dataSource->getDataObject(0)->getUnits().equals("knot") || dataSource->getDataObject(0)->getUnits().equals("kt")) {
        factor = 1852. / 3600;
      }
      CDBDebug("Applying beaufort for 1 element with factor %f", factor);
      dataSource->getDataObject(0)->setUnits("bft");
      size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
      float *src = (float *)dataSource->getDataObject(0)->cdfVariable->data;
      float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
      for (size_t cnt = 0; cnt < l; cnt++) {
        float speed = *src;
        if (speed == speed) {
          if (speed != noDataValue) {
            *src = getBeaufort(factor * speed);
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
            dataSource->getDataObject(0)->points[pointNo].v = getBeaufort(factor * speed);
          }
        }
      }
    }
    if (dataSource->getNumDataObjects() == 2) {
      CDBDebug("Applying beaufort for 2 elements %s %s", dataSource->getDataObject(0)->getUnits().c_str(), dataSource->getDataObject(1)->getUnits().c_str());
      if ((dataSource->getDataObject(0)->getUnits().equals("m/s") || dataSource->getDataObject(0)->getUnits().equals("m s-1")) && dataSource->getDataObject(1)->getUnits().equals("degree")) {
        // This is a (wind speed,direction) pair
        dataSource->getDataObject(0)->setUnits("bft");
        size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
        float *src = (float *)dataSource->getDataObject(0)->cdfVariable->data;
        float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
        for (size_t cnt = 0; cnt < l; cnt++) {
          float speed = *src;
          if (speed == speed) {
            if (speed != noDataValue) {
              *src = getBeaufort(factor * speed);
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
              dataSource->getDataObject(0)->points[pointNo].v = getBeaufort(factor * speed);
            }
          }
        }
      }
      if ((dataSource->getDataObject(0)->getUnits().equals("m/s") || dataSource->getDataObject(0)->getUnits().equals("m s-1")) &&
          (dataSource->getDataObject(1)->getUnits().equals("m/s") || dataSource->getDataObject(1)->getUnits().equals("m s-1"))) {
        // This is a (u,v) pair
        dataSource->getDataObject(0)->setUnits("bft");

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
              *srcu = getBeaufort(speed);
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
              dataSource->getDataObject(0)->points[pointNo].v = getBeaufort(speed);
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

int CDPPBeaufort::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numDataPoints) {
  if (isApplicable(proc, dataSource, mode) == CDATAPOSTPROCESSOR_NOTAPPLICABLE) {
    return -1;
  }
  CDBDebug("Applying beaufort %d", mode == CDATAPOSTPROCESSOR_RUNAFTERREADING);
  float factor = 1;
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    if (dataSource->getNumDataObjects() == 1) {
      CDBDebug("Applying beaufort for 1 element");
      if (dataSource->getDataObject(0)->getUnits().equals("knot") || dataSource->getDataObject(0)->getUnits().equals("kt")) {
        factor = 1852. / 3600;
      }
      CDBDebug("Applying beaufort for 1 element with factor %f", factor);
      dataSource->getDataObject(0)->setUnits("bft");
      double noDataValue = dataSource->getDataObject(0)->dfNodataValue;
      for (size_t cnt = 0; cnt < numDataPoints; cnt++) {
        double speed = data[cnt];
        if (speed == speed) {
          if (speed != noDataValue) {
            data[cnt] = getBeaufort(factor * speed);
          }
        }
      }
    }
  }
  return 0;
}