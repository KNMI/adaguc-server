#include "CTime.h"

void CConvertADAGUCPoint_convert_BIRA_IASB_NETCDF(CDFObject *cdfObject) {
  try {
    if (cdfObject->getAttributeThrows("source")->toString() == "BIRA-IASB NETCDF" && cdfObject->getVariableNE("obs") == NULL) {
      std::string timeString = cdfObject->getAttributeThrows("measurement_time")->toString();
      cdfObject->setAttributeText("featureType", "point");
      CDF::Variable *time = cdfObject->getVariableThrows("time");
      CDF::Dimension *dim = cdfObject->getDimensionThrows("time");
      dim->name = "obs";
      time->name = "obs";

      CDF::Dimension *realTimeDim;
      CDF::Variable *realTimeVar;

      realTimeDim = new CDF::Dimension();
      realTimeDim->name = "time";
      realTimeDim->setSize(1);
      cdfObject->addDimension(realTimeDim);
      realTimeVar = new CDF::Variable();
      realTimeVar->setType(CDF_DOUBLE);
      realTimeVar->name = ("time");
      realTimeVar->setAttributeText("standard_name", "time");
      realTimeVar->setAttributeText("units", "seconds since 1970-01-01 0:0:0");
      realTimeVar->isDimension = true;
      realTimeVar->dimensionlinks.push_back(realTimeDim);
      cdfObject->addVariable(realTimeVar);
      realTimeVar->allocateData(realTimeDim->length);
      CTime ctime;
      ctime.init("seconds since 1970-01-01 0:0:0", "");
      ((double *)realTimeVar->data)[0] = ctime.dateToOffset(ctime.freeDateStringToDate(timeString.c_str()));
      for (size_t v = 0; v < cdfObject->variables.size(); v++) {
        CDF::Variable *var = cdfObject->variables[v];
        if (var->isDimension == false) {
          if (var->name != "time2D" && var->name != "time" && var->name != "lon" && var->name != "lat" && var->name != "x" && var->name != "y" &&
              var->name != "lat_bnds" && var->name != "lon_bnds" && var->name != "custom" && var->name != "projection" && var->name != "product" &&
              var->name != "iso_dataset" && var->name != "tile_properties" && var->name != "forecast_reference_time") {
            var->dimensionlinks.push_back(realTimeDim);
          }
        }
      }
    }
  } catch (int e) {
  }
}
