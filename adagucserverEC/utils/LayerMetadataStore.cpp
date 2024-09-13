#include "./LayerMetadataStore.h"

#include <json_adaguc.h>
#include <CDBFactory.h>
#include "XMLGenUtils.h"

int getDimensionListAsJson(MetadataLayer *metadataLayer, json &dimListJson) {
  try {

    for (auto dimension : metadataLayer->layerMetadata.dimList) {
      json item;
      item["defaultValue"] = dimension.defaultValue.c_str();
      item["hasMultipleValues"] = dimension.hasMultipleValues;
      item["hidden"] = dimension.hidden;
      item["name"] = dimension.name.c_str();
      item["units"] = dimension.units.c_str();
      item["values"] = dimension.values.c_str();
      dimListJson[dimension.name.c_str()] = item;
    }
  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  }
  return 0;
}

int getLayerBaseMetadataAsJson(MetadataLayer *metadataLayer, json &layerMetadataItem) {
  try {
    layerMetadataItem["name"] = metadataLayer->layerMetadata.name;
    layerMetadataItem["title"] = metadataLayer->layerMetadata.title;
    layerMetadataItem["group"] = metadataLayer->layerMetadata.group;
    layerMetadataItem["abstract"] = metadataLayer->layerMetadata.abstract;
    layerMetadataItem["nativeepsg"] = metadataLayer->layerMetadata.nativeEPSG;

    layerMetadataItem["isqueryable"] = metadataLayer->layerMetadata.isQueryable;
    json latlonbox;
    for (size_t j = 0; j < 4; j++) {
      latlonbox.push_back(metadataLayer->layerMetadata.dfLatLonBBOX[j]);
    }
    layerMetadataItem["latlonbox"] = latlonbox;

    json gridspec;
    json bbox;
    for (size_t j = 0; j < 4; j++) {
      bbox.push_back(metadataLayer->layerMetadata.dfBBOX[j]);
    }
    gridspec["bbox"] = bbox;
    gridspec["width"] = metadataLayer->layerMetadata.width;
    gridspec["height"] = metadataLayer->layerMetadata.height;
    gridspec["cellsizex"] = metadataLayer->layerMetadata.cellsizeX;
    gridspec["cellsizey"] = metadataLayer->layerMetadata.cellsizeY;
    gridspec["projstring"] = metadataLayer->layerMetadata.projstring;

    layerMetadataItem["gridspec"] = gridspec;

    json variables;
    for (auto lv : metadataLayer->layerMetadata.variableList) {
      json variable;
      variable["units"] = lv.units;
      variable["label"] = lv.label;
      variable["variableName"] = lv.variableName;
      variables.push_back(variable);
    }
    layerMetadataItem["variables"] = variables;

  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  }
  return 0;
}
int getProjectionListAsJson(MetadataLayer *metadataLayer, json &projsettings) {
  try {
    for (auto projection : metadataLayer->layerMetadata.projectionList) {
      json item = {projection.dfBBOX[0], projection.dfBBOX[1], projection.dfBBOX[2], projection.dfBBOX[3]};
      projsettings[projection.name.c_str()] = item;
    }
  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  }
  return 0;
}

int getStyleListMetadataAsJson(MetadataLayer *metadataLayer, json &styleListJson) {
  try {
    for (auto style : metadataLayer->layerMetadata.styleList) {
      json item;
      item["abstract"] = style.abstract.c_str();
      item["title"] = style.title.c_str();
      item["name"] = style.name.c_str();
      styleListJson.push_back(item);
    }
  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  }
  return 0;
}

int storemetadataLayerIntoMetadataDb(MetadataLayer *metadataLayer) {
  storeLayerMetadataStructIntoMetadataDb(metadataLayer);
  storeLayerDimensionListIntoMetadataDb(metadataLayer);
  storeLayerProjectionAndExtentListIntoMetadataDb(metadataLayer);
  storeLayerStyleListIntoMetadataDb(metadataLayer);
  return 0;
}

int loadmetadataLayerFromMetadataDb(MetadataLayer *metadataLayer) {
  loadLayerMetadataStructFromMetadataDb(metadataLayer);
  loadLayerDimensionListFromMetadataDb(metadataLayer);
  loadLayerProjectionAndExtentListFromMetadataDb(metadataLayer);
  loadLayerStyleListFromMetadataDb(metadataLayer);
  return 0;
}

CT::string getLayerMetadataFromDb(MetadataLayer *metadataLayer, CT::string metadataKey) {
  CT::string layerName = metadataLayer->dataSource->getLayerName();
  CT::string datasetName = metadataLayer->dataSource->srvParams->datasetLocation;
  if (datasetName.empty()) {
    // CDBDebug("Not a dataset");
    return "";
  }
  CDBStore::Store *layerMetaDataStore = CDBFactory::getDBAdapter(metadataLayer->dataSource->srvParams->cfg)->getLayerMetadataStore(datasetName);
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
  CDBDebug("No metadata entry found for %s %s %s", datasetName.c_str(), layerName.c_str(), metadataKey.c_str());
  throw __LINE__;
}

int storeLayerMetadataInDb(MetadataLayer *metadataLayer, CT::string metadataKey, std::string metadataBlob) {
  try {
    CT::string datasetName = metadataLayer->dataSource->srvParams->datasetLocation;
    if (datasetName.empty()) {
      CDBDebug("Not a dataset");
      return 1;
    }
    CT::string layerName = metadataLayer->dataSource->getLayerName();

    return CDBFactory::getDBAdapter(metadataLayer->dataSource->srvParams->cfg)->storeLayerMetadata(datasetName, layerName, metadataKey, metadataBlob.c_str());
  } catch (int e) {
    return e;
  }
  return 0;
}

int storeLayerMetadataStructIntoMetadataDb(MetadataLayer *metadataLayer) {

  json layerMetadataItem;

  if (getLayerBaseMetadataAsJson(metadataLayer, layerMetadataItem) != 0) {
    return 1;
  }

  storeLayerMetadataInDb(metadataLayer, "layermetadata", layerMetadataItem.dump());
  return 0;
}

int loadLayerMetadataStructFromMetadataDb(MetadataLayer *metadataLayer) {
  if (!metadataLayer->readFromDb) {
    return 1;
  }
  try {
    CT::string layerMetadataAsJson = getLayerMetadataFromDb(metadataLayer, "layermetadata");
    if (layerMetadataAsJson.empty()) {
      return 1;
    }
    json a;
    auto i = a.parse(layerMetadataAsJson.c_str());
    metadataLayer->layerMetadata.name = i["name"].get<std::string>().c_str();
    metadataLayer->layerMetadata.title = i["title"].get<std::string>().c_str();
    metadataLayer->layerMetadata.group = i["group"].get<std::string>().c_str();
    metadataLayer->layerMetadata.abstract = i["abstract"].get<std::string>().c_str();
    metadataLayer->layerMetadata.isQueryable = i["isqueryable"].get<int>();
    metadataLayer->layerMetadata.nativeEPSG = i["nativeepsg"].get<std::string>().c_str();

    json latlonbox = i["latlonbox"];
    for (size_t j = 0; j < 4; j += 1) {
      latlonbox[j].get_to((metadataLayer->layerMetadata.dfLatLonBBOX[j]));
    }
    json gridspec = i["gridspec"];
    json bbox = gridspec["bbox"];
    for (size_t j = 0; j < 4; j += 1) {
      bbox[j].get_to((metadataLayer->layerMetadata.dfBBOX[j]));
    }
    metadataLayer->layerMetadata.width = gridspec["width"].get<int>();
    metadataLayer->layerMetadata.height = gridspec["height"].get<int>();
    metadataLayer->layerMetadata.cellsizeX = gridspec["cellsizex"].get<double>();
    metadataLayer->layerMetadata.cellsizeY = gridspec["cellsizey"].get<double>();
    metadataLayer->layerMetadata.projstring = gridspec["projstring"].get<std::string>().c_str();
    auto c = i["variables"];
    for (auto styleJson : c.items()) {
      auto variableProps = styleJson.value();
      LayerMetadataVariable variable = {
          .variableName = variableProps["variableName"].get<std::string>().c_str(),
          .units = variableProps["units"].get<std::string>().c_str(),
          .label = variableProps["label"].get<std::string>().c_str(),
      };
      metadataLayer->layerMetadata.variableList.push_back(variable);
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

int storeLayerProjectionAndExtentListIntoMetadataDb(MetadataLayer *metadataLayer) {
  try {
    json projsettings;
    if (getProjectionListAsJson(metadataLayer, projsettings) != 0) {
      return 1;
    }
    storeLayerMetadataInDb(metadataLayer, "projected_extents", projsettings.dump());
  } catch (int e) {
    return e;
  }
  return 0;
}

int loadLayerProjectionAndExtentListFromMetadataDb(MetadataLayer *metadataLayer) {
  if (!metadataLayer->readFromDb) {
    return 1;
  }
  if (metadataLayer->layerMetadata.projectionList.size() != 0) {
    CDBError("metadataLayer->layerMetadata.projectionList is not empty");
    return 1;
  }
  try {
    CT::string projInfo = getLayerMetadataFromDb(metadataLayer, "projected_extents");
    if (projInfo.empty()) {
      return 1;
    }
    json a;
    auto c = json::parse(projInfo.c_str());
    for (const auto& d : c.items()) {
      auto bboxArray = d.value();
      double bbox[4] =
          {
              bboxArray[0].get_to((bbox[0])),
              bboxArray[1].get_to((bbox[1])),
              bboxArray[2].get_to((bbox[2])),
              bboxArray[3].get_to((bbox[3])),
          };
      LayerMetadataProjection projection(d.key().c_str(), bbox);
      projection.name = d.key().c_str();
      metadataLayer->layerMetadata.projectionList.push_back(projection);
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

int storeLayerStyleListIntoMetadataDb(MetadataLayer *metadataLayer) {
  try {
    json styleListJson;
    if (getStyleListMetadataAsJson(metadataLayer, styleListJson) != 0) {
      CDBWarning("Unable to convert stylelist to json");
      return 1;
    }
    storeLayerMetadataInDb(metadataLayer, "stylelist", styleListJson.dump());
  } catch (int e) {
    CDBWarning("Unable to store stylelist json in db");
    return e;
  }
  return 0;
}

int loadLayerStyleListFromMetadataDb(MetadataLayer *metadataLayer) {
  if (metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeCascaded || metadataLayer->dataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate) {
    return 0;
  }
  if (!metadataLayer->readFromDb) {
    return 1;
  }

  if (metadataLayer->layerMetadata.styleList.size() != 0) {
    CDBError("metadataLayer->layerMetadata.styleList is not empty");
    return 1;
  }
  try {

    CT::string styleListAsJson = getLayerMetadataFromDb(metadataLayer, "stylelist");
    if (styleListAsJson.empty()) {
      return 1;
    }

    json a;
    auto c = a.parse(styleListAsJson.c_str());

    for (auto styleJson : c.items()) {
      auto styleProperties = styleJson.value();
      LayerMetadataStyle style = {
          .name = styleProperties["name"].get<std::string>().c_str(),
          .title = styleProperties["title"].get<std::string>().c_str(),
          .abstract = styleProperties["abstract"].get<std::string>().c_str(),
      };
      metadataLayer->layerMetadata.styleList.push_back(style);
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

int storeLayerDimensionListIntoMetadataDb(MetadataLayer *metadataLayer) {
  try {
    json dimListJson;
    if (getDimensionListAsJson(metadataLayer, dimListJson) != 0) {
      return 1;
    }
    storeLayerMetadataInDb(metadataLayer, "dimensionlist", dimListJson.dump());
  } catch (json::exception &e) {
    CDBWarning("Unable to build json structure");
    return 1;
  } catch (int e) {
    return e;
  }
  return 0;
}

int loadLayerDimensionListFromMetadataDb(MetadataLayer *metadataLayer) {
  if (!metadataLayer->readFromDb) {
    return 1;
  }
  if (metadataLayer->layerMetadata.dimList.size() != 0) {
    CDBError("metadataLayer->layerMetadata.dimList is not empty");
    return 1;
  }
  try {
    CT::string dimensionListAsJson = getLayerMetadataFromDb(metadataLayer, "dimensionlist");
    if (dimensionListAsJson.empty()) {
      return 1;
    }

    json a;
    auto c = a.parse(dimensionListAsJson.c_str());

    for (auto d : c.items()) {
      auto dimensionProperties = d.value();

      LayerMetadataDim dimension = {
          .name = dimensionProperties["name"].get<std::string>().c_str(),
          .units = dimensionProperties["units"].get<std::string>().c_str(),
          .values = dimensionProperties["values"].get<std::string>().c_str(),
          .defaultValue = dimensionProperties["defaultValue"].get<std::string>().c_str(),
          .hasMultipleValues = dimensionProperties["hasMultipleValues"].get<int>(),
          .hidden = dimensionProperties["hidden"].get<bool>(),
      };
      metadataLayer->layerMetadata.dimList.push_back(dimension);
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
  MetadataLayer *metadataLayer = new MetadataLayer();
  metadataLayer->layer = dataSource->cfgLayer;
  metadataLayer->srvParams = dataSource->srvParams;
  metadataLayer->dataSource = dataSource;
  populateMetadataLayerStruct(metadataLayer, false);
  storemetadataLayerIntoMetadataDb(metadataLayer);
  delete metadataLayer;
  return 0;
}