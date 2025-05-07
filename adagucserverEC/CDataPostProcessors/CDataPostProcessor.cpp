#include "CDataPostProcessor.h"
#include "CRequest.h"
#include "CDataPostProcessor_IncludeLayer.h"
#include "CDataPostProcessor_Beaufort.h"
#include "CDataPostProcessor_ClipMinMax.h"
#include "CDataPostProcessor_Operator.h"
#include "CDataPostProcessor_WFP.h"
#include "CDataPostProcessor_UVComponents.h"
#include "CDataPostProcessor_ToKnots.h"
#include "CDataPostProcessor_WindSpeedKnotsToMs.h"
#include "CDataPostProcessor_AXplusB.h"
#include "CDPPGoes16Metadata.h"
#include "CDataPostProcessors_MSGCPP.h"
#include "CDataPostProcessor_CDPDBZtoRR.h"
#include "CDataPostProcessor_AddFeatures.h"
#include "CDataPostProcessor_SolarTerminator.h"

extern CDPPExecutor cdppExecutorInstance;
CDPPExecutor cdppExecutorInstance;
CDPPExecutor *CDataPostProcessor::getCDPPExecutor() { return &cdppExecutorInstance; }

/*CPDPPExecutor*/
const char *CDPPExecutor::className = "CDPPExecutor";

CDPPExecutor::CDPPExecutor() {
  // CDBDebug("CDPPExecutor");
  dataPostProcessorList = new CT::PointerList<CDPPInterface *>();
  dataPostProcessorList->push_back(new CDPPIncludeLayer());
  dataPostProcessorList->push_back(new CDPPAXplusB());
  dataPostProcessorList->push_back(new CDPPDATAMASK);
  dataPostProcessorList->push_back(new CDPPMSGCPPVisibleMask());
  dataPostProcessorList->push_back(new CDPPMSGCPPHIWCMask());
  dataPostProcessorList->push_back(new CDPPBeaufort());
  dataPostProcessorList->push_back(new CDPPToKnots());
  dataPostProcessorList->push_back(new CDPDBZtoRR());
  dataPostProcessorList->push_back(new CDPPAddFeatures());
  dataPostProcessorList->push_back(new CDPPGoes16Metadata());
  dataPostProcessorList->push_back(new CDPPClipMinMax());
  dataPostProcessorList->push_back(new CDPPOperator());
  dataPostProcessorList->push_back(new CDPPWFP());
  dataPostProcessorList->push_back(new CDDPUVComponents());
  dataPostProcessorList->push_back(new CDPPWindSpeedKnotsToMs());
  dataPostProcessorList->push_back(new CDPPSolarTerminator());
}

CDPPExecutor::~CDPPExecutor() {
  // CDBDebug("~CDPPExecutor");
  delete dataPostProcessorList;
}

const CT::PointerList<CDPPInterface *> *CDPPExecutor::getPossibleProcessors() { return dataPostProcessorList; }

int CDPPExecutor::executeProcessors(CDataSource *dataSource, int mode) {
  for (size_t dpi = 0; dpi < dataSource->cfgLayer->DataPostProc.size(); dpi++) {
    CServerConfig::XMLE_DataPostProc *proc = dataSource->cfgLayer->DataPostProc[dpi];
    for (size_t procId = 0; procId < dataPostProcessorList->size(); procId++) {
      int code = dataPostProcessorList->get(procId)->isApplicable(proc, dataSource, mode);

      if (code == CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET) {
        CDBError("Constraints for DPP %s are not met", dataPostProcessorList->get(procId)->getId());
      }

      /*Will be runned when datasource metadata been loaded */
      if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
        if (code & CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
          try {
            int status = dataPostProcessorList->get(procId)->execute(proc, dataSource, CDATAPOSTPROCESSOR_RUNBEFOREREADING);
            if (status != 0) {
              CDBError("Processor %s failed RUNBEFOREREADING, statuscode %d", dataPostProcessorList->get(procId)->getId(), status);
            }
          } catch (int e) {
            CDBError("Exception in Processor %s failed RUNBEFOREREADING, exceptioncode %d", dataPostProcessorList->get(procId)->getId(), e);
          }
        }
      }
      /*Will be runned when datasource data been loaded */
      if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
        if (code & CDATAPOSTPROCESSOR_RUNAFTERREADING) {
          try {
            int status = dataPostProcessorList->get(procId)->execute(proc, dataSource, CDATAPOSTPROCESSOR_RUNAFTERREADING);
            if (status != 0) {
              CDBError("Processor %s failed RUNAFTERREADING, statuscode %d", dataPostProcessorList->get(procId)->getId(), status);
            }
          } catch (int e) {
            CDBError("Exception in Processor %s failed RUNAFTERREADING, exceptioncode %d", dataPostProcessorList->get(procId)->getId(), e);
          }
        }
      }
    }
  }
  return 0;
}

int CDPPExecutor::executeProcessors(CDataSource *dataSource, int mode, double *data, size_t numItems) {
  // const CT::PointerList<CDPPInterface*> *availableProcs = getPossibleProcessors();
  // CDBDebug("executeProcessors, found %d",dataSource->cfgLayer->DataPostProc.size());
  for (size_t dpi = 0; dpi < dataSource->cfgLayer->DataPostProc.size(); dpi++) {
    CServerConfig::XMLE_DataPostProc *proc = dataSource->cfgLayer->DataPostProc[dpi];
    for (size_t procId = 0; procId < dataPostProcessorList->size(); procId++) {
      int code = dataPostProcessorList->get(procId)->isApplicable(proc, dataSource, mode);

      if (code == CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET) {
        CDBError("Constraints for DPP %s are not met", dataPostProcessorList->get(procId)->getId());
      }

      /*Will be runned when datasource metadata been loaded */
      if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
        if (code & CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
          try {
            int status = dataPostProcessorList->get(procId)->execute(proc, dataSource, CDATAPOSTPROCESSOR_RUNBEFOREREADING, NULL, 0);
            if (status != 0) {
              CDBError("Processor %s failed RUNBEFOREREADING, statuscode %d", dataPostProcessorList->get(procId)->getId(), status);
            }
          } catch (int e) {
            CDBError("Exception in Processor %s failed RUNBEFOREREADING, exceptioncode %d", dataPostProcessorList->get(procId)->getId(), e);
          }
        }
      }

      /*Will be runned when datasource data been loaded */
      if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
        if (code & CDATAPOSTPROCESSOR_RUNAFTERREADING) {
          try {
            int status = dataPostProcessorList->get(procId)->execute(proc, dataSource, CDATAPOSTPROCESSOR_RUNAFTERREADING, data, numItems);
            if (status != 0) {
              CDBError("Processor %s failed RUNAFTERREADING, statuscode %d", dataPostProcessorList->get(procId)->getId(), status);
            }
          } catch (int e) {
            CDBError("Exception in Processor %s failed RUNAFTERREADING, exceptioncode %d", dataPostProcessorList->get(procId)->getId(), e);
          }
        }
      }
    }
  }
  return 0;
}
