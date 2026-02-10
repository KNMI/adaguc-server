#include "CDataPostProcessor_ConvertUnits.h"

const char *CDPPConvertUnits::getId() { return CDATAPOSTPROCESSOR_CONVERTUNITS_ID; }

int CDPPConvertUnits::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_CONVERTUNITS_ID)) {
    CT::string fromUnits = proc->attr.from_units.empty() ? "" : proc->attr.from_units;
    if ((mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) || (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING)) {
      if (fromUnits.empty()) {
        if (dataSource->getNumDataObjects() != 1) {
          // Without from_units: only allow 1 dataObject
          CDBError("1 variable allowed for %s, found %ld", CDATAPOSTPROCESSOR_CONVERTUNITS_ID, dataSource->getNumDataObjects());
          return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
        }
      }
      return CDATAPOSTPROCESSOR_RUNBEFOREREADING | CDATAPOSTPROCESSOR_RUNAFTERREADING;
    }
  }
  if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_TOKNOTS_ID)) {
    if ((mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) || (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING)) {
      return CDATAPOSTPROCESSOR_RUNBEFOREREADING | CDATAPOSTPROCESSOR_RUNAFTERREADING;
    }
  }
  if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID)) {
    if ((mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) || (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING)) {
      return CDATAPOSTPROCESSOR_RUNBEFOREREADING | CDATAPOSTPROCESSOR_RUNAFTERREADING;
    }
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

template <typename T> void do_convert(T noDataValue, size_t l, T *src, double a, double b) {
  if (std::isnan(noDataValue)) {
    for (size_t cnt = 0; cnt < l; cnt++) {
      float value = *src;
      if (!std::isnan(value)) {
        *src = a * value + b;
      }
      src++;
    }
  } else {
    for (size_t cnt = 0; cnt < l; cnt++) {
      float value = *src;
      if (!std::isnan(value)) {
        if (value != noDataValue) {
          *src = a * value + b;
        }
      }
      src++;
    }
  }
}

int CDPPConvertUnits::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {

  if (isApplicable(proc, dataSource, mode) == CDATAPOSTPROCESSOR_NOTAPPLICABLE) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_CONVERTUNITS_ID)) {
      double a = proc->attr.a.empty() ? 1 : proc->attr.a.toDouble();
      double b = proc->attr.b.empty() ? 0 : proc->attr.b.toDouble();
      CT::string newUnits = proc->attr.units;
      CT::string fromUnits = proc->attr.from_units;
      if (fromUnits.empty()) {
        auto dataObject = dataSource->getDataObject(0);
        dataObject->cdfVariable->needsDataConversion = true;
        dataObject->cdfVariable->needsDataConversion_ = true;
        dataObject->cdfVariable->factor = a;
        dataObject->cdfVariable->offset = b;
        dataObject->setUnits(newUnits);

      } else {
        for (const auto dataObject : dataSource->dataObjects) {
          if (dataObject->getUnits().equals(fromUnits)) {
            dataObject->cdfVariable->needsDataConversion = true;
            dataObject->cdfVariable->needsDataConversion_ = true;
            dataObject->cdfVariable->factor = a;
            dataObject->cdfVariable->offset = b;
            dataObject->setUnits(newUnits);
          }
        }
      }
    }
    if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_TOKNOTS_ID)) {
      double a = proc->attr.a.empty() ? 3600 / 1852. : proc->attr.a.toDouble();
      double b = proc->attr.b.empty() ? 0 : proc->attr.b.toDouble();
      CT::string newUnits = proc->attr.units.empty() ? "kts" : proc->attr.units;
      CT::string fromUnits = proc->attr.from_units.empty() ? "m/s" : proc->attr.from_units;
      for (const auto dataObject : dataSource->dataObjects) {
        if (dataObject->getUnits().equals(fromUnits)) {
          dataObject->cdfVariable->needsDataConversion = true;
          dataObject->cdfVariable->needsDataConversion_ = true;
          dataObject->cdfVariable->factor = a;
          dataObject->cdfVariable->offset = b;
          dataObject->setUnits(newUnits);
        }
      }
    }
    if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID)) {
      double a = proc->attr.a.empty() ? 1852. / 3600 : proc->attr.a.toDouble();
      double b = proc->attr.b.empty() ? 0 : proc->attr.b.toDouble();
      CT::string newUnits = proc->attr.units.empty() ? "m/s" : proc->attr.units;
      CT::string fromUnits = proc->attr.from_units.empty() ? "kts" : proc->attr.from_units;
      for (const auto dataObject : dataSource->dataObjects) {
        if (dataObject->getUnits().equals(fromUnits)) {
          dataObject->cdfVariable->needsDataConversion = true;
          dataObject->cdfVariable->needsDataConversion_ = true;
          dataObject->cdfVariable->factor = a;
          dataObject->cdfVariable->offset = b;
          dataObject->setUnits(newUnits);
        }
      }
    }
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    for (const auto dataObject : dataSource->dataObjects) {
      if (dataObject->cdfVariable->needsDataConversion) {
        CDFType type = dataObject->cdfVariable->getType();
        double noDataValue = dataObject->dfNodataValue;
        if (type == CDF_DOUBLE) {
          double *src = (double *)dataObject->cdfVariable->data;
          do_convert(noDataValue, l, src, dataObject->cdfVariable->factor, dataObject->cdfVariable->offset);
        } else if (type == CDF_FLOAT) {
          float *src = (float *)dataObject->cdfVariable->data;
          do_convert((float)noDataValue, l, src, dataObject->cdfVariable->factor, dataObject->cdfVariable->offset);
        }
        // Convert point data if needed
        size_t nrPoints = dataObject->points.size();
        for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
          if (type == CDF_FLOAT) {
            float value = (float)dataObject->points[pointNo].v;
            if (std::isnan(noDataValue)) {
              if (!std::isnan(value)) {
                dataObject->points[pointNo].v = dataObject->cdfVariable->factor * value + dataObject->cdfVariable->offset;
              }
            } else {
              if (!std::isnan(value) && (value != noDataValue)) {
                dataObject->points[pointNo].v = dataObject->cdfVariable->factor * value + dataObject->cdfVariable->offset;
              }
            }
          }
        }
        dataObject->cdfVariable->needsDataConversion = false;
      }
    }
  }
  return 0;
}

int CDPPConvertUnits::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numDataPoints) {
  if (isApplicable(proc, dataSource, mode) == CDATAPOSTPROCESSOR_NOTAPPLICABLE) {
    return -1;
  }
  double a = proc->attr.a.empty() ? 1 : proc->attr.a.toDouble();
  double b = proc->attr.b.empty() ? 0 : proc->attr.b.toDouble();
  CT::string fromUnits = proc->attr.from_units;

  for (const auto dataObject : dataSource->dataObjects) {
    if (dataObject->cdfVariable->needsDataConversion_) {
      double noDataValue = dataObject->dfNodataValue;
      if (std::isnan(noDataValue)) {
        for (size_t cnt = 0; cnt < numDataPoints; cnt++) {
          double value = data[cnt];
          if (!std::isnan(value)) {
            data[cnt] = a * value + b;
          }
        }
      } else {
        for (size_t cnt = 0; cnt < numDataPoints; cnt++) {
          double value = data[cnt];
          if (!std::isnan(value)) {
            if (value != noDataValue) {
              data[cnt] = a * value + b;
            }
          }
        }
      }
    }
    dataObject->cdfVariable->needsDataConversion_ = false;
  }

  return 0;
}