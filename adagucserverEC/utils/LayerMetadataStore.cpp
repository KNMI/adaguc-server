#include "./LayerMetadataStore.h"

#include <json_adaguc.h>
#include <CDBFactory.h>
#include "XMLGenUtils.h"

int getDimensionListAsJson(WMSLayer *myWMSLayer, json &dimListJson) {
  try {

    for (auto dimension : myWMSLayer->layerMetadata.dimList) {
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
    CDBError("Unable to build json structure");
    return 1;
  }
  return 0;
}

int getLayerBaseMetadataAsJson(WMSLayer *myWMSLayer, json &layerMetadataItem) {
  try {
    layerMetadataItem["name"] = myWMSLayer->layerMetadata.name;
    layerMetadataItem["title"] = myWMSLayer->layerMetadata.title;
    layerMetadataItem["group"] = myWMSLayer->layerMetadata.group;
    layerMetadataItem["abstract"] = myWMSLayer->layerMetadata.abstract;
    layerMetadataItem["nativeepsg"] = myWMSLayer->layerMetadata.nativeEPSG;
    layerMetadataItem["isqueryable"] = myWMSLayer->layerMetadata.isQueryable;
    json latlonbox;
    for (size_t j = 0; j < 4; j++) {
      latlonbox.push_back(myWMSLayer->layerMetadata.dfLatLonBBOX[j]);
    }
    layerMetadataItem["latlonbox"] = latlonbox;

    json gridspec;
    json bbox;
    for (size_t j = 0; j < 4; j++) {
      bbox.push_back(myWMSLayer->layerMetadata.dfBBOX[j]);
    }
    gridspec["bbox"] = bbox;
    gridspec["width"] = myWMSLayer->layerMetadata.width;
    gridspec["height"] = myWMSLayer->layerMetadata.height;
    gridspec["cellsizex"] = myWMSLayer->layerMetadata.cellsizeX;
    gridspec["cellsizey"] = myWMSLayer->layerMetadata.cellsizeY;
    layerMetadataItem["gridspec"] = gridspec;

    json variables;
    for (auto lv : myWMSLayer->layerMetadata.variableList) {
      json variable;
      variable["units"] = lv->units;
      variables.push_back(variable);
    }
    layerMetadataItem["variables"] = variables;

  } catch (json::exception &e) {
    CDBError("Unable to build json structure");
    return 1;
  }
  return 0;
}
int getProjectionListAsJson(WMSLayer *myWMSLayer, json &projsettings) {
  try {
    for (auto projection : myWMSLayer->layerMetadata.projectionList) {
      json item = {projection->dfBBOX[0], projection->dfBBOX[1], projection->dfBBOX[2], projection->dfBBOX[3]};
      projsettings[projection->name.c_str()] = item;
    }
  } catch (json::exception &e) {
    CDBError("Unable to build json structure");
    return 1;
  }
  return 0;
}

int getStyleListMetadataAsJson(WMSLayer *myWMSLayer, json &styleListJson) {
  try {

    for (auto style : myWMSLayer->layerMetadata.styleList) {
      json item;
      item["abstract"] = style->abstract.c_str();
      item["title"] = style->title.c_str();
      item["name"] = style->name.c_str();
      styleListJson.push_back(item);
    }
  } catch (json::exception &e) {
    CDBError("Unable to build json structure");
    return 1;
  }
  return 0;
}

int storeMyWMSLayerIntoMetadataDb(WMSLayer *myWMSLayer) {
  storeLayerMetadataStructIntoMetadataDb(myWMSLayer);
  storeLayerDimensionListIntoMetadataDb(myWMSLayer);
  storeLayerProjectionAndExtentListIntoMetadataDb(myWMSLayer);
  storeLayerStyleListIntoMetadataDb(myWMSLayer);
  return 0;
}

int loadMyWMSLayerFromMetadataDb(WMSLayer *myWMSLayer) {
  loadLayerMetadataStructFromMetadataDb(myWMSLayer);
  loadLayerDimensionListFromMetadataDb(myWMSLayer);
  loadLayerProjectionAndExtentListFromMetadataDb(myWMSLayer);
  loadLayerStyleListFromMetadataDb(myWMSLayer);
  return 0;
}

CT::string getLayerMetadataFromDb(WMSLayer *myWMSLayer, CT::string metadataKey) {
  CT::string layerName = myWMSLayer->dataSource->getLayerName();
  CT::string datasetName = myWMSLayer->dataSource->srvParams->datasetLocation;
  if (datasetName.empty()) {
    // CDBDebug("Not a dataset");
    return "";
  }
  return CDBFactory::getDBAdapter(myWMSLayer->dataSource->srvParams->cfg)->getLayerMetadata(datasetName, layerName, metadataKey);
}

int storeLayerMetadataInDb(WMSLayer *myWMSLayer, CT::string metadataKey, std::string metadataBlob) {
  try {
    CT::string datasetName = myWMSLayer->dataSource->srvParams->datasetLocation;
    if (datasetName.empty()) {
      CDBDebug("Not a dataset");
      return 1;
    }
    CT::string layerName = myWMSLayer->dataSource->getLayerName();

    return CDBFactory::getDBAdapter(myWMSLayer->dataSource->srvParams->cfg)->storeLayerMetadata(datasetName, layerName, metadataKey, metadataBlob.c_str());
  } catch (int e) {
    return e;
  }
  return 0;
}

int storeLayerMetadataStructIntoMetadataDb(WMSLayer *myWMSLayer) {
  json layerMetadataItem;

  if (getLayerBaseMetadataAsJson(myWMSLayer, layerMetadataItem) != 0) {
    return 1;
  }

  storeLayerMetadataInDb(myWMSLayer, "layermetadata", layerMetadataItem.dump());
  return 0;
}

int loadLayerMetadataStructFromMetadataDb(WMSLayer *myWMSLayer) {
  if (!myWMSLayer->readFromDb) {
    return 1;
  }
  try {
    CT::string layerMetadataAsJson = getLayerMetadataFromDb(myWMSLayer, "layermetadata");
    if (layerMetadataAsJson.empty()) {
      return 1;
    }
    json a;
    auto i = a.parse(layerMetadataAsJson.c_str());
    myWMSLayer->layerMetadata.name = i["name"].get<std::string>().c_str();
    myWMSLayer->layerMetadata.title = i["title"].get<std::string>().c_str();
    myWMSLayer->layerMetadata.group = i["group"].get<std::string>().c_str();
    myWMSLayer->layerMetadata.abstract = i["abstract"].get<std::string>().c_str();
    myWMSLayer->layerMetadata.isQueryable = i["isqueryable"].get<int>();
    myWMSLayer->layerMetadata.nativeEPSG = i["nativeepsg"].get<std::string>().c_str();

    json latlonbox = i["latlonbox"];
    for (size_t j = 0; j < 4; j += 1) {
      latlonbox[j].get_to((myWMSLayer->layerMetadata.dfLatLonBBOX[j]));
    }
    json gridspec = i["gridspec"];
    json bbox = gridspec["bbox"];
    for (size_t j = 0; j < 4; j += 1) {
      bbox[j].get_to((myWMSLayer->layerMetadata.dfBBOX[j]));
    }
    myWMSLayer->layerMetadata.width = gridspec["width"].get<int>();
    myWMSLayer->layerMetadata.height = gridspec["height"].get<int>();
    myWMSLayer->layerMetadata.cellsizeX = gridspec["cellsizex"].get<double>();
    myWMSLayer->layerMetadata.cellsizeY = gridspec["cellsizey"].get<double>();
    auto c = i["variables"];
    for (auto styleJson : c.items()) {
      auto variableProps = styleJson.value();
      LayerMetadataVariable *variable = new LayerMetadataVariable();
      myWMSLayer->layerMetadata.variableList.push_back(variable);
      variable->units = variableProps["units"].get<std::string>().c_str();
    }

  } catch (json::exception &e) {
    CDBError("Unable to build json structure");
    return 1;
  } catch (int e) {
    CDBError("loadLayerMetadataStructFromMetadataDb %d", e);
    return e;
  }
  return 0;
}

int storeLayerProjectionAndExtentListIntoMetadataDb(WMSLayer *myWMSLayer) {
  try {
    json projsettings;
    if (getProjectionListAsJson(myWMSLayer, projsettings) != 0) {
      return 1;
    }
    storeLayerMetadataInDb(myWMSLayer, "projected_extents", projsettings.dump());
  } catch (int e) {
    return e;
  }
  return 0;
}

int loadLayerProjectionAndExtentListFromMetadataDb(WMSLayer *myWMSLayer) {
  if (!myWMSLayer->readFromDb) {
    return 1;
  }
  if (myWMSLayer->layerMetadata.projectionList.size() != 0) {
    CDBError("myWMSLayer->layerMetadata.projectionList is not empty");
    return 1;
  }
  try {
    CT::string projInfo = getLayerMetadataFromDb(myWMSLayer, "projected_extents");
    if (projInfo.empty()) {
      return 1;
    }
    json a;
    auto c = a.parse(projInfo.c_str());
    for (auto d : c.items()) {
      auto bboxArray = d.value();
      LayerMetadataProjection *projection = new LayerMetadataProjection();
      myWMSLayer->layerMetadata.projectionList.push_back(projection);
      projection->name = d.key().c_str();
      bboxArray[0].get_to((projection->dfBBOX[0]));
      bboxArray[1].get_to((projection->dfBBOX[1]));
      bboxArray[2].get_to((projection->dfBBOX[2]));
      bboxArray[3].get_to((projection->dfBBOX[3]));
    }
  } catch (json::exception &e) {
    CDBError("Unable to build json structure");
    return 1;
  } catch (int e) {
    // CDBError("loadLayerProjectionAndExtentListFromMetadataDb %d", e);
    return e;
  }
  return 0;
}

int storeLayerStyleListIntoMetadataDb(WMSLayer *myWMSLayer) {
  try {
    json styleListJson;
    if (getStyleListMetadataAsJson(myWMSLayer, styleListJson) != 0) {
      return 1;
    }
    storeLayerMetadataInDb(myWMSLayer, "stylelist", styleListJson.dump());
  } catch (int e) {
    return e;
  }
  return 0;
}

int loadLayerStyleListFromMetadataDb(WMSLayer *myWMSLayer) {
  if (myWMSLayer->dataSource->dLayerType == CConfigReaderLayerTypeCascaded || myWMSLayer->dataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate) {
    return 0;
  }
  if (!myWMSLayer->readFromDb) {
    return 1;
  }

  if (myWMSLayer->layerMetadata.styleList.size() != 0) {
    CDBError("myWMSLayer->layerMetadata.styleList is not empty");
    return 1;
  }
  try {

    CT::string styleListAsJson = getLayerMetadataFromDb(myWMSLayer, "stylelist");
    if (styleListAsJson.empty()) {
      return 1;
    }

    json a;
    auto c = a.parse(styleListAsJson.c_str());

    for (auto styleJson : c.items()) {
      auto styleProperties = styleJson.value();
      LayerMetadataStyle *style = new LayerMetadataStyle();
      myWMSLayer->layerMetadata.styleList.push_back(style);
      style->name = styleProperties["name"].get<std::string>().c_str();
      style->abstract = styleProperties["abstract"].get<std::string>().c_str();
      style->title = styleProperties["title"].get<std::string>().c_str();
    }

  } catch (json::exception &e) {
    CDBError("Unable to build json structure");
    return 1;
  } catch (int e) {
    // CDBError("loadLayerStyleListFromMetadataDb %d", e);
    return e;
  }
  return 0;
}

int storeLayerDimensionListIntoMetadataDb(WMSLayer *myWMSLayer) {
  CDBDebug("storeLayerDimensionListIntoMetadataDb");
  try {
    json dimListJson;
    if (getDimensionListAsJson(myWMSLayer, dimListJson) != 0) {
      return 1;
    }
    storeLayerMetadataInDb(myWMSLayer, "dimensionlist", dimListJson.dump());
  } catch (json::exception &e) {
    CDBError("Unable to build json structure");
    return 1;
  } catch (int e) {
    return e;
  }
  return 0;
}

int loadLayerDimensionListFromMetadataDb(WMSLayer *myWMSLayer) {
  if (!myWMSLayer->readFromDb) {
    return 1;
  }
  if (myWMSLayer->layerMetadata.dimList.size() != 0) {
    CDBError("myWMSLayer->layerMetadata.dimList is not empty");
    return 1;
  }
  try {
    CT::string dimensionListAsJson = getLayerMetadataFromDb(myWMSLayer, "dimensionlist");
    if (dimensionListAsJson.empty()) {
      return 1;
    }

    json a;
    auto c = a.parse(dimensionListAsJson.c_str());

    for (auto d : c.items()) {
      auto dimensionProperties = d.value();

      LayerMetadataDim *dimension = new LayerMetadataDim();
      myWMSLayer->layerMetadata.dimList.push_back(dimension);
      dimension->name = d.key().c_str();
      dimension->defaultValue = dimensionProperties["defaultValue"].get<std::string>().c_str();
      dimension->hasMultipleValues = dimensionProperties["hasMultipleValues"].get<int>();
      dimension->hidden = dimensionProperties["hidden"].get<bool>();
      dimension->name = dimensionProperties["name"].get<std::string>().c_str();
      dimension->units = dimensionProperties["units"].get<std::string>().c_str();
      dimension->values = dimensionProperties["values"].get<std::string>().c_str();
    }
  } catch (json::exception &e) {
    CDBError("Unable to build json structure");
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
  WMSLayer *myWMSLayer = new WMSLayer();
  myWMSLayer->layer = dataSource->cfgLayer;
  myWMSLayer->srvParams = dataSource->srvParams;
  myWMSLayer->dataSource = dataSource;
  populateMyWMSLayerStruct(myWMSLayer, false);
  storeMyWMSLayerIntoMetadataDb(myWMSLayer);
  delete myWMSLayer;
  return 0;
}