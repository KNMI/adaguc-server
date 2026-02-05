#include "CDPPGoes16Metadata.h"
#include "CTime.h"

/************************/
/*      CDPPFixGOES16     */
/************************/

const char *CDPPGoes16Metadata::getId() {
  CDBDebug("getId");
  return "goes16metadata";
}
int CDPPGoes16Metadata::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int mode) {
  // CDBDebug("isApplicable");
  if (proc->attr.algorithm.equals("goes16metadata") && mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    return CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPGoes16Metadata::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  CDBDebug("execute");
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {

    CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
    CDF::Variable *productT = cdfObject->getVariableNE("time_bounds");

    if (!productT) {
      CDBError("Could not get PRODUCT/time variable");
      return 1;
    }

    if (cdfObject->getVariableNE("time") != NULL) {
      CDBError("time var already defined");
      return 1;
    }
    CDF::Dimension *dimT = new CDF::Dimension();
    dimT->setSize(1);
    cdfObject->addDimension(dimT);
    CDF::Variable *varT = new CDF::Variable();
    varT->setType(CDF_DOUBLE);
    varT->name.copy("time");
    varT->isDimension = true;
    varT->dimensionlinks.push_back(dimT);
    cdfObject->addVariable(varT);
    CDF::allocateData(CDF_DOUBLE, &varT->data, dimT->length);
    varT->setAttributeText("standard_name", "time");
    try {
      varT->setAttributeText("units", productT->getAttribute("time_coverage_start")->toString().c_str());
      CTime *myTime = CTime::GetCTimeInstance(productT);

      if (myTime == nullptr) {
        CDBDebug(CTIME_GETINSTANCE_ERROR_MESSAGE);
        return 1;
      }

      CTime::Date date = myTime->freeDateStringToDate(cdfObject->getAttribute("time_coverage_start")->toString().c_str());
      ((double *)varT->data)[0] = myTime->dateToOffset(date);
    } catch (int) {
      CDBError("Could not get units for time_coverage_start");
      return 1;
    }
  }
  return 0;
}
