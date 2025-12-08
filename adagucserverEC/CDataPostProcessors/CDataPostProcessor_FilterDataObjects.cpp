#include "CDataPostProcessor_FilterDataObjects.h"
#include "CRequest.h"
#include "CGenericDataWarper.h"
#include <utils/LayerUtils.h>
#include <CImgRenderFieldVectors.h>

/************************/
/*      CDDPFILTERDATAOBJECTS  */
/************************/
const char *CDDPFilterDataObjects::className = "CDDPFILTERDATAOBJECTS";

const char *CDDPFilterDataObjects::getId() { return CDATAPOSTPROCESSOR_CDDPFILTERDATAOBJECTS_ID; }

int CDDPFilterDataObjects::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm.equals(getId())) {
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDDPFilterDataObjects::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if ((isApplicable(proc, dataSource, mode) & mode) == false) {
    return -1;
  }

  for (auto d : *dataSource->getDataObjectsVector()) {
    d->filterFromOutput = true;
  }
  auto itemsToFilter = proc->attr.select.split(",");
  for (auto item : itemsToFilter) {
    auto d = dataSource->getDataObjectByName(item.trim().c_str());
    if (d != nullptr) {
      d->filterFromOutput = false;
      if (d->cdfVariable != nullptr) {
        d->cdfVariable->setAttributeText("selected_for_output", "true");
      }
    }
  }

  return 0;
}
