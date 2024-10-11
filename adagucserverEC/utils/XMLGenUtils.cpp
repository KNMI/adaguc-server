#include <CXMLGen.h>
#include "LayerMetadataStore.h"
#include "XMLGenUtils.h"
#include <CDBStore.h>
#include <CDBFactory.h>
#include <LayerTypeLiveUpdate/LayerTypeLiveUpdate.h>
#include "LayerUtils.h"

int populateMetadataLayerStruct(MetadataLayer *metadataLayer, bool readFromDB) {
  metadataLayer->readFromDb = readFromDB;
  if (!metadataLayer->srvParams->useMetadataTable()) {
    metadataLayer->readFromDb = false;
  }

  // Make the layer name
  CT::string layerUniqueName;
  if (makeUniqueLayerName(&layerUniqueName, metadataLayer->layer) != 0) {
    metadataLayer->hasError = true;
    return 1;
  }
  metadataLayer->layerMetadata.name.copy(&layerUniqueName);

  // Create and datasource
  if (metadataLayer->dataSource == NULL) {
    metadataLayer->dataSource = new CDataSource();
    if (metadataLayer->dataSource->setCFGLayer(metadataLayer->srvParams, metadataLayer->srvParams->configObj->Configuration[0], metadataLayer->layer, metadataLayer->layerMetadata.name.c_str(), -1) !=
        0) {
      return 1;
    }
  }

  // Make the group
  CT::string layerGroup = "";
  if (metadataLayer->layer->Group.size() > 0) {
    if (metadataLayer->layer->Group[0]->attr.value.empty() == false) {
      layerGroup.copy(metadataLayer->layer->Group[0]->attr.value.c_str());
    }
  }
  metadataLayer->layerMetadata.group.copy(&layerGroup);

  // Check if this layer is querable
  int datasetRestriction = CServerParams::checkDataRestriction();
  if ((datasetRestriction & ALLOW_GFI)) {
    metadataLayer->layerMetadata.isQueryable = 1;
  }

  // Get Abstract
  if (metadataLayer->dataSource->cfgLayer->Abstract.size() > 0) {
    metadataLayer->layerMetadata.abstract = metadataLayer->dataSource->cfgLayer->Abstract[0]->value;
  }

  // Fill in Layer title, with fallback to Name (later this can be set based on metadata or info from the file)
  if (metadataLayer->dataSource->cfgLayer->Title.size() != 0) {
    metadataLayer->layerMetadata.title.copy(metadataLayer->dataSource->cfgLayer->Title[0]->value.c_str());
  } else {
    metadataLayer->layerMetadata.title.copy(metadataLayer->dataSource->cfgLayer->Name[0]->value.c_str());
  }
  bool readFileInfo = readFromDB ? (loadLayerMetadataStructFromMetadataDb(metadataLayer) != 0) : true;
  if (readFileInfo) {
    metadataLayer->readFromDb = false;
    // Get a default file name for this layer to obtain some information
    int status = getFileNameForLayer(metadataLayer);
    if (status != 0) {
      metadataLayer->hasError = 1;
      return 1;
    }

    if (metadataLayer->dataSource->timeSteps.size() == 0) {
      metadataLayer->dataSource->addStep(metadataLayer->fileName.c_str(), NULL);
    }

    if (getTitleForLayer(metadataLayer) != 0) {
      metadataLayer->hasError = 1;
      return 1;
    }

    CDataReader reader;
    status = reader.open(metadataLayer->dataSource, CNETCDFREADER_MODE_OPEN_DIMENSIONS);
    if (status != 0) {
      CDBError("Could not open file: %s", metadataLayer->dataSource->getFileName());
      return 1;
    }

    metadataLayer->layerMetadata.dfBBOX[0] = metadataLayer->dataSource->dfBBOX[0];
    metadataLayer->layerMetadata.dfBBOX[1] = metadataLayer->dataSource->dfBBOX[1];
    metadataLayer->layerMetadata.dfBBOX[2] = metadataLayer->dataSource->dfBBOX[2];
    metadataLayer->layerMetadata.dfBBOX[3] = metadataLayer->dataSource->dfBBOX[3];

    metadataLayer->layerMetadata.width = metadataLayer->dataSource->dWidth;
    metadataLayer->layerMetadata.height = metadataLayer->dataSource->dHeight;
    metadataLayer->layerMetadata.cellsizeX = metadataLayer->dataSource->dfCellSizeX;
    metadataLayer->layerMetadata.cellsizeY = metadataLayer->dataSource->dfCellSizeY;
    metadataLayer->layerMetadata.nativeEPSG = metadataLayer->dataSource->nativeEPSG;
    metadataLayer->layerMetadata.projstring = metadataLayer->dataSource->nativeProj4;

    auto v = metadataLayer->dataSource->getDataObjectsVector();
    for (auto d : (*v)) {
      CDF::Attribute *longName = d->cdfVariable->getAttributeNE("long_name");
      if (longName == nullptr) {
        longName = d->cdfVariable->getAttributeNE("standard_name");
      }
      CT::string label = longName != nullptr ? longName->getDataAsString() : d->variableName;
      LayerMetadataVariable layerMetadataVariable = {
          .variableName = d->cdfVariable->name,
          .units = d->getUnits(),
          .label = label,
      };
      metadataLayer->layerMetadata.variableList.push_back(layerMetadataVariable);
    }
  }
  if (getDimsForLayer(metadataLayer) != 0) {
    metadataLayer->hasError = 1;
    return 1;
  }

  // CDBDebug("getProjectionInformationForLayer");
  if (getProjectionInformationForLayer(metadataLayer) != 0) {
    metadataLayer->hasError = 1;
    return 1;
  }

  // CDBDebug("getStylesForLayer");
  if (getStylesForLayer(metadataLayer) != 0) {
    metadataLayer->hasError = 1;
    return 1;
  }

  std::map<std::string, LayerMetadataProjection> projectionMap;
  // Make a unique list of projections
  for (const auto &p : metadataLayer->layerMetadata.projectionList) {
    projectionMap.emplace(p.name.c_str(), p);
  }
  metadataLayer->layerMetadata.projectionList.clear();
  for (const auto &p : projectionMap) {
    metadataLayer->layerMetadata.projectionList.push_back(p.second);
  }

  std::sort(metadataLayer->layerMetadata.projectionList.begin(), metadataLayer->layerMetadata.projectionList.end(), compareProjection);
  std::sort(metadataLayer->layerMetadata.dimList.begin(), metadataLayer->layerMetadata.dimList.end(), compareDim);

  return 0;
}

int getDimsForLayer(MetadataLayer *metadataLayer) {
#ifdef CXMLGEN_DEBUG
  CDBDebug("getDimsForLayer");
#endif
  char szMaxTime[32];
  char szMinTime[32];
  // char szInterval[32];
  // int hastimedomain = 0;

  // Dimensions
  if (metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeDataBase || metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeStyled) {
    if (loadLayerDimensionListFromMetadataDb(metadataLayer) == 0) {
      // CDBDebug("LayerMetadata: dimensionList information fetched!");
      return 0;
    }

#ifdef CXMLGEN_DEBUG
    CDBDebug("Start looping dimensions");
    CDBDebug("Number of dimensions is %d", metadataLayer->dataSource->cfgLayer->Dimension.size());
#endif
    /* Auto configure dimensions */
    for (size_t i = 0; i < metadataLayer->dataSource->cfgLayer->Dimension.size(); i++) {

      /* This dimension is a filetimedate type, its values come from the modification date of the file */
      if (metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.defaultV.equals("filetimedate")) {
        CT::string fileDate = CDirReader::getFileDate(metadataLayer->layer->FilePath[0]->value.c_str());

        LayerMetadataDim dim;
        dim.serviceName.copy("time");
        dim.cdfName.copy("time");
        dim.units.copy("ISO8601");
        dim.values.copy(fileDate.c_str());
        dim.defaultValue.copy(fileDate.c_str());
        dim.hasMultipleValues = true;
        dim.hidden = false;
        metadataLayer->layerMetadata.dimList.push_back(dim);
        break;
      }
#ifdef CXMLGEN_DEBUG
      CDBDebug("%d = %s / %s", i, metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.name.c_str(), metadataLayer->dataSource->cfgLayer->Dimension[i]->value.c_str());
#endif
      if (i == 0 && metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.name.equals("none")) break;
      // Shorthand dimName
      const char *pszDimName = metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.name.c_str();

      // Create a new dim to store in the layer
      LayerMetadataDim dim;
      dim.hidden = false;
      dim.serviceName.copy(metadataLayer->dataSource->cfgLayer->Dimension[i]->value.c_str());
      dim.cdfName.copy(metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.name.c_str());

      // Get the tablename
      CT::string tableName;
      CServerParams *srvParam = metadataLayer->dataSource->srvParams;
      try {
        tableName = CDBFactory::getDBAdapter(srvParam->cfg)
                        ->getTableNameForPathFilterAndDimension(metadataLayer->layer->FilePath[0]->value.c_str(), metadataLayer->layer->FilePath[0]->attr.filter.c_str(), pszDimName,
                                                                metadataLayer->dataSource);
      } catch (int e) {
        CDBError("Unable to create tableName from '%s' '%s' '%s'", metadataLayer->layer->FilePath[0]->value.c_str(), metadataLayer->layer->FilePath[0]->attr.filter.c_str(), pszDimName);
        return 1;
      }

      bool hasMultipleValues = false;
      bool isTimeDim = false;

      CDataReader reader;
      int status = reader.open(metadataLayer->dataSource, CNETCDFREADER_MODE_OPEN_DIMENSIONS);
      if (status != 0) {
        CDBError("Could not open file: %s", metadataLayer->dataSource->getFileName());
        return 1;
      }
      if (metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.interval.empty()) {
        hasMultipleValues = true;

        /* Automatically scan the time dimension, two types are avaible, start/stop/resolution and individual values */
        // TODO try to detect automatically the time resolution of the layer.
        CT::string varName = metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.name.c_str();
        // CDBDebug("VarName = [%s]",varName.c_str());
        int ind = varName.indexOf("time");
        if (ind >= 0) {
          // CDBDebug("VarName = [%s] and this is a time dim at %d!",varName.c_str(),ind);
          CT::string units;
          isTimeDim = true;
          try {
            metadataLayer->dataSource->getDataObject(0)->cdfObject->getVariable("time")->getAttribute("units")->getDataAsString(&units);

          } catch (int e) {
          }
          if (units.length() > 0) {
#ifdef CXMLGEN_DEBUG
            CDBDebug("Time dimension units = %s", units.c_str());
#endif

#ifdef MEASURETIME
            StopWatch_Stop("Get the first 100 values from the database, and determine whether the time resolution is continous or multivalue.");
#endif

            // Get the first 100 values from the database, and determine whether the time resolution is continous or multivalue.
            CDBStore::Store *store = CDBFactory::getDBAdapter(srvParam->cfg)->getUniqueValuesOrderedByValue(pszDimName, 100, true, tableName.c_str());
            bool dataHasBeenFoundInStore = false;
            if (store != NULL) {
              if (store->size() != 0) {
                dataHasBeenFoundInStore = true;
                tm tms[store->size()];

                try {

                  for (size_t j = 0; j < store->size(); j++) {
                    store->getRecord(j)->get("time")->setChar(10, 'T');
                    const char *isotime = store->getRecord(j)->get("time")->c_str();
#ifdef CXMLGEN_DEBUG
                    //                    CDBDebug("isotime = %s",isotime);
#endif
                    CT::string year, month, day, hour, minute, second;
                    year.copy(isotime + 0, 4);
                    tms[j].tm_year = year.toInt() - 1900;
                    month.copy(isotime + 5, 2);
                    tms[j].tm_mon = month.toInt() - 1;
                    day.copy(isotime + 8, 2);
                    tms[j].tm_mday = day.toInt();
                    hour.copy(isotime + 11, 2);
                    tms[j].tm_hour = hour.toInt();
                    minute.copy(isotime + 14, 2);
                    tms[j].tm_min = minute.toInt();
                    second.copy(isotime + 17, 2);
                    tms[j].tm_sec = second.toInt();
                  }
                  size_t nrTimes = store->size() - 1;
                  bool isConst = true;
                  if (store->size() < 4) {
                    isConst = false;
                  }
                  try {
                    CTime *time = CTime::GetCTimeInstance(metadataLayer->dataSource->getDataObject(0)->cdfObject->getVariable("time"));
                    if (time == nullptr) {
                      CDBDebug(CTIME_GETINSTANCE_ERROR_MESSAGE);
                      return 1;
                    }
                    if (time->getMode() != 0) {
                      isConst = false;
                    }
                  } catch (int e) {
                  }

                  CT::string iso8601timeRes = "P";
                  CT::string yearPart = "";
                  if (tms[1].tm_year - tms[0].tm_year != 0) {
                    if (tms[1].tm_year - tms[0].tm_year == (tms[nrTimes < 10 ? nrTimes : 10].tm_year - tms[0].tm_year) / double(nrTimes < 10 ? nrTimes : 10)) {
                      yearPart.printconcat("%dY", abs(tms[1].tm_year - tms[0].tm_year));
                    } else {
                      isConst = false;
#ifdef CXMLGEN_DEBUG
                      CDBDebug("year is irregular");
#endif
                    }
                  }
                  if (tms[1].tm_mon - tms[0].tm_mon != 0) {
                    if (tms[1].tm_mon - tms[0].tm_mon == (tms[nrTimes < 10 ? nrTimes : 10].tm_mon - tms[0].tm_mon) / double(nrTimes < 10 ? nrTimes : 10))
                      yearPart.printconcat("%dM", abs(tms[1].tm_mon - tms[0].tm_mon));
                    else {
                      isConst = false;
#ifdef CXMLGEN_DEBUG
                      CDBDebug("month is irregular");
#endif
                    }
                  }

                  if (tms[1].tm_mday - tms[0].tm_mday != 0) {
                    if (tms[1].tm_mday - tms[0].tm_mday == (tms[nrTimes < 10 ? nrTimes : 10].tm_mday - tms[0].tm_mday) / double(nrTimes < 10 ? nrTimes : 10))
                      yearPart.printconcat("%dD", abs(tms[1].tm_mday - tms[0].tm_mday));
                    else {
                      isConst = false;
#ifdef CXMLGEN_DEBUG
                      CDBDebug("day irregular");
                      for (size_t j = 0; j < nrTimes; j++) {
                        CDBDebug("Day %d = %d", j, tms[j].tm_mday);
                      }
#endif
                    }
                  }

                  CT::string hourPart = "";
                  if (tms[1].tm_hour - tms[0].tm_hour != 0) {
                    hourPart.printconcat("%dH", abs(tms[1].tm_hour - tms[0].tm_hour));
                  }
                  if (tms[1].tm_min - tms[0].tm_min != 0) {
                    hourPart.printconcat("%dM", abs(tms[1].tm_min - tms[0].tm_min));
                  }
                  if (tms[1].tm_sec - tms[0].tm_sec != 0) {
                    hourPart.printconcat("%dS", abs(tms[1].tm_sec - tms[0].tm_sec));
                  }

                  int sd = (tms[1].tm_hour * 3600 + tms[1].tm_min * 60 + tms[1].tm_sec) - (tms[0].tm_hour * 3600 + tms[0].tm_min * 60 + tms[0].tm_sec);
                  for (size_t j = 2; j < store->size() && isConst; j++) {
                    int d = (tms[j].tm_hour * 3600 + tms[j].tm_min * 60 + tms[j].tm_sec) - (tms[j - 1].tm_hour * 3600 + tms[j - 1].tm_min * 60 + tms[j - 1].tm_sec);
                    if (d > 0) {
                      if (sd != d) {
                        isConst = false;
#ifdef CXMLGEN_DEBUG
                        CDBDebug("hour/min/sec is irregular %d ", j);
#endif
                      }
                    }
                  }

                  // Check whether we found a time resolution
                  if (isConst == false) {
                    hasMultipleValues = true;
#ifdef CXMLGEN_DEBUG
                    CDBDebug("Not a continous time dimension, multipleValues required");
#endif
                  } else {
#ifdef CXMLGEN_DEBUG
                    CDBDebug("Continous time dimension, Time resolution needs to be calculated");
#endif
                    hasMultipleValues = false;
                  }

                  if (isConst) {
                    if (yearPart.length() > 0) {
                      iso8601timeRes.concat(&yearPart);
                    }
                    if (hourPart.length() > 0) {
                      iso8601timeRes.concat("T");
                      iso8601timeRes.concat(&hourPart);
                    }
#ifdef CXMLGEN_DEBUG
                    CDBDebug("Calculated a timeresolution of %s", iso8601timeRes.c_str());
#endif
                    metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.interval.copy(iso8601timeRes.c_str());
                    metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.units.copy("ISO8601");
                  }
                } catch (int e) {
                }
              }
              delete store;
              store = NULL;
            }
            if (dataHasBeenFoundInStore == false) {
              CDBDebug("No data available in database for dimension %s", pszDimName);
            }
          }
        }
      }
      CDBStore::Store *values = NULL;
      // This is a multival dim, defined as val1,val2,val3,val4,val5,etc...
      if (hasMultipleValues == true) {
        // Get all dimension values from the db
        if (isTimeDim) {
          values = CDBFactory::getDBAdapter(srvParam->cfg)->getUniqueValuesOrderedByValue(pszDimName, 0, true, tableName.c_str());
        } else {
          // query.print("select distinct %s,dim%s from %s order by dim%s,%s",pszDimName,pszDimName,tableName.c_str(),pszDimName,pszDimName);
          values = CDBFactory::getDBAdapter(srvParam->cfg)->getUniqueValuesOrderedByIndex(pszDimName, 0, true, tableName.c_str());
        }

        if (values == NULL) {
          CDBError("Query failed");
          return 1;
        }

        if (values->getSize() > 0) {
          // if(srvParam->requestType==REQUEST_WMS_GETCAPABILITIES)
          {

            dim.serviceName.copy(metadataLayer->dataSource->cfgLayer->Dimension[i]->value.c_str());
            dim.cdfName.copy(metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.name.c_str());

            // Try to get units from the variable
            dim.units.copy("NA");
            if (metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.units.empty()) {
              CT::string units;
              try {
                metadataLayer->dataSource->getDataObject(0)->cdfObject->getVariable(dim.cdfName.c_str())->getAttribute("units")->getDataAsString(&units);
                dim.units.copy(&units);
              } catch (int e) {
              }
            }

            dim.hasMultipleValues = 1;
            if (isTimeDim == true) {
              dim.units.copy("ISO8601");
              for (size_t j = 0; j < values->getSize(); j++) {
                // 2011-01-01T22:00:01Z
                // 01234567890123456789
                values->getRecord(j)->get(0)->setChar(10, 'T');
                if (values->getRecord(j)->get(0)->length() == 19) {
                  values->getRecord(j)->get(0)->printconcat("Z");
                }
              }
              dim.units.copy("ISO8601");
            }

            if (!metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.units.empty()) {
              // Units are configured in the configuration file.
              dim.units.copy(metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.units.c_str());
            }

            const char *pszDefaultV = metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.defaultV.c_str();
            CT::string defaultV;
            if (pszDefaultV != NULL) defaultV = pszDefaultV;

            if (defaultV.length() == 0 || defaultV.equals("max", 3)) {
              dim.defaultValue.copy(values->getRecord(values->getSize() - 1)->get(0)->c_str());
            } else if (defaultV.equals("min", 3)) {
              dim.defaultValue.copy(values->getRecord(0)->get(0)->c_str());
            } else {
              dim.defaultValue.copy(&defaultV);
            }

            dim.values.copy(values->getRecord(0)->get(0));
            for (size_t j = 1; j < values->getSize(); j++) {
              dim.values.printconcat(",%s", values->getRecord(j)->get(0)->c_str());
            }
          }
        }
        delete values;
      }

      // This is an interval defined as start/stop/resolution
      if (hasMultipleValues == false) {
        // Retrieve the max dimension value
        CDBStore::Store *values = CDBFactory::getDBAdapter(srvParam->cfg)->getMax(pszDimName, tableName.c_str());
        if (values == NULL) {
          CDBError("Query failed");
          return 1;
        }
        if (values->getSize() > 0) {
          snprintf(szMaxTime, 31, "%s", values->getRecord(0)->get(0)->c_str());
          szMaxTime[10] = 'T';
        }
        delete values;
        // Retrieve the minimum dimension value
        values = CDBFactory::getDBAdapter(srvParam->cfg)->getMin(pszDimName, tableName.c_str());
        if (values == NULL) {
          CDBError("Query failed");
          return 1;
        }
        if (values->getSize() > 0) {
          snprintf(szMinTime, 31, "%s", values->getRecord(0)->get(0)->c_str());
          szMinTime[10] = 'T';
        }
        delete values;

        // Retrieve all values for time position
        //    if(srvParam->serviceType==SERVICE_WCS){

        /*
      query.print("select %s from %s",dimName,tableName.c_str());
      values = DB.query_select(query.c_str(),0);
      if(values == NULL){CDBError("Query failed");DB.close();return 1;}
      if(values->count>0){
        if(TimePositions!=NULL){
          delete[] TimePositions;
          TimePositions=NULL;
        }
        TimePositions=new CT::string[values->count];
        char szTemp[32];
        for(size_t l=0;l<values->count;l++){
          snprintf(szTemp,31,"%s",values[l].c_str());szTemp[10]='T';
          TimePositions[l].copy(szTemp);
          TimePositions[l].count=values[l].count;
        }
      }
      delete[] values;*/

        if (metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.interval.empty()) {
          // TODO
          CDBError("Dimension interval '%d' not defined", i);
          return 1;
        }
        // strncpy(szInterval,metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.interval.c_str(),32);szInterval[31]='\0';
        const char *pszInterval = metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.interval.c_str();
        // hastimedomain = 1;
        // if(srvParam->requestType==REQUEST_WMS_GETCAPABILITIES)
        {
          CT::string dimUnits("ISO8601");
          if (metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.units.empty() == false) {
            dimUnits.copy(metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.units.c_str());
          }
          dim.serviceName.copy(metadataLayer->dataSource->cfgLayer->Dimension[i]->value.c_str());
          dim.cdfName.copy(metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.name.c_str());
          dim.units.copy(dimUnits.c_str());
          dim.hasMultipleValues = 0;
          // metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.defaultV.c_str()
          const char *pszDefaultV = metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.defaultV.c_str();
          CT::string defaultV;
          if (pszDefaultV != NULL) defaultV = pszDefaultV;
          if (defaultV.length() == 0 || defaultV.equals("max", 3)) {
            dim.defaultValue.copy(szMaxTime);
          } else if (defaultV.equals("min", 3)) {
            dim.defaultValue.copy(szMinTime);
          } else {
            dim.defaultValue.copy(&defaultV);
          }
          if (dim.defaultValue.length() == 19) {
            dim.defaultValue.concat("Z");
          }

          CT::string minTime = szMinTime;
          if (minTime.equals(szMaxTime)) {
            dim.values.print("%s", szMinTime);
          } else {
            dim.values.print("%s/%s/%s", szMinTime, szMaxTime, pszInterval);
          }
        }
      }

      // Check for forced values
      if (!metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.fixvalue.empty()) {
        dim.values = metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.fixvalue;
        dim.defaultValue = metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.fixvalue;
        dim.hasMultipleValues = false;
      }

      // Check if it should be hidden
      if (metadataLayer->dataSource->cfgLayer->Dimension[i]->attr.hidden == true) {
        dim.hidden = true;
      }
      metadataLayer->layerMetadata.dimList.push_back(dim);
    }
  }

  return 0;
}

int getProjectionInformationForLayer(MetadataLayer *metadataLayer) {
#ifdef CXMLGEN_DEBUG
  CDBDebug("getProjectionInformationForLayer");
#endif
  if (metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeCascaded || metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate) {
    if (metadataLayer->dataSource->cfgLayer->LatLonBox.size() == 0) {
      return 0;
    }
  }

  if (loadLayerProjectionAndExtentListFromMetadataDb(metadataLayer) == 0) {

    return 0;
  }

  CDataReader reader;
  int status = reader.open(metadataLayer->dataSource, CNETCDFREADER_MODE_OPEN_DIMENSIONS);
  if (status != 0) {
    CDBError("Could not open file: %s", metadataLayer->dataSource->getFileName());
    return 1;
  }

  CServerParams *srvParam = metadataLayer->dataSource->srvParams;

  for (size_t p = 0; p < srvParam->cfg->Projection.size(); p++) {
    CGeoParams geo;
    geo.CRS.copy(srvParam->cfg->Projection[p]->attr.id.c_str());

#ifdef MEASURETIME
    StopWatch_Stop("start initreproj %s", geo.CRS.c_str());
#endif
    CImageWarper warper;
    int status = warper.initreproj(metadataLayer->dataSource, &geo, &srvParam->cfg->Projection);
    if (status != 0) {
      warper.closereproj();
      return 1;
    }

#ifdef MEASURETIME
    StopWatch_Stop("finished initreproj");
#endif

#ifdef CXMLGEN_DEBUG
    if (status != 0) {
      warper.closereproj();
      CDBDebug("Unable to initialize projection ");
    }
#endif

#ifdef MEASURETIME
    StopWatch_Stop("start findExtent");
#endif

    double bboxToFind[4];
    warper.findExtent(metadataLayer->dataSource, bboxToFind);
    metadataLayer->layerMetadata.projectionList.push_back(LayerMetadataProjection(geo.CRS.c_str(), bboxToFind));

#ifdef MEASURETIME
    StopWatch_Stop("finished findExtent");
#endif

#ifdef CXMLGEN_DEBUG
    CDBDebug("PROJ=%s\tBBOX=(%f,%f,%f,%f)", myProjection.name.c_str(), myProjection.dfBBOX[0], myProjection.dfBBOX[1], myProjection.dfBBOX[2], myProjection.dfBBOX[3]);
#endif

    // TODO!!! THIS IS DONE WAY TO OFTEN!
    // Calculate the latlonBBOX
    if (geo.CRS.equals("EPSG:4326")) {
      for (int k = 0; k < 4; k++) metadataLayer->layerMetadata.dfLatLonBBOX[k] = bboxToFind[k];
    }

    warper.closereproj();
  }

  // Add the layers native projection as well
  if (!metadataLayer->dataSource->nativeEPSG.empty()) {
    metadataLayer->layerMetadata.projectionList.push_back(LayerMetadataProjection(metadataLayer->dataSource->nativeEPSG, metadataLayer->dataSource->dfBBOX));
  }

  return 0;
}

int getStylesForLayer(MetadataLayer *metadataLayer) {
  if (metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeCascaded ||
      (metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate) && metadataLayer->dataSource->cfgLayer->DataPostProc.empty()) {
    // Ignore styling in default case of the demo liveupdate layer, but not if there are data postprocessors
    return 0;
  }

  if (loadLayerStyleListFromMetadataDb(metadataLayer) == 0) {
    return 0;
  }

  // Auto configure styles
  if (metadataLayer->hasError == false) {
    if (metadataLayer->dataSource->cfgLayer->Styles.size() == 0) {
      if (metadataLayer->dataSource->dLayerType != CConfigReaderLayerTypeCascaded && metadataLayer->dataSource->dLayerType != CConfigReaderLayerTypeLiveUpdate) {
#ifdef CXMLGEN_DEBUG
        CDBDebug("cfgLayer->attr.type  %d", metadataLayer->dataSource->dLayerType);
#endif
        int status = CAutoConfigure::autoConfigureStyles(metadataLayer->dataSource);
        if (status != 0) {
          metadataLayer->hasError = 1;
          CDBError("Unable to autoconfigure styles for layer %s", metadataLayer->layerMetadata.name.c_str());
        }
        // Get the defined styles for this layer
      }
    }
  }

  CT::PointerList<CStyleConfiguration *> *styleListFromDataSource = metadataLayer->dataSource->getStyleListForDataSource(metadataLayer->dataSource);

  if (styleListFromDataSource == NULL) return 1;

  for (size_t j = 0; j < styleListFromDataSource->size(); j++) {
    LayerMetadataStyle style = {
        .name = styleListFromDataSource->get(j)->styleCompositionName, .title = styleListFromDataSource->get(j)->styleTitle, .abstract = styleListFromDataSource->get(j)->styleAbstract};
    metadataLayer->layerMetadata.styleList.push_back(style);
  }

  delete styleListFromDataSource;

  return 0;
}

bool compareStringCase(const std::string &s1, const std::string &s2) { return strcmp(s1.c_str(), s2.c_str()) < 0; }

bool compareProjection(const LayerMetadataProjection &p1, const LayerMetadataProjection &p2) { return strcmp(p1.name.c_str(), p2.name.c_str()) < 0; }
bool compareDim(const LayerMetadataDim &p2, const LayerMetadataDim &p1) { return strcmp(p1.serviceName.c_str(), p2.serviceName.c_str()) < 0; }

int getTitleForLayer(MetadataLayer *metadataLayer) {
#ifdef CXMLGEN_DEBUG
  CDBDebug("getTitleForLayer");
#endif
  // Is this a cascaded WMS server?
  if (metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeCascaded) {
    return 0;
  }
  // This a liveupdate layer
  if (metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate) {
    return layerTypeLiveUpdateConfigureWMSLayerForGetCapabilities(metadataLayer);
  }
  // Get a nice name for this layer (if not configured) from the file attributes
  if (metadataLayer->dataSource->cfgLayer->Title.size() == 0) {
    if (metadataLayer->fileName.empty()) {
      CDBError("No file name specified for layer %s", metadataLayer->dataSource->layerName.c_str());
      return 1;
    }
    // TODO, possibly read file metadata from db in future?
    CDataReader reader;
    int status = reader.open(metadataLayer->dataSource, CNETCDFREADER_MODE_OPEN_DIMENSIONS); // TODO, would open header also work?
    if (status != 0 || metadataLayer->dataSource->getNumDataObjects() == 0) {
      CDBError("Could not open file: %s", metadataLayer->dataSource->getFileName());
      return 1;
    }
    CDF::Attribute *longName = metadataLayer->dataSource->getDataObject(0)->cdfVariable->getAttributeNE("long_name");
    if (longName != nullptr) {
      metadataLayer->layerMetadata.title.copy(longName->getDataAsString());
      // Concat variable name prefixed with longname
      metadataLayer->layerMetadata.title.printconcat(" (%s)", metadataLayer->dataSource->getDataObject(0)->cdfVariable->name.c_str());
    } else {
      CDF::Attribute *standardName = metadataLayer->dataSource->getDataObject(0)->cdfVariable->getAttributeNE("standard_name");
      if (standardName != nullptr) {
        metadataLayer->layerMetadata.title.copy(standardName->getDataAsString());
        // Concat variable name prefixed with standardname
        metadataLayer->layerMetadata.title.printconcat(" (%s)", metadataLayer->dataSource->getDataObject(0)->cdfVariable->name.c_str());
      } else {
        // Only variable name
        metadataLayer->layerMetadata.title.copy(metadataLayer->dataSource->getDataObject(0)->cdfVariable->name);
      }
    }
  }
  return 0;
}

int getFileNameForLayer(MetadataLayer *metadataLayer) {
#ifdef CXMLGEN_DEBUG
  CDBDebug("getFileNameForLayer");
#endif
  if (!metadataLayer->fileName.empty()) {
    CDBDebug("seems already done");
    return 0;
  }
  CServerParams *srvParam = metadataLayer->dataSource->srvParams;

  if (metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeDataBase || metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeStyled ||
      metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate) {
    if (metadataLayer->dataSource->cfgLayer->Dimension.size() == 0) {
      metadataLayer->fileName.copy(metadataLayer->dataSource->cfgLayer->FilePath[0]->value.c_str());
      if (CAutoConfigure::autoConfigureDimensions(metadataLayer->dataSource) != 0) {
        CDBError("Unable to autoconfigure dimensions");
        return 1;
      }
    }

    /* A dimension where the default value is set to filetimedate should not be queried from the db */
    bool dataBaseDimension = true;
    if (metadataLayer->layer->Dimension.size() == 1 && metadataLayer->layer->Dimension[0]->attr.defaultV.equals("filetimedate")) {
      dataBaseDimension = false;
    }

    // Check if any dimension is given:
    if (dataBaseDimension == false || (metadataLayer->layer->Dimension.size() == 0) || (metadataLayer->layer->Dimension.size() == 1 && metadataLayer->layer->Dimension[0]->attr.name.equals("none"))) {
#ifdef CXMLGEN_DEBUG
      CDBDebug("Layer %s has no dimensions", metadataLayer->dataSource->layerName.c_str());
#endif
      // If not, just return the filename as configured in the layer
      std::vector<std::string> fileList;
      try {
        fileList = CDBFileScanner::searchFileNames(metadataLayer->dataSource->cfgLayer->FilePath[0]->value.c_str(), metadataLayer->dataSource->cfgLayer->FilePath[0]->attr.filter, NULL);
      } catch (int linenr) {
      };
      metadataLayer->fileName.copy(fileList[0].c_str());
      return 0;
    }

    // Auto scan in case of autowms
    if (srvParam->isAutoLocalFileResourceEnabled() == true) {
      int status = CDBFactory::getDBAdapter(srvParam->cfg)->autoUpdateAndScanDimensionTables(metadataLayer->dataSource);
      if (status != 0) {
        CDBError("Unable to checkDimTables");
        return 1;
      }
    }

    // Find the first occuring filename.
    CT::string tableName;
    CT::string dimName(metadataLayer->layer->Dimension[0]->attr.name.c_str());
    try {
      tableName = CDBFactory::getDBAdapter(srvParam->cfg)
                      ->getTableNameForPathFilterAndDimension(metadataLayer->layer->FilePath[0]->value.c_str(), metadataLayer->layer->FilePath[0]->attr.filter.c_str(), dimName.c_str(),
                                                              metadataLayer->dataSource);
    } catch (int e) {
      CDBError("Unable to create tableName from '%s' '%s' '%s'", metadataLayer->layer->FilePath[0]->value.c_str(), metadataLayer->layer->FilePath[0]->attr.filter.c_str(), dimName.c_str());
      return 1;
    }

    // Query one filename
    CDBStore::Store *values = CDBFactory::getDBAdapter(srvParam->cfg)->getUniqueValuesOrderedByValue("path", 1, false, tableName.c_str());
    bool databaseError = false;

    if (values == NULL) {
      CDBError("No files found for %s ", metadataLayer->dataSource->layerName.c_str());
      databaseError = true;
    }
    if (databaseError == false) {
      if (values->getSize() > 0) {
#ifdef CXMLGEN_DEBUG
        CDBDebug("Query  succeeded: Filename = %s", values->getRecord(0)->get(0)->c_str());
#endif
        metadataLayer->fileName.copy(values->getRecord(0)->get(0));
      } else {
        // The file is not in the database, probably an error during the database scan has been detected earlier.
        // Ignore the file for now too
        CDBError("Query for '%s' not succeeded", metadataLayer->layer->FilePath[0]->value.c_str());
        databaseError = true;
      }
      delete values;
    }

#ifdef CXMLGEN_DEBUG
    CDBDebug("/Database");
#endif
    if (databaseError) {
      return 1;
    }
  }

  return 0;
}
