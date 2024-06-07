#include "CDataPostProcessor_CDPDBZtoRR.h"

/************************/
/*      CDPDBZtoRR     */
/************************/
const char *CDPDBZtoRR::className = "CDPDBZtoRR";

const char *CDPDBZtoRR::getId() { return CDATAPOSTPROCESSOR_DBZtoRR_ID; }
int CDPDBZtoRR::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm.equals("dbztorr")) {
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

float CDPDBZtoRR::getRR(float dbZ) {
  // TODO: Check why -32 as input does not return 0
  return pow((pow(10, dbZ / 10.) / 200), 1 / 1.6);
}

int CDPDBZtoRR::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numItems) {
  CDBDebug("CDPDBZtoRR");
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    dataSource->getDataObject(0)->setUnits("mm/hr");
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
    for (size_t j = 0; j < numItems; j++) {
      if (data[j] == data[j]) {       // Check if NaN
        if (data[j] != noDataValue) { // Check if equal to nodatavalue of the datasource
          data[j] = getRR(data[j]);
        }
      }
    }
  }
  return 0;
}

int CDPDBZtoRR::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    dataSource->getDataObject(0)->setUnits("mm/hr");
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    float *src = (float *)dataSource->getDataObject(0)->cdfVariable->data;
    float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
    for (size_t cnt = 0; cnt < l; cnt++) {
      float dbZ = *src;
      if (dbZ == dbZ) {
        if (dbZ != noDataValue) {
          *src = getRR(dbZ);
        }
      }
      src++;
    }
  }
  return 0;
}
