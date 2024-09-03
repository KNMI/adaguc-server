#include "./LayerMetadataStore.h"

#include <json_adaguc.h>
#include <CDBFactory.h>

std::string convertProjectionListToJsonString(std::vector<WMSLayer::Projection *> projectionList) {
  json projsettings;
  for (auto projection : projectionList) {
    json item = {projection->dfBBOX[0], projection->dfBBOX[1], projection->dfBBOX[2], projection->dfBBOX[3]};
    projsettings[projection->name.c_str()] = item;
  }
  std::string projSettingsAsJsonString = projsettings.dump();
  return projSettingsAsJsonString;
}

int storeProjectionList(WMSLayer *myWMSLayer) {
  try {
    CT::string tableName =
        CDBFactory::getDBAdapter(myWMSLayer->dataSource->srvParams->cfg)
            ->getTableNameForPathFilterAndDimension(myWMSLayer->layer->FilePath[0]->value.c_str(), myWMSLayer->layer->FilePath[0]->attr.filter.c_str(), "metadata", myWMSLayer->dataSource);

    std::string projSettingsAsJsonString = convertProjectionListToJsonString(myWMSLayer->projectionList);

    CDBFactory::getDBAdapter(myWMSLayer->dataSource->srvParams->cfg)->storeLayerMetadata(tableName, "projected_extents", projSettingsAsJsonString.c_str());
  } catch (int e) {
    return e;
  }
  return 0;
}

int getProjectionList(WMSLayer *myWMSLayer) {
  if (myWMSLayer->projectionList.size() != 0) {
    return 1;
  }
  try {
    CT::string tableName =
        CDBFactory::getDBAdapter(myWMSLayer->dataSource->srvParams->cfg)
            ->getTableNameForPathFilterAndDimension(myWMSLayer->layer->FilePath[0]->value.c_str(), myWMSLayer->layer->FilePath[0]->attr.filter.c_str(), "metadata", myWMSLayer->dataSource);

    CT::string projInfo = CDBFactory::getDBAdapter(myWMSLayer->dataSource->srvParams->cfg)->getLayerMetadata(tableName, "projected_extents");

    json a;
    auto c = a.parse(projInfo.c_str());

    for (auto d : c.items()) {
      auto bboxArray = d.value();

      WMSLayer::Projection *projection = new WMSLayer::Projection();
      myWMSLayer->projectionList.push_back(projection);
      projection->name = d.key().c_str();
      bboxArray[0].get_to((projection->dfBBOX[0]));
      bboxArray[1].get_to((projection->dfBBOX[1]));
      bboxArray[2].get_to((projection->dfBBOX[2]));
      bboxArray[3].get_to((projection->dfBBOX[3]));
    }

  } catch (int e) {
    return e;
  }
  return 0;
}