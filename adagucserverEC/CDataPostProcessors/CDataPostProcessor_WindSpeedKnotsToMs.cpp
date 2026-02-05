#include "CDataPostProcessor_WindSpeedKnotsToMs.h"

const char *CDPPWindSpeedKnotsToMs::getId() { return CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID; }

int CDPPWindSpeedKnotsToMs::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int) {
  if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID)) {
    if (CDATAPOSTPROCESSOR_RUNAFTERREADING) {
      if (dataSource->getNumDataObjects() != 1) {
        CDBError("1 variables is needed for %s, found %lu", CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID, dataSource->getNumDataObjects());
        return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
      }
      CT::string units = dataSource->getDataObject(0)->getUnits().c_str();
      CDBDebug("UNITS us %s", units.c_str());
      if (!units.equals("kts")) {
        CDBError("Units should be kts, but they are %s", units.c_str());
        return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
      }
      return CDATAPOSTPROCESSOR_RUNAFTERREADING;
    }
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPWindSpeedKnotsToMs::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == CDATAPOSTPROCESSOR_NOTAPPLICABLE) {
    return -1;
  }
  CDBDebug("Applying CDPPWindSpeedKnotsToMs for grid");
  dataSource->getDataObject(0)->setUnits(CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_NEWUNITS);
  size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
  float *src = (float *)dataSource->getDataObject(0)->cdfVariable->data;
  float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
  for (size_t cnt = 0; cnt < l; cnt++) {
    float speed = *src;
    if (speed == speed) {
      if (speed != noDataValue) {
        *src = CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_KTSTOMSFACTOR * speed;
      }
    }
    src++;
  }

  // Convert point data if needed
  size_t nrPoints = dataSource->getDataObject(0)->points.size();
  for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
    float speed = (float)dataSource->getDataObject(0)->points[pointNo].v;
    if (speed == speed) {
      if (speed != noDataValue) {
        dataSource->getDataObject(0)->points[pointNo].v = CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_KTSTOMSFACTOR * speed;
      }
    }
  }
  return 0;
}

int CDPPWindSpeedKnotsToMs::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numDataPoints) {
  if (isApplicable(proc, dataSource, mode) == CDATAPOSTPROCESSOR_NOTAPPLICABLE) {
    return -1;
  }
  CDBDebug("Applying CDPPWindSpeedKnotsToMs for timeseries");
  dataSource->getDataObject(0)->setUnits(CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_NEWUNITS);
  double noDataValue = dataSource->getDataObject(0)->dfNodataValue;
  for (size_t cnt = 0; cnt < numDataPoints; cnt++) {
    double speed = data[cnt];
    if (speed == speed) {
      if (speed != noDataValue) {
        data[cnt] = CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_KTSTOMSFACTOR * speed;
      }
    }
  }

  return 0;
}