#include "LayerUtils.h"

CT::string makeUniqueLayerName(CServerConfig::XMLE_Layer *cfgLayer, const char *optionalLayerName) {

  CT::string layerName;
  if (optionalLayerName == nullptr) {
    // If no Name is configured, use the configured Variable name instead.
    if (cfgLayer->Name.size() == 0) {
      CServerConfig::XMLE_Name *name = new CServerConfig::XMLE_Name();
      cfgLayer->Name.push_back(name);
      if (cfgLayer->Variable.size() == 0) {
        name->elementValue = ("undefined_variable");
      } else {
        name->elementValue = (cfgLayer->Variable[0]->elementValue.c_str());
      }
    }
    // The groupname should be suffixed to the real layername
    if (cfgLayer->Group.size() == 1 && cfgLayer->Name[0]->attr.force != ("true") && cfgLayer->Group[0]->attr.value.empty() == false) {
      layerName.print("%s/%s", cfgLayer->Group[0]->attr.value.c_str(), cfgLayer->Name[0]->elementValue.c_str());
    } else {
      layerName = (cfgLayer->Name[0]->elementValue.c_str());
    }
    // Spaces are not allowed
    layerName.replaceSelf(" ", "_");
  } else {
    layerName = optionalLayerName;
  }

  return layerName;
}
