#include "./LayerMetadataStore.h"

#include <json_adaguc.h>
#include <CDBFactory.h>
#include "XMLGenUtils.h"

int storeMetadataLayerIntoMetadataDb(MetadataLayer *metadataLayer) {
  storeLayerMetadataStructIntoMetadataDb(metadataLayer);
  storeLayerDimensionListIntoMetadataDb(metadataLayer);
  storeLayerProjectionAndExtentListIntoMetadataDb(metadataLayer);
  storeLayerStyleListIntoMetadataDb(metadataLayer);
  return 0;
}

CT::string getLayerMetadataFromDb(MetadataLayer *metadataLayer, CT::string metadataKey) {
  CT::string layerName = metadataLayer->dataSource->getLayerName();
  CT::string datasetName = metadataLayer->dataSource->srvParams->datasetLocation;
  if (datasetName.empty()) {
    // CDBDebug("Not a dataset");
    return "";
  }
  return CDBFactory::getDBAdapter(metadataLayer->dataSource->srvParams->cfg)->getLayerMetadata(datasetName, layerName, metadataKey);
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
  layerMetadataItem["name"] = metadataLayer->layerMetadata.name;
  layerMetadataItem["title"] = metadataLayer->layerMetadata.title;
  layerMetadataItem["group"] = metadataLayer->layerMetadata.group;
  layerMetadataItem["abstract"] = metadataLayer->layerMetadata.abstract;
  layerMetadataItem["nativeepsg"] = metadataLayer->layerMetadata.nativeEPSG;
  layerMetadataItem["isqueryable"] = metadataLayer->layerMetadata.isQueryable;
  layerMetadataItem["latlonboxleft"] = metadataLayer->layerMetadata.dfLatLonBBOX[0];
  layerMetadataItem["latlonboxright"] = metadataLayer->layerMetadata.dfLatLonBBOX[1];
  layerMetadataItem["latlonboxbottom"] = metadataLayer->layerMetadata.dfLatLonBBOX[2];
  layerMetadataItem["latlonboxtop"] = metadataLayer->layerMetadata.dfLatLonBBOX[3];
  layerMetadataItem["bboxleft"] = metadataLayer->layerMetadata.dfBBOX[0];
  layerMetadataItem["bboxright"] = metadataLayer->layerMetadata.dfBBOX[1];
  layerMetadataItem["bboxbottom"] = metadataLayer->layerMetadata.dfBBOX[2];
  layerMetadataItem["bboxtop"] = metadataLayer->layerMetadata.dfBBOX[3];
  layerMetadataItem["width"] = metadataLayer->layerMetadata.width;
  layerMetadataItem["height"] = metadataLayer->layerMetadata.height;
  layerMetadataItem["cellsizex"] = metadataLayer->layerMetadata.cellsizeX;
  layerMetadataItem["cellsizey"] = metadataLayer->layerMetadata.cellsizeY;
  json variables;
  for (auto lv : metadataLayer->layerMetadata.variableList) {
    json variable;
    variable["units"] = lv.units;
    variables.push_back(variable);
  }
  layerMetadataItem["variables"] = variables;

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
    metadataLayer->layerMetadata.dfLatLonBBOX[0] = i["latlonboxleft"].get<double>();
    metadataLayer->layerMetadata.dfLatLonBBOX[1] = i["latlonboxright"].get<double>();
    metadataLayer->layerMetadata.dfLatLonBBOX[2] = i["latlonboxbottom"].get<double>();
    metadataLayer->layerMetadata.dfLatLonBBOX[3] = i["latlonboxtop"].get<double>();
    metadataLayer->layerMetadata.dfBBOX[0] = i["bboxleft"].get<double>();
    metadataLayer->layerMetadata.dfBBOX[1] = i["bboxright"].get<double>();
    metadataLayer->layerMetadata.dfBBOX[2] = i["bboxbottom"].get<double>();
    metadataLayer->layerMetadata.dfBBOX[3] = i["bboxtop"].get<double>();
    metadataLayer->layerMetadata.width = i["width"].get<int>();
    metadataLayer->layerMetadata.height = i["height"].get<int>();
    metadataLayer->layerMetadata.cellsizeX = i["cellsizex"].get<double>();
    metadataLayer->layerMetadata.cellsizeY = i["cellsizey"].get<double>();
    auto c = i["variables"];
    for (auto styleJson : c.items()) {
      auto variableProps = styleJson.value();
      LayerMetadataVariable variable = {variableProps["units"].get<std::string>().c_str()};
      metadataLayer->layerMetadata.variableList.push_back(variable);
    }
  } catch (json::exception &e) {
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
    for (auto projection : metadataLayer->layerMetadata.projectionList) {
      json item = {projection.dfBBOX[0], projection.dfBBOX[1], projection.dfBBOX[2], projection.dfBBOX[3]};
      projsettings[projection.name.c_str()] = item;
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
    auto c = a.parse(projInfo.c_str());
    for (auto d : c.items()) {
      auto bboxArray = d.value();
      LayerMetadataProjection projection;
      projection.name = d.key().c_str();
      bboxArray[0].get_to((projection.dfBBOX[0]));
      bboxArray[1].get_to((projection.dfBBOX[1]));
      bboxArray[2].get_to((projection.dfBBOX[2]));
      bboxArray[3].get_to((projection.dfBBOX[3]));
      metadataLayer->layerMetadata.projectionList.push_back(projection);
    }
  } catch (json::exception &e) {
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
    for (auto style : metadataLayer->layerMetadata.styleList) {
      json item;
      item["abstract"] = style.abstract.c_str();
      item["title"] = style.title.c_str();
      item["name"] = style.name.c_str();
      styleListJson.push_back(item);
    }
    storeLayerMetadataInDb(metadataLayer, "stylelist", styleListJson.dump());
  } catch (int e) {
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
        name : styleProperties["name"].get<std::string>().c_str(),
        title : styleProperties["title"].get<std::string>().c_str(),
        abstract : styleProperties["abstract"].get<std::string>().c_str(),
      };
      metadataLayer->layerMetadata.styleList.push_back(style);
    }

  } catch (json::exception &e) {
    return 1;
  } catch (int e) {
    // CDBError("loadLayerStyleListFromMetadataDb %d", e);
    return e;
  }
  return 0;
}

int storeLayerDimensionListIntoMetadataDb(MetadataLayer *metadataLayer) {
  CDBDebug("storeLayerDimensionListIntoMetadataDb");
  try {
    json dimListJson;
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
    storeLayerMetadataInDb(metadataLayer, "dimensionlist", dimListJson.dump());
  } catch (json::exception &e) {
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
        name : dimensionProperties["name"].get<std::string>().c_str(),
        units : dimensionProperties["units"].get<std::string>().c_str(),
        values : dimensionProperties["values"].get<std::string>().c_str(),
        defaultValue : dimensionProperties["defaultValue"].get<std::string>().c_str(),
        hasMultipleValues : dimensionProperties["hasMultipleValues"].get<int>(),
        hidden : dimensionProperties["hidden"].get<bool>(),
      };
      metadataLayer->layerMetadata.dimList.push_back(dimension);
    }
  } catch (json::exception &e) {
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
  storeMetadataLayerIntoMetadataDb(metadataLayer);
  delete metadataLayer;
  return 0;
}