#include "./LayerMetadataStore.h"

#include <json_adaguc.h>
#include <CDBFactory.h>
#include "XMLGenUtils.h"

int storeMyWMSLayerIntoMetadataDb(WMSLayer *myWMSLayer) {
  storeLayerMetadataStructIntoMetadataDb(myWMSLayer);
  storeLayerDimensionListIntoMetadataDb(myWMSLayer);
  storeLayerProjectionAndExtentListIntoMetadataDb(myWMSLayer);
  storeLayerStyleListIntoMetadataDb(myWMSLayer);
  return 0;
}

// Not sure if this will be used
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
}

int storeLayerMetadataStructIntoMetadataDb(WMSLayer *myWMSLayer) {
  json layerMetadataItem;
  // layerMetadataItem["name"] = myWMSLayer->layerMetadata.name;
  // layerMetadataItem["title"] = myWMSLayer->layerMetadata.title;
  // layerMetadataItem["group"] = myWMSLayer->layerMetadata.group;
  // layerMetadataItem["abstract"] = myWMSLayer->layerMetadata.abstract;
  // layerMetadataItem["isQueryable"] = myWMSLayer->layerMetadata.isQueryable;
  layerMetadataItem["latlonboxleft"] = myWMSLayer->layerMetadata.dfLatLonBBOX[0];
  layerMetadataItem["latlonboxright"] = myWMSLayer->layerMetadata.dfLatLonBBOX[1];
  layerMetadataItem["latlonboxbottom"] = myWMSLayer->layerMetadata.dfLatLonBBOX[2];
  layerMetadataItem["latlonboxtop"] = myWMSLayer->layerMetadata.dfLatLonBBOX[3];
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
    // myWMSLayer->layerMetadata.name = i["name"].get<std::string>().c_str();
    // myWMSLayer->layerMetadata.title = i["title"].get<std::string>().c_str();
    // myWMSLayer->layerMetadata.group = i["group"].get<std::string>().c_str();
    // myWMSLayer->layerMetadata.abstract = i["abstract"].get<std::string>().c_str();
    // myWMSLayer->layerMetadata.isQueryable = i["isQueryable"].get<int>();
    myWMSLayer->layerMetadata.dfLatLonBBOX[0] = i["latlonboxleft"].get<double>();
    myWMSLayer->layerMetadata.dfLatLonBBOX[1] = i["latlonboxright"].get<double>();
    myWMSLayer->layerMetadata.dfLatLonBBOX[2] = i["latlonboxbottom"].get<double>();
    myWMSLayer->layerMetadata.dfLatLonBBOX[3] = i["latlonboxtop"].get<double>();
  } catch (int e) {
    CDBError("loadLayerMetadataStructFromMetadataDb %d", e);
    return e;
  }
  return 0;
}

int storeLayerProjectionAndExtentListIntoMetadataDb(WMSLayer *myWMSLayer) {
  try {
    json projsettings;
    for (auto projection : myWMSLayer->layerMetadata.projectionList) {
      json item = {projection->dfBBOX[0], projection->dfBBOX[1], projection->dfBBOX[2], projection->dfBBOX[3]};
      projsettings[projection->name.c_str()] = item;
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
  } catch (int e) {
    // CDBError("loadLayerProjectionAndExtentListFromMetadataDb %d", e);
    return e;
  }
  return 0;
}

int storeLayerStyleListIntoMetadataDb(WMSLayer *myWMSLayer) {
  try {
    json styleListJson;
    for (auto style : myWMSLayer->layerMetadata.styleList) {
      json item;
      item["abstract"] = style->abstract.c_str();
      item["title"] = style->title.c_str();
      item["name"] = style->name.c_str();
      styleListJson.push_back(item);
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
    storeLayerMetadataInDb(myWMSLayer, "dimensionlist", dimListJson.dump());
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

  } catch (int e) {
    // CDBDebug("loadLayerDimensionListFromMetadataDb %d", e);
    return e;
  }
  return 0;
}

int updateMetaDataTable(CDataSource *dataSource) {
  // return 0;
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