#include <CXMLGen.h>
#include "LayerMetadataStore.h"
#include "XMLGenUtils.h"
#include "CStopWatch.h"
#include <CDBStore.h>
#include <CDBFactory.h>
#include <LayerTypeLiveUpdate/LayerTypeLiveUpdate.h>
#include "LayerUtils.h"
#include <sstream>
#include "CRequestUtils.h"
#include <timeutils.h>

int populateMetadataLayerStruct(MetadataLayer *metadataLayer, bool readFromDB) {
  metadataLayer->readFromDb = readFromDB;
  if (!metadataLayer->srvParams->useMetadataTable()) {
    metadataLayer->readFromDb = false;
  }

  bool isEdrEnabled = metadataLayer->srvParams->isEdrEnabled();

  // Make the layer name
  metadataLayer->layerMetadata.name = makeUniqueLayerName(metadataLayer->layer);

  // Create new datasource
  if (metadataLayer->dataSource == NULL) {
    metadataLayer->dataSource = new CDataSource();
    if (metadataLayer->dataSource->setCFGLayer(metadataLayer->srvParams, metadataLayer->layer, -1) != 0) {
      return 1;
    }
  }

  // Make the group
  CT::string layerGroup = "";
  if (metadataLayer->layer->Group.size() > 0) {
    if (metadataLayer->layer->Group[0]->attr.value.empty() == false) {
      layerGroup = (metadataLayer->layer->Group[0]->attr.value);
    }
  }
  metadataLayer->layerMetadata.wmsgroup = (&layerGroup);

  // Check if this layer is querable
  int datasetRestriction = checkDataRestriction();
  if ((datasetRestriction & ALLOW_GFI)) {
    metadataLayer->layerMetadata.isQueryable = 1;
  }

  // Get collection for layer from Group def
  CT::string collection = "";
  if (metadataLayer->layer->Group.size() > 0) {
    if (metadataLayer->layer->Group[0]->attr.collection.empty() == false) {
      collection = (metadataLayer->layer->Group[0]->attr.collection);
    }
  }
  metadataLayer->layerMetadata.collection = (&collection);

  // Get Abstract
  if (metadataLayer->dataSource->cfgLayer->Abstract.size() > 0) {
    metadataLayer->layerMetadata.abstract = metadataLayer->dataSource->cfgLayer->Abstract[0]->elementValue;
  }

  // Check if it is hidden
  if (metadataLayer->dataSource->cfgLayer->attr.hidden == ("true")) {
    metadataLayer->layerMetadata.hidden = true;
  }

  // Determine if edr is enabled for this layer
  CT::string layer_enable_edr = metadataLayer->layer->attr.enable_edr;
  if (layer_enable_edr == ("false")) {
    metadataLayer->layerMetadata.enable_edr = false;
  } else if (layer_enable_edr == ("true")) {
    metadataLayer->layerMetadata.enable_edr = true;
  } else {
    metadataLayer->layerMetadata.enable_edr = isEdrEnabled;
  }

  // Fill in Layer title, with fallback to Name (later this can be set based on metadata or info from the file)
  if (metadataLayer->dataSource->cfgLayer->Title.size() != 0) {
    metadataLayer->layerMetadata.title = (metadataLayer->dataSource->cfgLayer->Title[0]->elementValue);
  } else {
    metadataLayer->layerMetadata.title = (metadataLayer->dataSource->cfgLayer->Name[0]->elementValue);
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
      metadataLayer->dataSource->addStep(metadataLayer->fileName);
    }

    if (getTitleForLayer(metadataLayer) != 0) {
      metadataLayer->hasError = 1;
      return 1;
    }

    CDataReader reader;
    status = reader.open(metadataLayer->dataSource, CNETCDFREADER_MODE_OPEN_HEADER);
    if (status != 0 && metadataLayer->dataSource->dLayerType != CConfigReaderLayerTypeLiveUpdate) {
      CDBError("Could not open file: %s", metadataLayer->dataSource->getFileName().c_str());
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

    for (auto &d: metadataLayer->dataSource->dataObjects) {
      if (d.filterFromOutput) {
        continue;
      }
      if (d.cdfVariable == nullptr) {
        continue;
      }
      CDF::Attribute *longName = d.cdfVariable->getAttributeNE("long_name");
      if (longName == nullptr) {
        longName = d.cdfVariable->getAttributeNE("standard_name");
      }
      CDF::Attribute *standardNameAttr = d.cdfVariable->getAttributeNE("standard_name");

      CT::string label = longName != nullptr ? longName->toString() : dObjgetVariableName(d).c_str();
      LayerMetadataVariable layerMetadataVariable = {.variableName = d.cdfVariable->name, .units = dObjgetUnits(d), .label = label, .standard_name = d.cdfVariable->name};

      if (standardNameAttr != nullptr) {
        layerMetadataVariable.standard_name = standardNameAttr->toString();
        // CDBDebug("standard_name for %s: %s", d.cdfVariable->name.c_str(), standardNameAttr->toString().c_str());
      }
      metadataLayer->layerMetadata.variableList.push_back(layerMetadataVariable);
    }
  }

  if (loadLayerDimensionListFromMetadataDb(metadataLayer) != 0) {

    if (getDimsForLayer(metadataLayer->dataSource, metadataLayer->layerMetadata.dimList, {}) != 0) {
      metadataLayer->hasError = 1;
      return 1;
    }
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
  for (const auto &p: metadataLayer->layerMetadata.projectionList) {
    projectionMap.emplace(p.name, p);
  }
  metadataLayer->layerMetadata.projectionList.clear();
  for (const auto &p: projectionMap) {
    metadataLayer->layerMetadata.projectionList.push_back(p.second);
  }

  std::sort(metadataLayer->layerMetadata.projectionList.begin(), metadataLayer->layerMetadata.projectionList.end(), compareProjection);
  std::sort(metadataLayer->layerMetadata.dimList.begin(), metadataLayer->layerMetadata.dimList.end(), compareDim);

  return 0;
}

int checkDependenciesBetweenDims(const CDataSource *dataSource, std::vector<LayerMetadataDim> &layerMetadataDimList) {
  if (dataSource == nullptr) {
    CDBError("No datasource defined");
    return XMLGENUTILS_CHECKDEP_NODATASOURCE;
  }
  if (dataSource->cfgLayer == nullptr) {
    CDBError("No configuration defined for datasource");
    return XMLGENUTILS_CHECKDEP_DATASOURCE_NOT_CONFIGURED;
  }

  auto cfgDims = dataSource->cfgLayer->Dimension; // Layer configuration dimensions

  // Find time dimension in Layer configuration dimensions
  auto xmleDimTimeIt = std::find_if(cfgDims.begin(), cfgDims.end(), [](const CServerConfig::XMLE_Dimension *d) -> bool { return d->elementValue == "time"; });
  if (xmleDimTimeIt == cfgDims.end()) {
    // There is no time dim. Do nothing, all OK.
    return XMLGENUTILS_CHECKDEP_DATASOURCE_NO_TIME;
  }
  CServerConfig::XMLE_Dimension *layerCfgTimeDim = xmleDimTimeIt[0];
  int hasIsoDuration = CT::indexOf(layerCfgTimeDim->attr.defaultV, "+");

  if (hasIsoDuration == -1) {
    // There is no iso time range defined. Do nothing, all OK.
    return XMLGENUTILS_CHECKDEP_DATASOURCE_NO_ISO_DURATION;
  }

  // Find time dimensions in the layerMetadata dimList
  auto &lmDims = layerMetadataDimList; // Layer metadata dimensions, referenced by & so we can adjust the value in the object of the vector
  auto lmDimTimeIt = std::find_if(lmDims.begin(), lmDims.end(), [](const LayerMetadataDim &d) -> bool { return d.serviceName == "time"; });
  if (lmDimTimeIt == lmDims.end()) {
    // There is no metadata dimension named time
    return XMLGENUTILS_CHECKDEP_DATASOURCE_NO_DIMS_IN_LAYERMETADATA;
  }
  // Find reference_time dimensions in the layerMetadata dimList
  auto lmDimRefTimeIt = std::find_if(lmDims.begin(), lmDims.end(), [](const LayerMetadataDim &d) -> bool { return d.serviceName == "reference_time" || d.serviceName == "forecast_reference_time"; });
  if (lmDimRefTimeIt == lmDims.end()) {
    lmDimTimeIt->defaultValue = "";
    return XMLGENUTILS_CHECKDEP_DATASOURCE_NO_DIMS_IN_LAYERMETADATA;
  }

  LayerMetadataDim layerMetadataTimeDim = lmDimTimeIt[0];
  LayerMetadataDim layerMetadataRefTimeDim = lmDimRefTimeIt[0];

  CT::string isoDurationString = CT::substring(layerCfgTimeDim->attr.defaultV, hasIsoDuration + 1, -1);
  // CDBDebug("Going to use isoduration [%s] to add to [%s]", isoDurationString.c_str(), layerMetadataRefTimeDim.defaultValue.c_str());
  try {
    CTime *time = CTime::GetCTimeEpochInstance();
    CTime::Date refTimeDate = time->freeDateStringToDate(layerMetadataRefTimeDim.defaultValue.c_str());
    CTime::Date refTimeWithAddedPeriod = time->addPeriodToDate(refTimeDate, isoDurationString);
    // Assign the default value of the reference time dimension in the layer metadata to the default value of the the time dimension
    lmDimTimeIt->defaultValue = time->dateToISOString(refTimeWithAddedPeriod);
    // CDBDebug("New defaultvalue for time is [%s]", lmDimTimeIt->defaultValue.c_str());
    return 0; // OK!
  } catch (int e) {
    CDBWarning("Unable to parse given duration in default value of time dimension [%d]", e);
    lmDimTimeIt->defaultValue = layerMetadataRefTimeDim.defaultValue;
    return XMLGENUTILS_CHECKDEP_DATASOURCE_UNABLE_TO_PARSETIME;
  }
}

bool isLayerDimATimeDim(CServerConfig::XMLE_Dimension *cfgLayerDim) {
  return (cfgLayerDim->attr.name == ("time") || (CT::indexOf(cfgLayerDim->attr.name, "time") >= 0 && cfgLayerDim->attr.units == ("ISO8601")));
}

LayerMetadataDim handleMultipleValueDim(CDataSource *dataSource, CServerConfig::XMLE_Dimension *cfgLayerDim, const std::map<std::string, std::vector<std::string>> &dimValuesMap) {
  // Get all dimension values from the db
  auto srvParam = dataSource->srvParams;
  bool isTimeDim = isLayerDimATimeDim(cfgLayerDim);

  auto dimValueInMapIt = dimValuesMap.find(cfgLayerDim->elementValue);
  std::vector<std::string> queryValues;
  if (dimValueInMapIt == dimValuesMap.end() || dimValueInMapIt->second.size() == 0) {

    // Get the tablename
    std::string tableName = CDBFactory::getDBAdapter(srvParam->cfg)
                                ->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->elementValue, dataSource->cfgLayer->FilePath[0]->attr.filter,
                                                                        cfgLayerDim->attr.name.c_str(), dataSource);

    auto values = isTimeDim ? CDBFactory::getDBAdapter(srvParam->cfg)->getUniqueValuesOrderedByValue(cfgLayerDim->attr.name.c_str(), 0, true, tableName.c_str())
                            : CDBFactory::getDBAdapter(srvParam->cfg)->getUniqueValuesOrderedByIndex(cfgLayerDim->attr.name.c_str(), 0, true, tableName.c_str());

    if (values == NULL) {
      CDBError("Query failed");
      throw __LINE__;
    }
    if (isTimeDim) {
      for (auto &record: values->records) {
        queryValues.push_back(makeIsoStringFromDbString(record.get(0)));
      }
    } else {
      for (auto &record: values->records) {
        queryValues.push_back(record.get(0));
      }
    }
    delete values;
  } else {
    queryValues = dimValueInMapIt->second;
  }

  LayerMetadataDim layerMetadataDim;
  layerMetadataDim.hidden = cfgLayerDim->attr.hidden;
  layerMetadataDim.type = cfgLayerDim->attr.type;
  layerMetadataDim.serviceName = cfgLayerDim->elementValue;
  layerMetadataDim.cdfName = cfgLayerDim->attr.name;
  if (queryValues.size() > 0) {
    layerMetadataDim.serviceName = cfgLayerDim->elementValue;
    layerMetadataDim.cdfName = (cfgLayerDim->attr.name);

    // // Try to get units from the variable
    layerMetadataDim.units = "NA";
    if (cfgLayerDim->attr.units.empty()) {
      try {
        if (dataSource->getDataObject(0)->cdfObject != nullptr) {
          layerMetadataDim.units = dataSource->getDataObject(0)->cdfObject->getVariableThrows(cfgLayerDim->attr.name)->getAttributeThrows("units")->toString();
        }
      } catch (int e) {
      }
    }

    layerMetadataDim.hasMultipleValues = 1;
    if (isTimeDim == true) {
      layerMetadataDim.units = "ISO8601";
    }

    if (!cfgLayerDim->attr.units.empty()) {
      layerMetadataDim.units = (cfgLayerDim->attr.units);
    }

    if (cfgLayerDim->attr.defaultV == ("max")) {
      layerMetadataDim.defaultValue = queryValues[queryValues.size() - 1];
    } else if (cfgLayerDim->attr.defaultV == ("min")) {
      layerMetadataDim.defaultValue = queryValues[0];
    } else {
      if (cfgLayerDim->attr.defaultV.empty()) {
        layerMetadataDim.defaultValue = queryValues[queryValues.size() - 1];
      } else {
        layerMetadataDim.defaultValue = cfgLayerDim->attr.defaultV;
      }
    }

    layerMetadataDim.values = CT::join(queryValues);
  }

  // Check for forced values
  if (!cfgLayerDim->attr.fixvalue.empty()) {
    layerMetadataDim.values = cfgLayerDim->attr.fixvalue;
    layerMetadataDim.defaultValue = cfgLayerDim->attr.fixvalue;
    layerMetadataDim.hasMultipleValues = false;
  }

  return layerMetadataDim;
}

LayerMetadataDim handleRangeBasedDim(CDataSource *dataSource, CServerConfig::XMLE_Dimension *cfgLayerDim, const std::map<std::string, std::vector<std::string>> &dimValuesMap) {
  auto srvParam = dataSource->srvParams;

  // CDBDebug("%s %s", cfgLayerDim->attr.name.c_str(), CT::join(valuesFromDimMap).c_str());

  LayerMetadataDim dim;
  dim.hidden = false;
  dim.type = "dimtype_none";
  dim.serviceName = (cfgLayerDim->elementValue);
  dim.cdfName = (cfgLayerDim->attr.name);
  if (cfgLayerDim->attr.interval.empty()) {
    CDBError("Dimension interval not defined");
    throw __LINE__;
  }

  std::string minTimeStamp, maxTimeStamp;
  auto dimValueInMapIt = dimValuesMap.find(cfgLayerDim->elementValue);
  if (dimValueInMapIt == dimValuesMap.end() || dimValueInMapIt->second.size() == 0) { //
    // Query the values from the DB if they are not set in the map
    // This is an interval defined as start/stop/resolution
    // Retrieve the minimum dimension value
    CT::string tableName = CDBFactory::getDBAdapter(srvParam->cfg)
                               ->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->elementValue, dataSource->cfgLayer->FilePath[0]->attr.filter,
                                                                       cfgLayerDim->attr.name.c_str(), dataSource);

    auto values = CDBFactory::getDBAdapter(srvParam->cfg)->getMin(cfgLayerDim->attr.name.c_str(), tableName.c_str());

    if (values != nullptr && values->records.size() > 0) {
      minTimeStamp = (makeIsoStringFromDbString(values->records[0].get(0)));
    }
    delete values;
    // Retrieve the max dimension value
    values = CDBFactory::getDBAdapter(srvParam->cfg)->getMax(cfgLayerDim->attr.name.c_str(), tableName.c_str());
    if (values != nullptr && values->records.size() > 0) {
      maxTimeStamp = (makeIsoStringFromDbString(values->records[0].get(0)));
    }
    delete values;
  } else {
    minTimeStamp = dimValueInMapIt->second[0];
    maxTimeStamp = dimValueInMapIt->second[dimValueInMapIt->second.size() - 1];
  }

  CT::string dimUnits("ISO8601");

  if (cfgLayerDim->attr.units.empty() == false) {
    dimUnits = (cfgLayerDim->attr.units);
  }
  dim.serviceName = (cfgLayerDim->elementValue);
  dim.cdfName = (cfgLayerDim->attr.name);
  dim.units = (dimUnits);
  dim.hasMultipleValues = false;
  // cfgLayerDim->attr.defaultV.c_str()
  const char *pszDefaultV = cfgLayerDim->attr.defaultV.c_str();
  CT::string defaultV;
  if (pszDefaultV != NULL) defaultV = pszDefaultV;
  if (defaultV.length() == 0 || defaultV == ("max")) {
    dim.defaultValue = maxTimeStamp;
  } else if (defaultV == ("min")) {
    dim.defaultValue = minTimeStamp;
  } else {
    dim.defaultValue = defaultV;
  }
  if (dim.defaultValue.length() == 19) {
    dim.defaultValue += "Z";
  }

  if (minTimeStamp == maxTimeStamp) {
    dim.values = minTimeStamp;
  } else {
    dim.values = CT::printf("%s/%s/%s", minTimeStamp.c_str(), maxTimeStamp.c_str(), cfgLayerDim->attr.interval.c_str());
  }
  // Check for forced values
  if (!cfgLayerDim->attr.fixvalue.empty()) {
    dim.values = cfgLayerDim->attr.fixvalue;
    dim.defaultValue = cfgLayerDim->attr.fixvalue;
  }

  // Check if it should be hidden
  dim.hidden = cfgLayerDim->attr.hidden;
  dim.type = cfgLayerDim->attr.type;
  return dim;
}

std::string makeIntervalFromTimeList(const std::vector<std::string> &timeStampList, size_t limit) {

  if (timeStampList.size() < limit) {
    return "";
  }

  return estimateISO8601Duration(timeStampList, 1);
}

LayerMetadataDim handleFileTimeDateDim(CDataSource *dataSource) {
  std::string fileDate = getFileDate(dataSource->cfgLayer->FilePath[0]->elementValue);
  LayerMetadataDim dim;
  dim.serviceName = "time";
  dim.cdfName = "time";
  dim.units = "ISO8601";
  dim.values = fileDate;
  dim.defaultValue = fileDate;
  dim.hasMultipleValues = true;
  dim.hidden = false;
  dim.type = "dimtype_none";
  return dim;
}

std::vector<std::string> queryTimeStampListFromDb(CDataSource *dataSource, CServerConfig::XMLE_Dimension *cfgDim) {
  std::vector<std::string> timeStampList;
  auto srvParam = dataSource->srvParams;
  if (!(cfgDim->attr.name == "time" || (CT::indexOf(cfgDim->attr.name, "time") >= 0 && cfgDim->attr.units == "ISO8601"))) {
    return timeStampList;
  }
  // Get the tablename
  CT::string tableName = CDBFactory::getDBAdapter(srvParam->cfg)
                             ->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->elementValue, dataSource->cfgLayer->FilePath[0]->attr.filter, cfgDim->attr.name.c_str(),
                                                                     dataSource);

  // Get the first n values from the database, and determine whether the time resolution is continous or multivalue.
  CDBStore::Store *store = CDBFactory::getDBAdapter(srvParam->cfg)->getUniqueValuesOrderedByValue(cfgDim->attr.name.c_str(), 200, true, tableName.c_str());
  if (store == nullptr || store->records.size() == 0) {
    delete store;
    CDBDebug("No data available in database for dimension %s", cfgDim->attr.name.c_str());
    return timeStampList;
  }
  try {
    for (auto &record: store->records) {
      timeStampList.push_back(makeIsoStringFromDbString(record.get("time")));
    }
  } catch (int e) {
  }
  delete store;
  return timeStampList;
}

int getDimsForLayer(CDataSource *dataSource, std::vector<LayerMetadataDim> &layerMetadataDimensionList, std::map<std::string, std::vector<std::string>> dimValuesMap) {
  if (dataSource->dLayerType != CConfigReaderLayerTypeDataBase) {
    return 0;
  }

  for (auto it = dataSource->cfgLayer->Dimension.begin(); it != dataSource->cfgLayer->Dimension.end(); ++it) {
    auto *cfgDim = (*it);
    // This dimension is a filetimedate type, its values come from the modification date of the file
    if (cfgDim->attr.defaultV == ("filetimedate")) {
      layerMetadataDimensionList.push_back(handleFileTimeDateDim(dataSource));
      break;
    }
    if (it == dataSource->cfgLayer->Dimension.begin() && cfgDim->attr.name == ("none")) break;
    if (cfgDim->attr.interval.empty()) {
      const auto &valuesFromDimMap = dimValuesMap[cfgDim->elementValue];
      const auto &dimValues = valuesFromDimMap.size() == 0 ? queryTimeStampListFromDb(dataSource, cfgDim) : valuesFromDimMap;
      const auto &interval = makeIntervalFromTimeList(dimValues);
      if (!interval.empty()) {
        // Add dimension with auto calculated interval
        cfgDim->attr.interval = interval;
        layerMetadataDimensionList.push_back(handleRangeBasedDim(dataSource, cfgDim, dimValuesMap));
      } else {
        // Add dimension with individual values
        layerMetadataDimensionList.push_back(handleMultipleValueDim(dataSource, cfgDim, dimValuesMap));
      }
    } else {
      // Add dimension with pre-defined interval
      layerMetadataDimensionList.push_back(handleRangeBasedDim(dataSource, cfgDim, dimValuesMap));
    }
  }
  // Check dependencies between dimensions
  checkDependenciesBetweenDims(dataSource, layerMetadataDimensionList);
  return 0;
}

int getProjectionInformationForLayer(MetadataLayer *metadataLayer) {
#ifdef CXMLGEN_DEBUG
  CDBDebug("getProjectionInformationForLayer");
#endif
  if (metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeGraticule || metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate) {
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
    CDBError("Could not open file: %s", metadataLayer->dataSource->getFileName().c_str());
    return 1;
  }

  CServerParams *srvParam = metadataLayer->dataSource->srvParams;

  for (size_t p = 0; p < srvParam->cfg->Projection.size(); p++) {
    GeoParameters geo;
    geo.crs = (srvParam->cfg->Projection[p]->attr.id);

#ifdef MEASURETIME
    StopWatch_Stop("start initreproj %s", geo.crs.c_str());
#endif
    CImageWarper warper;
    int status = warper.initreproj(metadataLayer->dataSource, geo, &srvParam->cfg->Projection);
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
    metadataLayer->layerMetadata.projectionList.push_back(LayerMetadataProjection(geo.crs, bboxToFind));

#ifdef MEASURETIME
    StopWatch_Stop("finished findExtent");
#endif

    // TODO!!! THIS IS DONE WAY TO OFTEN!
    // Calculate the latlonBBOX
    if (geo.crs == ("EPSG:4326")) {
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
  if (metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeGraticule ||
      (metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate && metadataLayer->dataSource->cfgLayer->DataPostProc.empty())) {
    // Ignore styling in default case of the demo liveupdate layer, but not if there are data postprocessors
    return 0;
  }

  if (loadLayerStyleListFromMetadataDb(metadataLayer) == 0) {
    return 0;
  }

  // Auto configure styles
  if (metadataLayer->hasError == false) {
    if (metadataLayer->dataSource->cfgLayer->Styles.size() == 0) {
      if (metadataLayer->dataSource->dLayerType != CConfigReaderLayerTypeGraticule && metadataLayer->dataSource->dLayerType != CConfigReaderLayerTypeLiveUpdate) {
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

  std::vector<CStyleConfiguration> styleListFromDataSource = metadataLayer->dataSource->getStyleListForDataSource();

  for (const auto &styleConfig: styleListFromDataSource) {
    LayerMetadataStyle style = {.name = styleConfig.styleCompositionName, .title = styleConfig.styleTitle, .abstract = styleConfig.styleAbstract};
    metadataLayer->layerMetadata.styleList.push_back(style);
  }

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
  if (metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeGraticule) {
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
      CDBError("Could not open file: %s", metadataLayer->dataSource->getFileName().c_str());
      return 1;
    }
    CDF::Attribute *longName = metadataLayer->dataSource->getDataObject(0)->cdfVariable->getAttributeNE("long_name");
    if (longName != nullptr) {
      metadataLayer->layerMetadata.title = (longName->toString());
      // Concat variable name prefixed with longname
      metadataLayer->layerMetadata.title.printconcat(" (%s)", metadataLayer->dataSource->getDataObject(0)->cdfVariable->name.c_str());
    } else {
      CDF::Attribute *standardName = metadataLayer->dataSource->getDataObject(0)->cdfVariable->getAttributeNE("standard_name");
      if (standardName != nullptr) {
        metadataLayer->layerMetadata.title = (standardName->toString());
        // Concat variable name prefixed with standardname
        metadataLayer->layerMetadata.title.printconcat(" (%s)", metadataLayer->dataSource->getDataObject(0)->cdfVariable->name.c_str());
      } else {
        // Only variable name
        metadataLayer->layerMetadata.title = (metadataLayer->dataSource->getDataObject(0)->cdfVariable->name);
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
    return 0;
  }
  CServerParams *srvParam = metadataLayer->dataSource->srvParams;

  if (metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeDataBase) {
    if (metadataLayer->dataSource->cfgLayer->Dimension.size() == 0) {
      metadataLayer->fileName = (metadataLayer->dataSource->cfgLayer->FilePath[0]->elementValue);
      if (CAutoConfigure::autoConfigureDimensions(metadataLayer->dataSource) != 0) {
        CDBError("Unable to autoconfigure dimensions");
        return 1;
      }
    }

    /* A dimension where the default value is set to filetimedate should not be queried from the db */
    bool dataBaseDimension = true;
    if (metadataLayer->layer->Dimension.size() == 1 && metadataLayer->layer->Dimension[0]->attr.defaultV == ("filetimedate")) {
      dataBaseDimension = false;
    }

    // Check if any dimension is given:
    if (dataBaseDimension == false || (metadataLayer->layer->Dimension.size() == 0) || (metadataLayer->layer->Dimension.size() == 1 && metadataLayer->layer->Dimension[0]->attr.name == ("none"))) {
#ifdef CXMLGEN_DEBUG
      CDBDebug("Layer %s has no dimensions", metadataLayer->dataSource->layerName.c_str());
#endif
      // If not, just return the filename as configured in the layer
      std::vector<std::string> fileList;
      try {
        fileList = CDBFileScanner::searchFileNames(metadataLayer->dataSource->cfgLayer->FilePath[0]->elementValue.c_str(), metadataLayer->dataSource->cfgLayer->FilePath[0]->attr.filter, NULL);
      } catch (int linenr) {
      };
      metadataLayer->fileName = (fileList[0]);
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
    CT::string dimName(metadataLayer->layer->Dimension[0]->attr.name);
    try {
      tableName = CDBFactory::getDBAdapter(srvParam->cfg)
                      ->getTableNameForPathFilterAndDimension(metadataLayer->layer->FilePath[0]->elementValue, metadataLayer->layer->FilePath[0]->attr.filter, dimName.c_str(),
                                                              metadataLayer->dataSource);
    } catch (int e) {
      CDBError("Unable to create tableName from '%s' '%s' '%s'", metadataLayer->layer->FilePath[0]->elementValue.c_str(), metadataLayer->layer->FilePath[0]->attr.filter.c_str(), dimName.c_str());
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
      if (values->records.size() > 0) {
#ifdef CXMLGEN_DEBUG
        CDBDebug("Query  succeeded: Filename = %s", values->records[0].get(0).c_str());
#endif
        metadataLayer->fileName = values->records[0].get(0);
      } else {
        // The file is not in the database, probably an error during the database scan has been detected earlier.
        // Ignore the file for now too
        CDBError("Query for '%s' not succeeded", metadataLayer->layer->FilePath[0]->elementValue.c_str());
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

// Function to parse a string to double if numeric
double parseNumeric(std::string const &str, bool &isNumeric) {
  auto result = double();
  auto i = std::istringstream(str);
  i >> result;
  isNumeric = !i.fail() && i.eof();
  return result;
}

// Sort values that can either be numeric of a string
bool multiTypeSort(const CT::string &a, const CT::string &b) {
  // Try to convert strings to numbers
  float aNum, bNum;
  bool isANum, isBNum;

  isANum = false;
  aNum = parseNumeric(a, isANum);

  isBNum = false;
  bNum = parseNumeric(b, isBNum);

  // Do numerical comparison or alphabetical comparison according to type
  if (isANum && isBNum) {
    return aNum < bNum;
  } else {
    return a < b;
  }
}
