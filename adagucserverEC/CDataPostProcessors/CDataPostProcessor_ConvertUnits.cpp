#include "CDataPostProcessor_ConvertUnits.h"

const char *CDPPConvertUnits::getId() { return CDATAPOSTPROCESSOR_CONVERTUNITS_ID; }

int CDPPConvertUnits::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int mode) {

  if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_CONVERTUNITS_ID) || proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_TOKNOTS_ID) ||
      proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID)) {
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

struct LookupUnits {
  double a;
  double b;
  std::string units;
};

std::map<std::string, LookupUnits> lookUp = {{CDATAPOSTPROCESSOR_TOKNOTS_ID, {.a = 3600 / 1852., .b = 0, .units = "kts"}},
                                             {CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID, {.a = 1852. / 3600., .b = 0, .units = "m s-1"}}};

LookupUnits getLookupUnits(CServerConfig::XMLE_DataPostProc &proc) {
  // If the ID is recognized in the lookup, always return those a an b values (ignore the configured a and b )
  if (lookUp.find(proc.attr.algorithm) != lookUp.end()) {
    auto lookedUp = lookUp[proc.attr.algorithm];
    if (!proc.attr.units.empty()) {

      lookedUp.units = proc.attr.units;
    }
    CDBDebug("FOUND LOOKUP: %s %f %f %s", proc.attr.algorithm.c_str(), lookedUp.a, lookedUp.b, lookedUp.units.c_str());
    return lookedUp;
  }
  // Not in lookup, use the a an b from DataProcessor configuration
  LookupUnits l = {.a = 1, .b = 0, .units = ""};
  if (!proc.attr.a.empty()) {
    l.a = proc.attr.a.toDouble();
  }
  if (!proc.attr.b.empty()) {
    l.b = proc.attr.b.toDouble();
  }
  if (!proc.attr.units.empty()) {
    l.units = proc.attr.units;
  }

  CDBDebug("Using own: %s %f %f %s", proc.attr.algorithm.c_str(), l.a, l.b, l.units.c_str());
  return l;
}

int CDPPConvertUnits::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == CDATAPOSTPROCESSOR_NOTAPPLICABLE) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    auto lookupUnit = getLookupUnits(*proc);
    CDBDebug("BEFORE: %s %f %f %s", proc->attr.algorithm.c_str(), lookupUnit.a, lookupUnit.b, lookupUnit.units.c_str());
    auto fromUnits = proc->attr.from_units;
    for (const auto dataObject: dataSource->dataObjects) {
      if (fromUnits.empty() || dataObject->getUnits().equals(fromUnits)) {
        dataObject->cdfVariable->setAttributeText("ADAGUC_POSTPROC_WAS_UNITS", dataObject->getUnits());
        dataObject->cdfVariable->setAttributeText("ADAGUC_POSTPROC_NEEDSCONVERSION", "true");
        dataObject->setUnits(lookupUnit.units.c_str());

        // Keep track how often we hit before reading with these kind of procs.
        auto histBeforeAttr = dataObject->cdfVariable->getAttributeNE("ADAGUC_POSTPROC_BEFORE_READING");
        std::string histBeforeStr = "";
        if (histBeforeAttr == nullptr) {
          dataObject->cdfVariable->setAttributeText("ADAGUC_POSTPROC_BEFORE_READING", "");
          histBeforeAttr = dataObject->cdfVariable->getAttributeNE("ADAGUC_POSTPROC_BEFORE_READING");
        }
        histBeforeStr = histBeforeAttr->getDataAsString();

        CT::printfconcat(histBeforeStr, "%s", proc->attr.algorithm.c_str());
        histBeforeAttr->setData(histBeforeStr.c_str());
      }
    }
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    auto lookupUnit = getLookupUnits(*proc);
    CDBDebug("AFTER: %s %f %f %s", proc->attr.algorithm.c_str(), lookupUnit.a, lookupUnit.b, lookupUnit.units.c_str());

    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    for (const auto dataObject: dataSource->dataObjects) {
      // CDBDebug("Using var %s", dataObject->cdfVariable->name.c_str());
      auto attrNeedsConversion = dataObject->cdfVariable->getAttributeNE("ADAGUC_POSTPROC_NEEDSCONVERSION");
      if (attrNeedsConversion != nullptr && attrNeedsConversion->getDataAsString().equals("done")) {
        CDBError("Warning not yet possible!!!"); // TODO:
      }
      // Check if we need to convert the data
      if (attrNeedsConversion != nullptr && attrNeedsConversion->getDataAsString().equals("true")) {
        // Indicate that this is converted, so it is not converted twice.
        attrNeedsConversion->setData("done");
        CDFType type = dataObject->cdfVariable->getType();

        if (type == CDF_DOUBLE) {
          do_convert<double>(dataObject->hasNodataValue ? dataObject->dfNodataValue : NAN, l, (double *)dataObject->cdfVariable->data, lookupUnit.a, lookupUnit.b);
        } else if (type == CDF_FLOAT) {
          do_convert<float>(dataObject->hasNodataValue ? dataObject->dfNodataValue : NAN, l, (float *)dataObject->cdfVariable->data, lookupUnit.a, lookupUnit.b);
        }

        // Convert point data if needed
        size_t nrPoints = dataObject->points.size();
        float noDataValue = dataObject->hasNodataValue ? dataObject->dfNodataValue : NAN;

        for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
          if (!(dataObject->points[pointNo].v == noDataValue)) {
            dataObject->points[pointNo].v = lookupUnit.a * dataObject->points[pointNo].v + lookupUnit.b;
          }
        }
        // TODO: Calc stat
      } else {
        CDBDebug("Skip Using var %s", dataObject->cdfVariable->name.c_str());
      }
    }

    if (dataSource->statistics == nullptr) {
      CDBDebug("<%d> No statistics %s", mode, dataSource->layerName.c_str());
    } else {
      dataSource->statistics->min = dataSource->statistics->min * lookupUnit.a + lookupUnit.b;
      dataSource->statistics->max = dataSource->statistics->max * lookupUnit.a + lookupUnit.b;
      CDBDebug("<%d> %f,%f %s", mode, dataSource->statistics->getMinimum(), dataSource->statistics->getMaximum(), dataSource->layerName.c_str());
    }
  }
  return 0;
}

int CDPPConvertUnits::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numDataPoints) {
  if (isApplicable(proc, dataSource, mode) == CDATAPOSTPROCESSOR_NOTAPPLICABLE) {
    return -1;
  }
  size_t l = numDataPoints;
  auto lookupUnit = getLookupUnits(*proc);
  for (const auto dataObject: dataSource->dataObjects) {
    CDBDebug("Using var %s", dataObject->cdfVariable->name.c_str());
    auto attrNeedsConversion = dataObject->cdfVariable->getAttributeNE("ADAGUC_POSTPROC_NEEDSCONVERSION");
    if (attrNeedsConversion != nullptr && attrNeedsConversion->getDataAsString().equals("done")) {
      CDBError("Warning not yet possible!!!"); // TODO:
    }
    // Check if we need to convert the data
    if (attrNeedsConversion != nullptr && attrNeedsConversion->getDataAsString().equals("true")) {
      // Indicate that this is converted, so it is not converted twice.
      attrNeedsConversion->setData("done");

      do_convert<double>(dataObject->hasNodataValue ? dataObject->dfNodataValue : NAN, l, (double *)data, lookupUnit.a, lookupUnit.b);
    }
  }
  return 0;
  // double a = proc->attr.a.empty() ? 1 : proc->attr.a.toDouble();
  // double b = proc->attr.b.empty() ? 0 : proc->attr.b.toDouble();
  // CT::string fromUnits = proc->attr.from_units;
  // CDBDebug("execute numDataPoints");
  // for (const auto dataObject: dataSource->dataObjects) {
  //   if (dataObject->cdfVariable->needsDataConversion_) {
  //     double noDataValue = dataObject->dfNodataValue;
  //     if (std::isnan(noDataValue)) {
  //       for (size_t cnt = 0; cnt < numDataPoints; cnt++) {
  //         double value = data[cnt];
  //         if (!std::isnan(value)) {
  //           data[cnt] = a * value + b;
  //         }
  //       }
  //     } else {
  //       for (size_t cnt = 0; cnt < numDataPoints; cnt++) {
  //         double value = data[cnt];
  //         if (!std::isnan(value)) {
  //           if (value != noDataValue) {
  //             data[cnt] = a * value + b;
  //           }
  //         }
  //       }
  //     }
  //   }
  //   dataObject->cdfVariable->needsDataConversion_ = false;
  // }

  // return 0;
}