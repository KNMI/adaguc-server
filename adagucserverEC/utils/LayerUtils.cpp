#include "LayerUtils.h"

CT::string makeUniqueLayerName(CServerConfig::XMLE_Layer *cfgLayer, const char *optionalLayerName) {

  CT::string layerName;
  if (optionalLayerName == nullptr) {
    // If no Name is configured, use the configured Variable name instead.
    if (cfgLayer->Name.size() == 0) {
      CServerConfig::XMLE_Name *name = new CServerConfig::XMLE_Name();
      cfgLayer->Name.push_back(name);
      if (cfgLayer->Variable.size() == 0) {
        name->value.copy("undefined_variable");
      } else {
        name->value.copy(cfgLayer->Variable[0]->value.c_str());
      }
    }
    // The groupname should be prefixed to the real layername
    if (cfgLayer->Group.size() == 1 && !cfgLayer->Name[0]->attr.force.equals("true") && cfgLayer->Group[0]->attr.value.empty() == false) {
      layerName.print("%s/%s", cfgLayer->Group[0]->attr.value.c_str(), cfgLayer->Name[0]->value.c_str());
    } else {
      layerName.copy(cfgLayer->Name[0]->value.c_str());
    }
    // Spaces are not allowed
    layerName.replaceSelf(" ", "_");
  } else {
    layerName = optionalLayerName;
  }

  // A layername has to start with a letter (not numeric value);
  if (isalpha(layerName.charAt(0)) == 0) {
    CT::string layerNameWithPrefix = "ID_";
    layerNameWithPrefix.concat(layerName);
    return layerNameWithPrefix;
  }

  return layerName;
}
