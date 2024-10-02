#include "LayerUtils.h"

int makeUniqueLayerName(CT::string *layerName, CServerConfig::XMLE_Layer *cfgLayer) {

  layerName->copy("");
  if (cfgLayer->Group.size() == 1) {
    if (cfgLayer->Group[0]->attr.value.empty() == false) {
      layerName->copy(cfgLayer->Group[0]->attr.value.c_str());
      layerName->concat("/");
    }
  }
  if (cfgLayer->Name.size() == 0) {
    CServerConfig::XMLE_Name *name = new CServerConfig::XMLE_Name();
    cfgLayer->Name.push_back(name);
    if (cfgLayer->Variable.size() == 0) {
      name->value.copy("undefined_variable");
    } else {
      name->value.copy(cfgLayer->Variable[0]->value.c_str());
    }
  }

  if (!cfgLayer->Name[0]->attr.force.equals("true")) {
    layerName->concat(cfgLayer->Name[0]->value.c_str());
    layerName->replaceSelf(" ", "_");
  } else {
    layerName->copy(cfgLayer->Name[0]->value.c_str());
  }

  return 0;
}
