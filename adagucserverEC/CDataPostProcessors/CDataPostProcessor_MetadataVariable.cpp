#include "CDataPostProcessor_MetadataVariable.h"
#include "CRequest.h"
#include "CGenericDataWarper.h"
#include <utils/LayerUtils.h>
#include <CImgRenderFieldVectors.h>

/************************/
/*      CDDPMETADATA_VARIABLE  */
/************************/
const char *CDDPMetadataVariable::className = "CDDPMETADATA_VARIABLE";

const char *CDDPMetadataVariable::getId() { return CDATAPOSTPROCESSOR_CDDPMETADATA_VARIABLE_ID; }

int CDDPMetadataVariable::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm.equals(getId())) {
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDDPMetadataVariable::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if ((isApplicable(proc, dataSource, mode) & mode) == false) {
    return -1;
  }

  auto cdfObject = dataSource->getDataObject(0)->cdfObject;
  // Find variable by its variable name
  if (!proc->attr.variable.empty()) {
    auto varToAdjust = cdfObject->getVariableNE(proc->attr.variable.c_str());
    if (varToAdjust != nullptr) {
      // Adjust units
      if (!proc->attr.units.empty()) {
        varToAdjust->setAttributeText("units", proc->attr.units.c_str());

        auto dataObject = dataSource->getDataObjectByName(proc->attr.variable.c_str());
        if (dataObject != NULL) {
          dataObject->setUnits(proc->attr.units.c_str());
        }
      }
      // Adjust standard_name
      if (!proc->attr.standard_name.empty()) {
        varToAdjust->setAttributeText("standard_name", proc->attr.standard_name.c_str());
      }
      // Adjust long_name
      if (!proc->attr.long_name.empty()) {
        varToAdjust->setAttributeText("long_name", proc->attr.long_name.c_str());
      }
      // Adjust name of the variable itself
      if (!proc->attr.name.empty()) {
        varToAdjust->setAttributeText("orgname", varToAdjust->name.c_str());
        varToAdjust->name = proc->attr.name;
      }
    }
    // variable="direction_component" name="wind_from_direction" units="degrees" standard_name="wind wind_from_direction" long_name="Wind direction"
  }
  return 0;
}
