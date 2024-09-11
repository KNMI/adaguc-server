#include "./LayerMetadataStore.h"

#include <json_adaguc.h>
#include <CDBFactory.h>
#include "XMLGenUtils.h"

int getDimensionListAsJson(MetadataLayer *myMetadataLayer, json &dimListJson) {
  try {

    for (auto dimension : myMetadataLayer->layerMetadata.dimList) {
      json item;
      item["defaultValue"] = dimension->defaultValue.c_str();
      item["hasMultipleValues"] = dimension->hasMultipleValues;
      item["hidden"] = dimension->hidden;
      item["name"] = dimension->name.c_str();
      item["units"] = dimension->units.c_str();
      item["values"] = dimension->values.c_str();
      dimListJson[dimension->name.c_str()] = item;
    }
  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  }
  return 0;
}

int getLayerBaseMetadataAsJson(MetadataLayer *myMetadataLayer, json &layerMetadataItem) {
  try {
    layerMetadataItem["name"] = myMetadataLayer->layerMetadata.name;
    layerMetadataItem["title"] = myMetadataLayer->layerMetadata.title;
    layerMetadataItem["group"] = myMetadataLayer->layerMetadata.group;
    layerMetadataItem["abstract"] = myMetadataLayer->layerMetadata.abstract;
    layerMetadataItem["nativeepsg"] = myMetadataLayer->layerMetadata.nativeEPSG;

    layerMetadataItem["isqueryable"] = myMetadataLayer->layerMetadata.isQueryable;
    json latlonbox;
    for (size_t j = 0; j < 4; j++) {
      latlonbox.push_back(myMetadataLayer->layerMetadata.dfLatLonBBOX[j]);
    }
    layerMetadataItem["latlonbox"] = latlonbox;

    json gridspec;
    json bbox;
    for (size_t j = 0; j < 4; j++) {
      bbox.push_back(myMetadataLayer->layerMetadata.dfBBOX[j]);
    }
    gridspec["bbox"] = bbox;
    gridspec["width"] = myMetadataLayer->layerMetadata.width;
    gridspec["height"] = myMetadataLayer->layerMetadata.height;
    gridspec["cellsizex"] = myMetadataLayer->layerMetadata.cellsizeX;
    gridspec["cellsizey"] = myMetadataLayer->layerMetadata.cellsizeY;
    gridspec["projstring"] = myMetadataLayer->layerMetadata.projstring;

    layerMetadataItem["gridspec"] = gridspec;

    json variables;
    for (auto lv : myMetadataLayer->layerMetadata.variableList) {
      json variable;
      variable["units"] = lv->units;
      variable["label"] = lv->label;
      variable["variableName"] = lv->variableName;
      variables.push_back(variable);
    }
    layerMetadataItem["variables"] = variables;

  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  }
  return 0;
}
int getProjectionListAsJson(MetadataLayer *myMetadataLayer, json &projsettings) {
  try {
    for (auto projection : myMetadataLayer->layerMetadata.projectionList) {
      json item = {projection->dfBBOX[0], projection->dfBBOX[1], projection->dfBBOX[2], projection->dfBBOX[3]};
      projsettings[projection->name.c_str()] = item;
    }
  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  }
  return 0;
}

int getStyleListMetadataAsJson(MetadataLayer *myMetadataLayer, json &styleListJson) {
  try {

    for (auto style : myMetadataLayer->layerMetadata.styleList) {
      json item;
      item["abstract"] = style->abstract.c_str();
      item["title"] = style->title.c_str();
      item["name"] = style->name.c_str();
      styleListJson.push_back(item);
    }
  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  }
  return 0;
}

int storeMyMetadataLayerIntoMetadataDb(MetadataLayer *myMetadataLayer) {
  storeLayerMetadataStructIntoMetadataDb(myMetadataLayer);
  storeLayerDimensionListIntoMetadataDb(myMetadataLayer);
  storeLayerProjectionAndExtentListIntoMetadataDb(myMetadataLayer);
  storeLayerStyleListIntoMetadataDb(myMetadataLayer);
  return 0;
}

int loadMyMetadataLayerFromMetadataDb(MetadataLayer *myMetadataLayer) {
  loadLayerMetadataStructFromMetadataDb(myMetadataLayer);
  loadLayerDimensionListFromMetadataDb(myMetadataLayer);
  loadLayerProjectionAndExtentListFromMetadataDb(myMetadataLayer);
  loadLayerStyleListFromMetadataDb(myMetadataLayer);
  return 0;
}

CT::string getLayerMetadataFromDb(MetadataLayer *myMetadataLayer, CT::string metadataKey) {
  CT::string layerName = myMetadataLayer->dataSource->getLayerName();
  CT::string datasetName = myMetadataLayer->dataSource->srvParams->datasetLocation;
  if (datasetName.empty()) {
    // CDBDebug("Not a dataset");
    return "";
  }
  CDBStore::Store *layerMetaDataStore = CDBFactory::getDBAdapter(myMetadataLayer->dataSource->srvParams->cfg)->getLayerMetadataStore(datasetName);
  if (layerMetaDataStore == nullptr) {
    return "";
  }
  auto records = layerMetaDataStore->getRecords();
  for (auto record : records) {
    if (record->get("layername")->equals(layerName) && record->get("metadatakey")->equals(metadataKey)) {
#ifdef MEASURETIME
      StopWatch_Stop("<CDBAdapterPostgreSQL::getLayerMetadata");
#endif
      return record->get("blob");
    }
  }

#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getLayerMetadata");
#endif
  CDBDebug("No metadata entry found for %s %s %s", datasetName, layerName, metadataKey);
  throw __LINE__;
}

int storeLayerMetadataInDb(MetadataLayer *myMetadataLayer, CT::string metadataKey, std::string metadataBlob) {
  try {
    CT::string datasetName = myMetadataLayer->dataSource->srvParams->datasetLocation;
    if (datasetName.empty()) {
      CDBDebug("Not a dataset");
      return 1;
    }
    CT::string layerName = myMetadataLayer->dataSource->getLayerName();

    return CDBFactory::getDBAdapter(myMetadataLayer->dataSource->srvParams->cfg)->storeLayerMetadata(datasetName, layerName, metadataKey, metadataBlob.c_str());
  } catch (int e) {
    return e;
  }
  return 0;
}

int storeLayerMetadataStructIntoMetadataDb(MetadataLayer *myMetadataLayer) {
  json layerMetadataItem;

  if (getLayerBaseMetadataAsJson(myMetadataLayer, layerMetadataItem) != 0) {
    return 1;
  }

  storeLayerMetadataInDb(myMetadataLayer, "layermetadata", layerMetadataItem.dump());
  return 0;
}

int loadLayerMetadataStructFromMetadataDb(MetadataLayer *myMetadataLayer) {
  if (!myMetadataLayer->readFromDb) {
    return 1;
  }
  try {
    CT::string layerMetadataAsJson = getLayerMetadataFromDb(myMetadataLayer, "layermetadata");
    if (layerMetadataAsJson.empty()) {
      return 1;
    }
    json a;
    auto i = a.parse(layerMetadataAsJson.c_str());
    myMetadataLayer->layerMetadata.name = i["name"].get<std::string>().c_str();
    myMetadataLayer->layerMetadata.title = i["title"].get<std::string>().c_str();
    myMetadataLayer->layerMetadata.group = i["group"].get<std::string>().c_str();
    myMetadataLayer->layerMetadata.abstract = i["abstract"].get<std::string>().c_str();
    myMetadataLayer->layerMetadata.isQueryable = i["isqueryable"].get<int>();
    myMetadataLayer->layerMetadata.nativeEPSG = i["nativeepsg"].get<std::string>().c_str();

    json latlonbox = i["latlonbox"];
    for (size_t j = 0; j < 4; j += 1) {
      latlonbox[j].get_to((myMetadataLayer->layerMetadata.dfLatLonBBOX[j]));
    }
    json gridspec = i["gridspec"];
    json bbox = gridspec["bbox"];
    for (size_t j = 0; j < 4; j += 1) {
      bbox[j].get_to((myMetadataLayer->layerMetadata.dfBBOX[j]));
    }
    myMetadataLayer->layerMetadata.width = gridspec["width"].get<int>();
    myMetadataLayer->layerMetadata.height = gridspec["height"].get<int>();
    myMetadataLayer->layerMetadata.cellsizeX = gridspec["cellsizex"].get<double>();
    myMetadataLayer->layerMetadata.cellsizeY = gridspec["cellsizey"].get<double>();
    myMetadataLayer->layerMetadata.projstring = gridspec["projstring"].get<std::string>().c_str();
    auto c = i["variables"];
    for (auto styleJson : c.items()) {
      auto variableProps = styleJson.value();
      LayerMetadataVariable *variable = new LayerMetadataVariable();
      myMetadataLayer->layerMetadata.variableList.push_back(variable);
      variable->units = variableProps["units"].get<std::string>().c_str();
      variable->label = variableProps["label"].get<std::string>().c_str();
      variable->variableName = variableProps["variableName"].get<std::string>().c_str();
    }

  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  } catch (int e) {
    CDBError("loadLayerMetadataStructFromMetadataDb %d", e);
    return e;
  }
  return 0;
}

int storeLayerProjectionAndExtentListIntoMetadataDb(MetadataLayer *myMetadataLayer) {
  try {
    json projsettings;
    if (getProjectionListAsJson(myMetadataLayer, projsettings) != 0) {
      return 1;
    }
    storeLayerMetadataInDb(myMetadataLayer, "projected_extents", projsettings.dump());
  } catch (int e) {
    return e;
  }
  return 0;
}

int loadLayerProjectionAndExtentListFromMetadataDb(MetadataLayer *myMetadataLayer) {
  if (!myMetadataLayer->readFromDb) {
    return 1;
  }
  if (myMetadataLayer->layerMetadata.projectionList.size() != 0) {
    CDBError("myMetadataLayer->layerMetadata.projectionList is not empty");
    return 1;
  }
  try {
    CT::string projInfo = getLayerMetadataFromDb(myMetadataLayer, "projected_extents");
    if (projInfo.empty()) {
      return 1;
    }
    json a;
    auto c = a.parse(projInfo.c_str());
    for (auto d : c.items()) {
      auto bboxArray = d.value();
      LayerMetadataProjection *projection = new LayerMetadataProjection();
      myMetadataLayer->layerMetadata.projectionList.push_back(projection);
      projection->name = d.key().c_str();
      bboxArray[0].get_to((projection->dfBBOX[0]));
      bboxArray[1].get_to((projection->dfBBOX[1]));
      bboxArray[2].get_to((projection->dfBBOX[2]));
      bboxArray[3].get_to((projection->dfBBOX[3]));
    }
  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  } catch (int e) {
    // CDBError("loadLayerProjectionAndExtentListFromMetadataDb %d", e);
    return e;
  }
  return 0;
}

int storeLayerStyleListIntoMetadataDb(MetadataLayer *myMetadataLayer) {
  try {
    json styleListJson;
    if (getStyleListMetadataAsJson(myMetadataLayer, styleListJson) != 0) {
      return 1;
    }
    storeLayerMetadataInDb(myMetadataLayer, "stylelist", styleListJson.dump());
  } catch (int e) {
    return e;
  }
  return 0;
}

int loadLayerStyleListFromMetadataDb(MetadataLayer *myMetadataLayer) {
  if (myMetadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeCascaded || myMetadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate) {
    return 0;
  }
  if (!myMetadataLayer->readFromDb) {
    return 1;
  }

  if (myMetadataLayer->layerMetadata.styleList.size() != 0) {
    CDBError("myMetadataLayer->layerMetadata.styleList is not empty");
    return 1;
  }
  try {

    CT::string styleListAsJson = getLayerMetadataFromDb(myMetadataLayer, "stylelist");
    if (styleListAsJson.empty()) {
      return 1;
    }

    json a;
    auto c = a.parse(styleListAsJson.c_str());

    for (auto styleJson : c.items()) {
      auto styleProperties = styleJson.value();
      LayerMetadataStyle *style = new LayerMetadataStyle();
      myMetadataLayer->layerMetadata.styleList.push_back(style);
      style->name = styleProperties["name"].get<std::string>().c_str();
      style->abstract = styleProperties["abstract"].get<std::string>().c_str();
      style->title = styleProperties["title"].get<std::string>().c_str();
    }

  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  } catch (int e) {
    // CDBError("loadLayerStyleListFromMetadataDb %d", e);
    return e;
  }
  return 0;
}

int storeLayerDimensionListIntoMetadataDb(MetadataLayer *myMetadataLayer) {
  CDBDebug("storeLayerDimensionListIntoMetadataDb");
  try {
    json dimListJson;
    if (getDimensionListAsJson(myMetadataLayer, dimListJson) != 0) {
      return 1;
    }
    storeLayerMetadataInDb(myMetadataLayer, "dimensionlist", dimListJson.dump());
  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  } catch (int e) {
    return e;
  }
  return 0;
}

int loadLayerDimensionListFromMetadataDb(MetadataLayer *myMetadataLayer) {
  if (!myMetadataLayer->readFromDb) {
    return 1;
  }
  if (myMetadataLayer->layerMetadata.dimList.size() != 0) {
    CDBError("myMetadataLayer->layerMetadata.dimList is not empty");
    return 1;
  }
  try {
    CT::string dimensionListAsJson = getLayerMetadataFromDb(myMetadataLayer, "dimensionlist");
    if (dimensionListAsJson.empty()) {
      return 1;
    }

    json a;
    auto c = a.parse(dimensionListAsJson.c_str());

    for (auto d : c.items()) {
      auto dimensionProperties = d.value();

      LayerMetadataDim *dimension = new LayerMetadataDim();
      myMetadataLayer->layerMetadata.dimList.push_back(dimension);
      dimension->name = d.key().c_str();
      dimension->defaultValue = dimensionProperties["defaultValue"].get<std::string>().c_str();
      dimension->hasMultipleValues = dimensionProperties["hasMultipleValues"].get<int>();
      dimension->hidden = dimensionProperties["hidden"].get<bool>();
      dimension->name = dimensionProperties["name"].get<std::string>().c_str();
      dimension->units = dimensionProperties["units"].get<std::string>().c_str();
      dimension->values = dimensionProperties["values"].get<std::string>().c_str();
    }
  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  } catch (int e) {
    return e;
  }
  return 0;
}

int updateMetaDataTable(CDataSource *dataSource) {

  if (dataSource->srvParams->datasetLocation.empty()) {
    return 0;
  }
  MetadataLayer *myMetadataLayer = new MetadataLayer();
  myMetadataLayer->layer = dataSource->cfgLayer;
  myMetadataLayer->srvParams = dataSource->srvParams;
  myMetadataLayer->dataSource = dataSource;
  populateLayerMetadataStruct(myMetadataLayer, false);
  storeMyMetadataLayerIntoMetadataDb(myMetadataLayer);
  delete myMetadataLayer;
  return 0;
}