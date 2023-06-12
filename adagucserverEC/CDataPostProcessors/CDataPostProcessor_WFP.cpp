#include "CDataPostProcessor_WFP.h"

/************************/
/*      CDPPWFP  */
/************************/
const char *CDPPWFP::className = "CDPPWFP";

const char *CDPPWFP::getId() { return "WFP"; }
int CDPPWFP::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource) {
  if (proc->attr.algorithm.equals("WFP")) {
    if (dataSource->getNumDataObjects() != 2 && dataSource->getNumDataObjects() != 3) {
      CDBError("2 variables are needed for WFP, found %d", dataSource->getNumDataObjects());
      return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
    }
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPWFP::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if ((isApplicable(proc, dataSource) & mode) == false) {
    return -1;
  }
  CDBDebug("Applying WFP");
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    if (dataSource->getDataObject(0)->cdfVariable->name.equals("result")) return 0;
    CDF::Variable *varToClone = dataSource->getDataObject(0)->cdfVariable;

    CDataSource::DataObject *newDataObject = new CDataSource::DataObject();

    newDataObject->variableName.copy("result");

    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), newDataObject);

    newDataObject->cdfVariable = new CDF::Variable();
    newDataObject->cdfObject = (CDFObject *)varToClone->getParentCDFObject();
    newDataObject->cdfObject->addVariable(newDataObject->cdfVariable);
    newDataObject->cdfVariable->setName("result");
    newDataObject->cdfVariable->setType(CDF_FLOAT);
    newDataObject->cdfVariable->setSize(dataSource->dWidth * dataSource->dHeight);

    for (size_t j = 0; j < varToClone->dimensionlinks.size(); j++) {
      newDataObject->cdfVariable->dimensionlinks.push_back(varToClone->dimensionlinks[j]);
    }

    for (size_t j = 0; j < varToClone->attributes.size(); j++) {
      newDataObject->cdfVariable->attributes.push_back(new CDF::Attribute(varToClone->attributes[j]));
    }

    newDataObject->cdfVariable->removeAttribute("scale_factor");
    newDataObject->cdfVariable->removeAttribute("add_offset");
    newDataObject->cdfVariable->setAttributeText("standard_name", "result");
    newDataObject->cdfVariable->setAttributeText("long_name", "result");
    newDataObject->cdfVariable->setAttributeText("units", "1");

    short attrData[3];
    attrData[0] = -1;
    newDataObject->cdfVariable->setAttribute("_FillValue", newDataObject->cdfVariable->getType(), attrData, 1);

    newDataObject->cdfVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    CDBDebug("CDATAPOSTPROCESSOR_RUNAFTERREADING::Applying WFP");
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    CDF::allocateData(dataSource->getDataObject(0)->cdfVariable->getType(), &dataSource->getDataObject(0)->cdfVariable->data, l);

    float *hiwc = (float *)dataSource->getDataObject(0)->cdfVariable->data;
    float *a = (float *)dataSource->getDataObject(1)->cdfVariable->data;
    float *b = (float *)dataSource->getDataObject(2)->cdfVariable->data;

    for (size_t j = 0; j < l; j++) {
      hiwc[j] = b[j] - a[j];
    }
  }
  // dataSource->eraseDataObject(1);
  return 0;
}