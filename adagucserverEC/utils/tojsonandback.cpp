#include "./tojsonandback.h"

#include <json_adaguc.h>

std::string convertProjectionListToJsonString(std::vector<WMSLayer::Projection *> projectionList) {
  json projsettings;
  for (auto projection : projectionList) {
    json item = {projection->dfBBOX[0], projection->dfBBOX[1], projection->dfBBOX[2], projection->dfBBOX[3]};
    projsettings[projection->name.c_str()] = item;
  }
  std::string projSettingsAsJsonString = projsettings.dump();
  return projSettingsAsJsonString;
}