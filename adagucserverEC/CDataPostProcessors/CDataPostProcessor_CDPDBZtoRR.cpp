#include <type_traits>
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

template <class T>
typename std::enable_if<std::is_same<T, double>::value || std::is_same<T, float>::value, int>::type //
CDPDBZtoRR::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, T *data, size_t numItems) {
  CDBDebug("CDPDBZtoRR");
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    dataSource->getDataObject(0)->setUnits("mm/hr");
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    double noDataValue = dataSource->getDataObject(0)->dfNodataValue;
    for (size_t j = 0; j < numItems; j++) {
      if (data[j] == data[j]) {       // Check if NaN
        if (data[j] != noDataValue) { // Check if equal to nodatavalue of the datasource
          data[j] = pow((pow(10., data[j] * 0.1) / 200.), 0.625);
        }
      }
    }
  }
  return 0;
}

int CDPDBZtoRR::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numItems) { //
  return execute<double>(proc, dataSource, mode, data, numItems);                                                                   //
}

int CDPDBZtoRR::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
  if (dataSource->getDataObject(0)->cdfVariable->currentType == CDF_FLOAT) {
    float *src = (float *)dataSource->getDataObject(0)->cdfVariable->data;
    return execute<float>(proc, dataSource, mode, src, l);
  } else if (dataSource->getDataObject(0)->cdfVariable->currentType == CDF_DOUBLE) {
    double *src = (double *)dataSource->getDataObject(0)->cdfVariable->data;
    return execute<double>(proc, dataSource, mode, src, l);
  } else {
    return -1;
  }
  return 0;
}
