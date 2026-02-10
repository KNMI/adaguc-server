#include "CDataPostProcessor_ConvertUnits.h"

const char *CDPPConvertUnits::className = "CDPPConvertUnits";

const char *CDPPConvertUnits::getId() { return CDATAPOSTPROCESSOR_CONVERTUNITS_ID; }

int CDPPConvertUnits::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  CDBDebug("mode: %d %s", mode, proc->attr.algorithm.c_str());
  if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_CONVERTUNITS_ID)) {
    CT::string fromUnits = proc->attr.from_units.empty() ? "" : proc->attr.from_units;
    if ((mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) || (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING)) {
      if (fromUnits.empty()) {
        if (dataSource->getNumDataObjects() != 1) {
          // Without from_units: only allow 1 dataObject
          CDBError("1 variable allowed for %s, found %d", CDATAPOSTPROCESSOR_CONVERTUNITS_ID, dataSource->getNumDataObjects());
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

// int isApplicable_(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
//   CDBDebug("mode: %d", mode);
//   if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_CONVERTUNITS_ID)) {
//     CT::string newUnits = proc->attr.units.empty() ? "" : proc->attr.units;
//     CT::string fromUnits = proc->attr.from_units.empty() ? "" : proc->attr.from_units;
//     if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
//       if (fromUnits.empty()) {
//         if (dataSource->getNumDataObjects() != 1) {
//           // Without from_units: only allow 1 dataObject
//           CDBError("1 variable allowed for %s, found %d", CDATAPOSTPROCESSOR_CONVERTUNITS_ID, dataSource->getNumDataObjects());
//           return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
//         }
//       }
//       return CDATAPOSTPROCESSOR_RUNAFTERREADING;
//     }
//   }
//   if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_TOKNOTS_ID)) {
//     CT::string newUnits = proc->attr.units.empty() ? "kts" : proc->attr.units;
//     CT::string fromUnits = proc->attr.from_units.empty() ? "m/s" : proc->attr.from_units;
//     if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
//       CT::string factor;
//       factor.print("%f", 3600. / 1852);
//       proc->addAttribute("a", factor.c_str());
//       proc->addAttribute("b", "0");
//       proc->addAttribute("units", newUnits);
//       proc->addAttribute("from_units", fromUnits);
//       return CDATAPOSTPROCESSOR_RUNAFTERREADING;
//     }
//   }
//   if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID)) {
//     CT::string newUnits = proc->attr.units.empty() ? "m/s" : proc->attr.units;
//     CT::string fromUnits = proc->attr.from_units.empty() ? "kts" : proc->attr.from_units;
//     if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
//       CT::string factor;
//       factor.print("%f", 1852. / 3600);
//       proc->addAttribute("a", factor.c_str());
//       proc->addAttribute("b", "0");
//       proc->addAttribute("units", newUnits);
//       proc->addAttribute("from_units", fromUnits);
//       return CDATAPOSTPROCESSOR_RUNAFTERREADING;
//     }
//   }
//   return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
// }
template <typename T> void do_convert(T noDataValue, size_t l, T *src, double a, double b) {
  int cnt1 = 0;
  int cnt2 = 0;
  if (isnan(noDataValue)) {
    CDBDebug("isnan");
    for (size_t cnt = 0; cnt < l; cnt++) {
      float value = *src;
      if (!isnan(value)) {
        *src = a * value + b;
      }
      src++;
      cnt1++;
    }
  } else {
    CDBDebug("not isnan");
    for (size_t cnt = 0; cnt < l; cnt++) {
      float value = *src;
      if (!isnan(value)) {
        if (value != noDataValue) {
          *src = a * value + b;
        }
      }
      src++;
      cnt2++;
    }
  }
  CDBDebug("changed %d %d", cnt1, cnt2);
}

int CDPPConvertUnits::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {

  if (isApplicable(proc, dataSource, mode) == CDATAPOSTPROCESSOR_NOTAPPLICABLE) {
    return -1;
  }
  CDBDebug("execute %d %s", mode, proc->attr.algorithm.c_str());
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_CONVERTUNITS_ID)) {
      double a = proc->attr.a.empty() ? 1 : proc->attr.a.toDouble();
      double b = proc->attr.b.empty() ? 0 : proc->attr.b.toDouble();
      CT::string newUnits = proc->attr.units;
      CT::string fromUnits = proc->attr.from_units;
      CDBDebug("Applying CDPPConvertUnits for grid (%f,%f) %s => %s", a, b, fromUnits.c_str(), newUnits.c_str());
      if (fromUnits.empty()) {
        auto dataObject = dataSource->getDataObject(0);
        dataObject->cdfVariable->needsDataConversion = true;
        dataObject->cdfVariable->needsDataConversion_ = true;
        dataObject->cdfVariable->factor = a;
        dataObject->cdfVariable->offset = b;
        dataObject->setUnits(newUnits);

      } else {
        for (const auto dataObject : dataSource->dataObjects) {
          CDBDebug("units: %s %s", dataObject->getUnits().c_str(), fromUnits.c_str());
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
      CDBDebug("Applying CDPPConvertUnits for grid (%f,%f) %s => %s", a, b, fromUnits.c_str(), newUnits.c_str());
      for (const auto dataObject : dataSource->dataObjects) {
        CDBDebug("%s %s", dataObject->getUnits().c_str(), fromUnits.c_str());
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
      CDBDebug("Applying CDPPConvertUnits %s for grid (%f,%f) %s => %s", CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID, a, b, fromUnits.c_str(), newUnits.c_str());
      for (const auto dataObject : dataSource->dataObjects) {
        CDBDebug("%s %s", dataObject->getUnits().c_str(), fromUnits.c_str());
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
    CDBDebug("Applying CDPPConvertUnits for grid %s", proc->attr.algorithm.c_str());
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    for (const auto dataObject : dataSource->dataObjects) {
      CDBDebug("%s %d", dataObject->cdfVariable->name.c_str(), dataObject->cdfVariable->needsDataConversion);
      if (dataObject->cdfVariable->needsDataConversion) {
        CDFType type = dataObject->cdfVariable->getType();
        double noDataValue = dataObject->dfNodataValue;
        CDBDebug("Converting %s [%d]", dataObject->cdfVariable->name.c_str(), dataObject->cdfVariable->getType());
        if (type == CDF_DOUBLE) {
          double *src = (double *)dataObject->cdfVariable->data;
          do_convert(noDataValue, l, src, dataObject->cdfVariable->factor, dataObject->cdfVariable->offset);
        } else if (type == CDF_FLOAT) {
          float *src = (float *)dataObject->cdfVariable->data;
          do_convert((float)noDataValue, l, src, dataObject->cdfVariable->factor, dataObject->cdfVariable->offset);
        }
        // Convert point data if needed
        size_t nrPoints = dataObject->points.size();
        if (nrPoints > 0) {
          CDBDebug("Applying CDPPConvertUnits for %d points", nrPoints);
        }
        for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
          if (type == CDF_FLOAT) {
            float value = (float)dataObject->points[pointNo].v;
            if (isnan(noDataValue)) {
              if (!isnan(value)) {
                dataObject->points[pointNo].v = dataObject->cdfVariable->factor * value + dataObject->cdfVariable->offset;
              }
            } else {
              if (!isnan(value) && (value != noDataValue)) {
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
  CDBDebug("Applying CDPPConvertUnits for timeseries, mode %d", mode);
  if (isApplicable(proc, dataSource, mode) == CDATAPOSTPROCESSOR_NOTAPPLICABLE) {
    return -1;
  }
  double a = proc->attr.a.empty() ? 1 : proc->attr.a.toDouble();
  double b = proc->attr.b.empty() ? 0 : proc->attr.b.toDouble();
  CT::string fromUnits = proc->attr.from_units;

  CDBDebug("Applying CDPPConvertUnits for timeseries [%d] (%f, %f) %s=> %s", mode, a, b, proc->attr.from_units.c_str(), proc->attr.units.c_str());
  CDBDebug("LL: %d", dataSource->dataObjects.size());
  for (const auto dataObject : dataSource->dataObjects) {
    CDBDebug("33");
    if (dataObject->cdfVariable->needsDataConversion_) {
      double noDataValue = dataObject->dfNodataValue;
      if (isnan(noDataValue)) {
        for (size_t cnt = 0; cnt < numDataPoints; cnt++) {
          double value = data[cnt];
          if (!isnan(value)) {
            data[cnt] = a * value + b;
          }
        }
      } else {
        for (size_t cnt = 0; cnt < numDataPoints; cnt++) {
          double value = data[cnt];
          if (!isnan(value)) {
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