#include "CDataPostProcessor_ConvertUnits.h"
#include <map>
#include <vector>
#include <cstddef>
#include <cstring>
#include <CCDFTypes.h>

#define CDATAPOSTPROCESSOR_CONVERTUNITS_ID "convert_units"
#define CDATAPOSTPROCESSOR_TOKNOTS_ID "toknots"
#define CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID "windspeed_knots_to_ms"
#define CDATAPOSTPROCESSOR_AXPLUSB_ID "ax+b"

bool adagucPostProcVerboseLog = false;
struct LookupUnits {
  double a;
  double b;
  std::string units;
  std::string from_units;
};

std::map<std::string, LookupUnits> lookUp = {{CDATAPOSTPROCESSOR_TOKNOTS_ID, {.a = 3600 / 1852., .b = 0, .units = "kts", .from_units = "m/s,m s-1"}},
                                             {CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID, {.a = 1852. / 3600., .b = 0, .units = "m s-1", .from_units = "kts,knots"}}};

std::vector<std::string> listOfProcs = {CDATAPOSTPROCESSOR_CONVERTUNITS_ID, CDATAPOSTPROCESSOR_TOKNOTS_ID, CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID, CDATAPOSTPROCESSOR_AXPLUSB_ID};

std::string getDataPostProcId(CServerConfig::XMLE_DataPostProc *proc) {
  return CT::printf("%s_%03d_%s_NEEDSCONVERSION", ADAGUCPOSTPROC_ATTR_PREFIX, proc->attr.postProcIndexInLayer, proc->attr.algorithm.c_str());
}

const char *CDPPConvertUnits::getId() { return CDATAPOSTPROCESSOR_CONVERTUNITS_ID; }

int CDPPConvertUnits::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int mode) {
  auto algorithm = proc->attr.algorithm;
  // Check if the algorithm matches one of the available id's
  if (std::find_if(listOfProcs.begin(), listOfProcs.end(), [&algorithm](const std::string &str) { return algorithm == str; }) != listOfProcs.end()) {
    if ((mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) || (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING)) {
      return CDATAPOSTPROCESSOR_RUNBEFOREREADING | CDATAPOSTPROCESSOR_RUNAFTERREADING;
    }
  }

  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}
LookupUnits getLookupUnits(CServerConfig::XMLE_DataPostProc &proc) {
  // If the ID is recognized in the lookup, always return those a an b values (ignore the configured a and b )
  if (lookUp.find(proc.attr.algorithm) != lookUp.end()) {
    auto lookedUp = lookUp[proc.attr.algorithm];
    if (!proc.attr.units.empty()) {

      lookedUp.units = proc.attr.units;
    }
    return lookedUp;
  }
  // Not in lookup, use the a an b from DataProcessor configuration
  LookupUnits l = {.a = 1, .b = 0, .units = "", .from_units = ""};
  if (!proc.attr.a.empty()) {
    l.a = proc.attr.a.toDouble();
  }
  if (!proc.attr.b.empty()) {
    l.b = proc.attr.b.toDouble();
  }
  if (!proc.attr.units.empty()) {
    l.units = proc.attr.units;
  }
  if (!proc.attr.from_units.empty()) {
    l.from_units = proc.attr.from_units;
  }
  if (adagucPostProcVerboseLog) {
    CDBDebug("Using own: %s %f %f %s <= %s", proc.attr.algorithm.c_str(), l.a, l.b, l.units.c_str(), l.from_units.c_str());
  }
  return l;
}

template <typename T> void do_convert(double noDataValue, size_t l, T *src, double a, double b) {
  if (std::isnan(noDataValue)) {
    for (size_t cnt = 0; cnt < l; cnt++) {
      T value = *src;
      if (!std::isnan(value)) {
        *src = a * value + b;
      }
      src++;
    }
  } else {
    for (size_t cnt = 0; cnt < l; cnt++) {
      T value = *src;
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
    auto lookupUnit = getLookupUnits(*proc);

    auto fromUnits = CT::split(lookupUnit.from_units, ",");
    for (const auto dataObject: dataSource->dataObjects) {

      if (fromUnits.empty() || std::find(fromUnits.begin(), fromUnits.end(), dataObject->getUnits().c_str()) != fromUnits.end()) {
        if (adagucPostProcVerboseLog) {
          CDBDebug("BEFORE: %s %f %f %s %s", proc->attr.algorithm.c_str(), lookupUnit.a, lookupUnit.b, lookupUnit.units.c_str(), lookupUnit.from_units.c_str());
        }
        dataObject->cdfVariable->setAttributeText((std::string(ADAGUCPOSTPROC_ATTR_PREFIX) + "_WAS_UNITS").c_str(), dataObject->getUnits());
        dataObject->cdfVariable->setAttributeText(getDataPostProcId(proc).c_str(), "true");
        CDBDebug("[Unit Conversion] --> [%s] Changing unit from [%s] to [%s]", proc->attr.algorithm.c_str(), dataObject->getUnits().c_str(), lookupUnit.units.c_str());
        dataObject->setUnits(lookupUnit.units.c_str());
        // Also set the unit directly in the datamodel.
        dataObject->cdfVariable->setAttributeText("units", lookupUnit.units.c_str());
      }
    }
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    auto lookupUnit = getLookupUnits(*proc);

    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    for (const auto dataObject: dataSource->dataObjects) {

      auto attrNeedsConversion = dataObject->cdfVariable->getAttributeNE(getDataPostProcId(proc).c_str());
      // Check if we need to convert the data
      if (attrNeedsConversion != nullptr && attrNeedsConversion->toString().equals("true")) {
        CDBDebug("AFTER (grid): %s %f %f %s", proc->attr.algorithm.c_str(), lookupUnit.a, lookupUnit.b, lookupUnit.units.c_str());
        CDFType type = dataObject->cdfVariable->getType();

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE)                                                                                                                                                          \
  if (type == CDFTYPE) do_convert<CPPTYPE>(dataObject->hasNodataValue ? dataObject->dfNodataValue : NAN, l, (CPPTYPE *)dataObject->cdfVariable->data, lookupUnit.a, lookupUnit.b);
        ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE

        // Convert point data if needed
        size_t nrPoints = dataObject->points.size();
        float noDataValue = dataObject->hasNodataValue ? dataObject->dfNodataValue : NAN;

        for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
          if (!(dataObject->points[pointNo].v == noDataValue)) {
            dataObject->points[pointNo].v = lookupUnit.a * dataObject->points[pointNo].v + lookupUnit.b;
          }
        }
      }
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
    if (adagucPostProcVerboseLog) {
      CDBDebug("Using var %s", dataObject->cdfVariable->name.c_str());
    }
    auto attrNeedsConversion = dataObject->cdfVariable->getAttributeNE(getDataPostProcId(proc).c_str());
    // Check if we need to convert the data
    if (attrNeedsConversion != nullptr && attrNeedsConversion->toString().equals("true")) {
      if (adagucPostProcVerboseLog) {
        CDBDebug("AFTER (timeseries): %s %f %f %s", proc->attr.algorithm.c_str(), lookupUnit.a, lookupUnit.b, lookupUnit.units.c_str());
      }
      do_convert<double>(dataObject->hasNodataValue ? dataObject->dfNodataValue : NAN, l, (double *)data, lookupUnit.a, lookupUnit.b);
    }
  }
  return 0;
}