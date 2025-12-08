
void CConvertADAGUCPoint_convert_BIRA_IASB_NETCDF(CDFObject *cdfObject) {
  try {
    if (cdfObject->getAttribute("source")->toString().equals("BIRA-IASB NETCDF") && cdfObject->getVariableNE("obs") == NULL) {
      CT::string timeString = cdfObject->getAttribute("measurement_time")->toString();
      cdfObject->setAttributeText("featureType", "point");
      CDF::Variable *time = cdfObject->getVariable("time");
      CDF::Dimension *dim = cdfObject->getDimension("time");
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
      realTimeVar->name.copy("time");
      realTimeVar->setAttributeText("standard_name", "time");
      realTimeVar->setAttributeText("units", "seconds since 1970-01-01 0:0:0");
      realTimeVar->isDimension = true;
      realTimeVar->dimensionlinks.push_back(realTimeDim);
      cdfObject->addVariable(realTimeVar);
      CDF::allocateData(CDF_DOUBLE, &realTimeVar->data, realTimeDim->length);
      CTime ctime;
      ctime.init("seconds since 1970-01-01 0:0:0", NULL);
      ((double *)realTimeVar->data)[0] = ctime.dateToOffset(ctime.freeDateStringToDate(timeString));
      for (size_t v = 0; v < cdfObject->variables.size(); v++) {
        CDF::Variable *var = cdfObject->variables[v];
        if (var->isDimension == false) {
          if (!var->name.equals("time2D") && !var->name.equals("time") && !var->name.equals("lon") && !var->name.equals("lat") && !var->name.equals("x") && !var->name.equals("y") &&
              !var->name.equals("lat_bnds") && !var->name.equals("lon_bnds") && !var->name.equals("custom") && !var->name.equals("projection") && !var->name.equals("product") &&
              !var->name.equals("iso_dataset") && !var->name.equals("tile_properties") && !var->name.equals("forecast_reference_time")) {
            var->dimensionlinks.push_back(realTimeDim);
          }
        }
      }
      // CDBDebug("%s", CDF::dump(cdfObject).c_str());
    }
  } catch (int e) {
  }
}
