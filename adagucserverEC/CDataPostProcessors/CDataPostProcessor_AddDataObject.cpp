#include "CDataPostProcessor_AddDataObject.h"

const char *CDPPAddDataObject::getId() { return CDATAPOSTPROCESSOR_AddDataObject_ID; }

int CDPPAddDataObject::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_AddDataObject_ID)) {
    if (proc->attr.name.empty() || proc->attr.a.empty()) {
      CDBError("Fill in attributes name (got: %s) and a (got: %s)", proc->attr.name.c_str(), proc->attr.a.c_str());
      return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
    }
    return CDATAPOSTPROCESSOR_RUNBEFOREREADING | CDATAPOSTPROCESSOR_RUNAFTERREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPAddDataObject::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *, size_t) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  CDBError("Not implemented yet");
  return 1;
}

int CDPPAddDataObject::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }

  CDBDebug("Applying %s", CDATAPOSTPROCESSOR_AddDataObject_ID);
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    CT::string newDataObjectName = proc->attr.name;

    CDF::Variable *varToClone = dataSource->getDataObject(0)->cdfVariable;
    CDataSource::DataObject *newDataObject = new CDataSource::DataObject();

    newDataObject->variableName.copy(newDataObjectName.c_str());
    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), newDataObject);

    newDataObject->cdfVariable = new CDF::Variable();
    newDataObject->cdfObject = (CDFObject *)varToClone->getParentCDFObject();
    newDataObject->cdfObject->addVariable(newDataObject->cdfVariable);
    newDataObject->cdfVariable->setName(newDataObjectName.c_str());
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
    newDataObject->cdfVariable->setAttributeText("standard_name", newDataObjectName.c_str());
    newDataObject->cdfVariable->setAttributeText("long_name", newDataObjectName.c_str());
    newDataObject->cdfVariable->setAttributeText("units", "1");
    if (!proc->attr.units.empty()) {
      newDataObject->cdfVariable->setAttributeText("units", proc->attr.units.c_str());
    }

    double attrData[1];
    attrData[0] = 0;
    newDataObject->cdfVariable->setAttribute("_FillValue", newDataObject->cdfVariable->getType(), attrData, 1);
    newDataObject->cdfVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    // Store the value of a="..." into the new data object

    float value = proc->attr.a.toDouble();
    size_t size = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    dataSource->getDataObject(0)->cdfVariable->data = NULL;
    CDF::allocateData(CDF_FLOAT, &dataSource->getDataObject(0)->cdfVariable->data, size);
    float *result = (float *)dataSource->getDataObject(0)->cdfVariable->data;

    for (size_t i = 0; i < size; i++) {
      result[i] = value;
    }
  }

  return 0;
}