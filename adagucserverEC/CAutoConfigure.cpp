#include "CAutoConfigure.h"

#include "CDBFactory.h"
#include "CDataReader.h"
#include "CReporter.h"
#include "CRequest.h"
const char *CAutoConfigure::className = "CAutoConfigure";

// #define CAUTOCONFIGURE_DEBUG

int CAutoConfigure::checkCascadedDimensions(const CDataSource *dataSource) {
  if (dataSource != NULL && dataSource->dLayerType == CConfigReaderLayerTypeCascaded) {
#ifdef CAUTOCONFIGURE_DEBUG
    CDBDebug("Cascaded layers cannot have dimensions at the moment");
#endif

    CREPORT_ERROR_NODOC(CT::string("Cascaded layer cannot have dimensions at the moment"), CReportMessage::Categories::GENERAL);

    return 0;
  }

  return 1;
}

int CAutoConfigure::autoConfigureDimensions(CDataSource *dataSource) {

#ifdef CAUTOCONFIGURE_DEBUG
  CDBDebug("[autoConfigureDimensions]");
#endif

  if (!checkCascadedDimensions(dataSource)) {
    return 0;
  }

  /* Auto configure dimensions, in case they are not configured by the user.
   * Dimension configuration is added to the internal XML configuration structure.
   */
  if (dataSource->cfgLayer->Dimension.size() > 0) {
#ifdef CAUTOCONFIGURE_DEBUG
    CDBDebug("[OK] Dimensions are already configured.");
#endif

    return 0;
  }
  if (dataSource == NULL) {
    CDBDebug("datasource == NULL");
    return 1;
  }
  if (dataSource->cfgLayer == NULL) {
    CDBDebug("datasource->cfgLayer == NULL");
    return 1;
  }

  if (dataSource->cfgLayer->FilePath.size() != 1 && dataSource->cfgLayer->FilePath[0] != nullptr) {
    CDBDebug("(dataSource->cfgLayer->FilePath.size() != 1");
    return 1;
  }

  /**
   * Load dimension information about the layer from the autoconfigure_dimensions table
   * This table stores only layerid, netcdf dimname, adaguc dimname and units
   * Actual dimension values are not storen in this table
   */
  CT::string query;
  CT::string autoconfigureDimensionsTable = "autoconfigure_dimensions";

  CT::string layerTableId;
  try {
    auto dbAdapter = CDBFactory::getDBAdapter(dataSource->srvParams->cfg);
    if (dbAdapter == nullptr) {
      CDBError("Unable to get a getDBAdapter");
      return 1;
    }
    layerTableId = dbAdapter->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), NULL, dataSource);

  } catch (int e) {
    CDBError("Unable to get layerTableId for autoconfigure_dimensions");
    return 1;
  }

  CT::string layerIdentifier = dataSource->getLayerName();
  CDBStore::Store *store = CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->getDimensionInfoForLayerTableAndLayerName(layerTableId.c_str(), layerIdentifier.c_str());
  if (store != NULL) {
    try {

      for (size_t j = 0; j < store->size(); j++) {
        CServerConfig::XMLE_Dimension *xmleDim = new CServerConfig::XMLE_Dimension();

        xmleDim->value.copy(store->getRecord(j)->get("ogcname")->c_str());
        xmleDim->attr.name.copy(store->getRecord(j)->get("ncname")->c_str());
        xmleDim->attr.units.copy(store->getRecord(j)->get("units")->c_str());
        dataSource->cfgLayer->Dimension.push_back(xmleDim);
#ifdef CAUTOCONFIGURE_DEBUG
        CDBDebug("[OK] From DB: Retrieved dim %s-%s for layer %s", xmleDim->value.c_str(), xmleDim->attr.name.c_str(), layerTableId.c_str());
#endif
      }
      size_t storeSize = store->size();
      delete store;
      if (storeSize > 0) {
        dataSource->dimsAreAutoConfigured = true;
        return 0;
      }
    } catch (int e) {
      delete store;
      CDBError("DB Exception: %s\n", CDBStore::getErrorMessage(e));
    }
  }

#ifdef CAUTOCONFIGURE_DEBUG
  CDBDebug("[BUSY] AutoConfigureDimensions information not in table %s", autoconfigureDimensionsTable.c_str());
#endif

  /* Dimension information is not available in the database. We need to load it from a file.*/
  int status = justLoadAFileHeader(dataSource);
  if (status != 0) {
    if (dataSource->cfgLayer->FilePath.size() > 0) {
      CDBDebug("Unable to Path %s", dataSource->cfgLayer->FilePath[0]->value.c_str());
    } else {
      CDBError("Layer configuration error");
    }
    CDBError("justLoadAFileHeader failed");
    return 1;
  }

  try {
    if (dataSource->getNumDataObjects() == 0) {
      CDBDebug("dataSource->getNumDataObjects()==0");
      throw(__LINE__);
    }
    if (dataSource->getDataObject(0)->cdfVariable == NULL) {
      CDBDebug("dataSource->getDataObject(0)->cdfVariable==NULL");
      throw(__LINE__);
    }

    if (dataSource->cfgLayer->Dimension.size() == 0) {

      /*
       * No dimensions are configured by the user, try to detect them from the netcdf file automatically
       * and store them in the table.
       */
      if (dataSource->getDataObject(0)->cdfVariable->dimensionlinks.size() >= 2) {
        /* Create the database table */

        CDF::Variable *variable = dataSource->getDataObject(0)->cdfVariable;
        /* When there are no extra dims besides x and y we can skip
         * Each time to find the non existing dims. */
        if (variable->dimensionlinks.size() == 2) {
          CServerConfig::XMLE_Dimension *xmleDim = new CServerConfig::XMLE_Dimension();
          xmleDim->value.copy("0");
          xmleDim->attr.name.copy("none");
          xmleDim->attr.units.copy("none");
          dataSource->cfgLayer->Dimension.push_back(xmleDim);
#ifdef CAUTOCONFIGURE_DEBUG
          CDBDebug("Creating an empty table, because variable [%s] has only x and y dims", variable->name.c_str());
#endif
          CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->storeDimensionInfoForLayerTableAndLayerName(layerTableId.c_str(), dataSource->getLayerName(), "none", "none", "none");
          dataSource->dimsAreAutoConfigured = true;
          return 0;
        }

        for (size_t d = 0; d < variable->dimensionlinks.size() - 2; d++) {

          CDF::Dimension *dim = variable->dimensionlinks[d];
          if (dim != NULL) {
            CDF::Variable *dimVar = NULL;
            try {
              dimVar = dataSource->getDataObject(0)->cdfObject->getVariable(dim->name.c_str());
            } catch (int e) {
              CDBDebug("Warning: Variable is not defined for dimension [%s], creating array", dim->name.c_str());
              dimVar = CDataReader::addBlankDimVariable(dataSource->getDataObject(0)->cdfObject, dim->name.c_str());
              if (dimVar == NULL) {
                CDBError("Unable to add dimension variable for dimension %s", dim->name.c_str());
                return 1;
              }
            }

            CT::string units = "";

            CT::string netcdfdimname = dim->name.c_str();

            CT::string OGCDimName;

            try {
              dimVar->getAttribute("units")->getDataAsString(&units);
            } catch (int e) {
            }

            /* By default use the netcdf dimname */
            OGCDimName.copy(&netcdfdimname);

            /* Try to specify the OGC name based on dimtype */
            CDataReader::DimensionType dtype = CDataReader::getDimensionType(dataSource->getDataObject(0)->cdfObject, dimVar);
            if (dtype == CDataReader::dtype_time) OGCDimName = "time";
            if (dtype == CDataReader::dtype_reference_time) OGCDimName = "reference_time";
            if (dtype == CDataReader::dtype_member) OGCDimName = "member";
            if (dtype == CDataReader::dtype_elevation) OGCDimName = "elevation";

#ifdef CAUTOCONFIGURE_DEBUG
            CDBDebug("Datasource %s: Dim %s; units %s; netcdfdimname %s", dataSource->layerName.c_str(), dim->name.c_str(), units.c_str(), netcdfdimname.c_str());
#endif
            CServerConfig::XMLE_Dimension *xmleDim = new CServerConfig::XMLE_Dimension();
            dataSource->cfgLayer->Dimension.push_back(xmleDim);
            xmleDim->value.copy(OGCDimName.c_str());
            xmleDim->attr.name.copy(netcdfdimname.c_str());
            if (dtype == CDataReader::dtype_time || dtype == CDataReader::dtype_reference_time) {
              xmleDim->attr.units.copy("ISO8601");
            } else {
              xmleDim->attr.units.copy(units.c_str());
            }

            /* Store the data in the db for quick access. */
            CDBFactory::getDBAdapter(dataSource->srvParams->cfg)
                ->storeDimensionInfoForLayerTableAndLayerName(layerTableId.c_str(), dataSource->getLayerName(), xmleDim->attr.name.c_str(), OGCDimName.c_str(), xmleDim->attr.units.c_str());
#ifdef CAUTOCONFIGURE_DEBUG
            CDBDebug("[OK] From DB: Stored dim %s-%s for layer %s", xmleDim->value.c_str(), xmleDim->attr.name.c_str(), layerTableId.c_str());
#endif

          } else {
            CDBDebug("variable->dimensionlinks[d]");
          }
        }

        /* Check for global variable with standard_name forecast_reference_time */
        CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
        for (size_t j = 0; j < cdfObject->variables.size(); j++) {
          try {
            if (cdfObject->variables[j]->getAttribute("standard_name")->toString().equals("forecast_reference_time") == true) {
              CDBDebug("Found forecast_reference_time variable with name [%s]", cdfObject->variables[j]->name.c_str());
              CT::string units = "";
              try {
                cdfObject->variables[j]->getAttribute("units")->toString();
              } catch (int e) {
                CDBError("No units found for forecast_reference_time variable");
                throw(e);
              }

              /* But only add if it is not already added */
              bool forecastRefererenceIsAlreadyThere = false;
              for (size_t check = 0; check < dataSource->cfgLayer->Dimension.size(); check += 1) {
                if (dataSource->cfgLayer->Dimension[check]->value.equals("reference_time")) {
                  CDBDebug("Found forecast_reference_time variable with name [%s], but it is already configured.", cdfObject->variables[j]->name.c_str());
                  forecastRefererenceIsAlreadyThere = true;
                  break;
                }
              }
              if (!forecastRefererenceIsAlreadyThere) {
                CServerConfig::XMLE_Dimension *xmleDim = new CServerConfig::XMLE_Dimension();
                dataSource->cfgLayer->Dimension.push_back(xmleDim);
                xmleDim->value.copy("reference_time");
                xmleDim->attr.name.copy(cdfObject->variables[j]->name.c_str());
                xmleDim->attr.units.copy(units.c_str());
                /* Store the data in the db for quick access. */
                CDBFactory::getDBAdapter(dataSource->srvParams->cfg)
                    ->storeDimensionInfoForLayerTableAndLayerName(layerTableId.c_str(), layerIdentifier.c_str(), xmleDim->attr.name.c_str(), "reference_time", xmleDim->attr.units.c_str());
#ifdef CAUTOCONFIGURE_DEBUG
                CDBDebug("[OK] From DB: Stored dim %s-%s for layer %s", xmleDim->value.c_str(), xmleDim->attr.name.c_str(), layerTableId.c_str());
#endif
              }
            }
          } catch (int e) {
          }
        }
      }
    }
  } catch (int linenr) {
    CDBError("[ERROR] Exception at line %d", linenr);

    return 1;
  }
#ifdef CAUTOCONFIGURE_DEBUG
  CDBDebug("/[DONE] Done AutoConfigureDimensions");
#endif
  dataSource->dimsAreAutoConfigured = true;
  return 0;
}

int CAutoConfigure::autoConfigureStyles(CDataSource *dataSource) {

  if (dataSource->dLayerType == CConfigReaderLayerTypeCascaded) {
#ifdef CAUTOCONFIGURE_DEBUG
    CDBDebug("Cascaded layers cannot have styles at the moment");
#endif
    return 0;
  }

#ifdef CAUTOCONFIGURE_DEBUG
  CDBDebug("[AutoConfigureStyles]");
#endif
  if (dataSource == NULL) {
    CDBDebug("datasource == NULL");
    return 1;
  }
  if (dataSource->cfgLayer == NULL) {
    CDBDebug("datasource->cfgLayer == NULL");
    return 1;
  }
  if (dataSource->cfgLayer->Styles.size() != 0) {
    /* Configured by user, auto styles is not required */
    return 0;
  };

  if (dataSource->cfgLayer->Legend.size() != 0) {
    /* Configured by user, auto styles is not required */
    return 0;
  };

  /* Try to find a style corresponding the the standard_name attribute of the file. */
  CServerConfig::XMLE_Styles *xmleStyle = new CServerConfig::XMLE_Styles();
  dataSource->cfgLayer->Styles.push_back(xmleStyle);

  CT::string tableName = "autoconfigure_styles";
  CT::string layerTableId;
  try {
    layerTableId = CDBFactory::getDBAdapter(dataSource->srvParams->cfg)
                       ->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), NULL, dataSource);
  } catch (int e) {
    CDBError("Unable to get layerTableId for autoconfigure_styles");
    return 1;
  }

  /* Auto style is not available in the database, so look it up in the file.*/

  /* If the file header is not yet loaded, load it.*/
  if (dataSource->getDataObject(0)->cdfVariable == NULL) {
    int status = justLoadAFileHeader(dataSource);
    if (status != 0) {
      CDBError("unable to load datasource headers");
      return 1;
    }
  }

  /* Get the searchname, based on variable, standard name or long name.*/
  CT::string searchStandardName = dataSource->getDataObject(0)->variableName.c_str();

  CT::string searchVariableName = dataSource->getDataObject(0)->variableName.c_str();

  try {
    if (dataSource->getDataObject(0)->cdfVariable == nullptr) {
      throw __LINE__;
    }
    dataSource->getDataObject(0)->cdfVariable->getAttribute("standard_name")->getDataAsString(&searchStandardName);
  } catch (int e) {
  }

  searchStandardName.toLowerCaseSelf();
  searchVariableName.toLowerCaseSelf();

  /* Get the units */
  CT::string dataSourceUnits;
  if (dataSource->getDataObject(0)->getUnits().length() > 0) {
    dataSourceUnits = dataSource->getDataObject(0)->getUnits().c_str();
  }
  dataSourceUnits.toLowerCaseSelf();

#ifdef CAUTOCONFIGURE_DEBUG
  CDBDebug("Retrieving auto styles by using fileinfo \"%s\"", searchStandardName.c_str());
#endif
  /* We now have the keyword searchname, with this keyword we are going to lookup all StandardName's in the server configured Styles */

  std::vector<CT::string> styleList;

  for (size_t j = 0; j < dataSource->cfg->Style.size(); j++) {

    const char *styleName = dataSource->cfg->Style[j]->attr.name.c_str();
#ifdef CAUTOCONFIGURE_DEBUG
    CDBDebug("Searching Style \"%s\"", styleName);
#endif
    if (styleName != NULL) {
      for (size_t i = 0; i < dataSource->cfg->Style[j]->StandardNames.size(); i++) {

        CT::string standard_name = "*";
        CT::string variable_name = "*";
        CT::string units;

        if (dataSource->cfg->Style[j]->StandardNames[i]->attr.standard_name.empty() == false) {
          standard_name.copy(dataSource->cfg->Style[j]->StandardNames[i]->attr.standard_name.c_str());
          standard_name.toLowerCaseSelf();
        }

        if (dataSource->cfg->Style[j]->StandardNames[i]->attr.variable_name.empty() == false) {
          variable_name.copy(dataSource->cfg->Style[j]->StandardNames[i]->attr.variable_name.c_str());
          variable_name.toLowerCaseSelf();
        }

        if (dataSource->cfg->Style[j]->StandardNames[i]->attr.units.empty() == false) {

          units.copy(dataSource->cfg->Style[j]->StandardNames[i]->attr.units.c_str());
        }
        units.toLowerCaseSelf();

#ifdef CAUTOCONFIGURE_DEBUG
        CDBDebug("Searching StandardNames \"%s\"", standard_name.c_str());
#endif
        if (standard_name.length() > 0) {
          CT::StackList<CT::string> standardNameList;

          if (standard_name.charAt(0) == '^') {
            standardNameList.push_back(standard_name);
          } else {
            standardNameList = standard_name.splitToStack(",");
          }

          for (size_t n = 0; n < standardNameList.size(); n++) {
            bool variableNameMatch = false;
            if (searchVariableName.equals(variable_name.c_str()) || variable_name.equals("*")) {
              variableNameMatch = true;
            } else if (variable_name.charAt(0) == '^') {
              if (searchVariableName.testRegEx(variable_name.c_str())) {
                variableNameMatch = true;
              }
            }

            bool standardNameMatch = false;
            if (searchStandardName.equals(standardNameList[n].c_str()) || standardNameList[n].equals("*")) {
              standardNameMatch = true;
            } else if (standardNameList[n].charAt(0) == '^') {
              /* Regex */
              if (searchStandardName.testRegEx(standardNameList[n].c_str())) {
                standardNameMatch = true;
              }
            }
            if (standardNameMatch && variableNameMatch) {
              /* StandardName matches. */
              bool unitsMatch = true;
              if (dataSourceUnits.length() != 0 && units.length() != 0) {
                unitsMatch = false;
                /* Test for same units */
                if (dataSourceUnits.equals(&units))
                  unitsMatch = true;
                else {
                  /* Test for regexp */
                  if (units.charAt(0) == '^') {
                    CDBDebug("Found regex %s", units.c_str());
                    if (dataSourceUnits.testRegEx(units.c_str())) {
                      unitsMatch = true;
                    }
                  }
                }
              }
              if (unitsMatch) {
#ifdef CAUTOCONFIGURE_DEBUG
                CDBDebug("*** Match: \"%s\"== \"%s\"", searchStandardName.c_str(), standardNameList[n].c_str());
#endif
                styleList.push_back(dataSource->cfg->Style[j]->attr.name.c_str());
              }
            }
          }
        }
      }
    }
  }

  CT::string styles = "";

  for (size_t j = 0; j < styleList.size(); j++) {
    if (styles.length() != 0) styles.concat(",");
    styles.concat(styleList[j].c_str());
  }

  if (styles.length() == 0) styles = "auto";

  xmleStyle->value.copy(styles.c_str());
#ifdef CAUTOCONFIGURE_DEBUG
  CDBDebug("/[DONE] [AutoConfigureStyles]");
#endif
  return 0;
}

int CAutoConfigure::getFileNameForDataSource(CDataSource *dataSource, std::string &fileName) {

  CT::string foundFileName = dataSource->getFileName();
  if (foundFileName.empty()) {
    /* Use the file specified as header file */
    foundFileName = dataSource->headerFileName.c_str();
  }
  if (foundFileName.empty()) {

    /* Try to get a file from DB */
    // CDBDebug("Looking up first file");

    /* ADAGUC-Server database queries don't work if there are no dimensions */
    bool removeRequiredDims = false;
    if (dataSource->requiredDims.size() == 0) {
      removeRequiredDims = true;
      if (dataSource->cfgLayer->Dimension.size() > 0) {
        try {
          CRequest::fillDimValuesForDataSource(dataSource, dataSource->srvParams);
        } catch (ServiceExceptionCode e) {
          CDBDebug("Unable to fillDimValuesForDataSource");
          return 1;
        }
      } else {
        CDBDebug("Required dims is still zero, add none now");
        COGCDims *ogcDim = new COGCDims();
        dataSource->requiredDims.push_back(ogcDim);
        ogcDim->name.copy("none");
        ogcDim->value.copy("0");
        ogcDim->queryValue.copy("0");
        ogcDim->netCDFDimName.copy("none");
      }
    }
    CDBStore::Store *store = CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->getFilesAndIndicesForDimensions(dataSource, 1, false);
    if (store != NULL && store->getSize() > 0) {
      CT::string fileNamestr = store->getRecord(0)->get(0)->c_str();
      // CDBDebug("fileName from DB: %s", fileNamestr.c_str());
      foundFileName = fileNamestr;
    }
    delete store;

    /* Restoremodifications to requiredDims*/
    if (removeRequiredDims) {
      for (size_t j = 0; j < dataSource->requiredDims.size(); j++) {
        delete dataSource->requiredDims[j];
      }
      dataSource->requiredDims.clear();
    }

    /* If still no file found, something is configured in the wrong way */
    if (foundFileName.empty()) {
      CDBError("No files found in the database, did you already update the database?");
      return 1;
    }
  }
  fileName = foundFileName.c_str();
  return 0;
}

int CAutoConfigure::justLoadAFileHeader(CDataSource *dataSource) {
  if (dataSource == NULL) {
    CDBError("datasource == NULL");
    return 1;
  }
  if (dataSource->cfgLayer == NULL) {
    CDBError("datasource->cfgLayer == NULL");
    return 1;
  }
  if (dataSource->getNumDataObjects() == 0) {
    CDBError("dataSource->getNumDataObjects()==0");
    return 1;
  }
  if (dataSource->getDataObject(0)->cdfVariable != NULL) {
#ifdef CAUTOCONFIGURE_DEBUG
    CDBDebug("already loaded: dataSource->getDataObject(0)->cdfVariable!=NULL");
#endif
    return 0;
  }

  std::string foundFileName;

  if (getFileNameForDataSource(dataSource, foundFileName) != 0) {
    CDBDebug("Unable to getFileNameForDataSource");
    return 1;
  }
  /* Open a file */
  try {
    // CDBDebug("Loading header [%s]", foundFileName.c_str());
    CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(dataSource, dataSource->srvParams, foundFileName.c_str());
    if (cdfObject == NULL) throw(__LINE__);
    dataSource->attachCDFObject(cdfObject);
  } catch (int linenr) {
    CDBError("Returning from line %d");
    dataSource->detachCDFObject();
    return 1;
  }

  return 0;
}
