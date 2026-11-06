#include "CDataPostProcessor_AddDataObject.h"
#include "CCDFObject.h"
#include "CDebugger.h"
#include <string>

#define ADDDATAOBJECT_ATTR ADAGUCPOSTPROC_ATTR_PREFIX "_ADD_DATAOBJECT"

// The data object added by this processor, marked with its index in the list of processors
static DataObject *findAddedDataObject(CDataSource *dataSource, CServerConfig::XMLE_DataPostProc *proc) {
  std::string mark = std::to_string(proc->attr.postProcIndexInLayer);
  for (auto &dataObject: dataSource->dataObjects) {
    auto attr = dataObject.cdfVariable != nullptr ? dataObject.cdfVariable->getAttributeNE(ADDDATAOBJECT_ATTR) : nullptr;
    if (attr != nullptr && attr->toString() == mark) return &dataObject;
  }
  return nullptr;
}

const char *CDPPAddDataObject::getId() { return CDATAPOSTPROCESSOR_AddDataObject_ID; }

int CDPPAddDataObject::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm == (CDATAPOSTPROCESSOR_AddDataObject_ID)) {
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
    // Runs again when the data is read a second time, for example for GetFeatureInfo. A second object would never be filled.
    if (findAddedDataObject(dataSource, proc) != nullptr) return 0;
    std::string newDataObjectName = proc->attr.name;

    CDF::Variable *varToClone = dataSource->getDataObject(0)->cdfVariable;

    dataSource->dataObjects.insert(dataSource->dataObjects.begin(), DataObject());
    auto &newDataObject = dataSource->dataObjects.front();
    newDataObject.cdfVariable = new CDF::Variable();
    newDataObject.cdfObject = (CDFObject *)varToClone->getParentCDFObject();
    newDataObject.cdfObject->addVariable(newDataObject.cdfVariable);
    newDataObject.cdfVariable->setName(newDataObjectName.c_str());
    newDataObject.cdfVariable->setType(CDF_FLOAT);
    newDataObject.cdfVariable->setSize(dataSource->dWidth * dataSource->dHeight);

    for (size_t j = 0; j < varToClone->dimensionlinks.size(); j++) {
      newDataObject.cdfVariable->dimensionlinks.push_back(varToClone->dimensionlinks[j]);
    }

    for (size_t j = 0; j < varToClone->attributes.size(); j++) {
      newDataObject.cdfVariable->attributes.push_back(new CDF::Attribute(varToClone->attributes[j]));
    }

    newDataObject.cdfVariable->removeAttribute("scale_factor");
    newDataObject.cdfVariable->removeAttribute("add_offset");
    newDataObject.cdfVariable->setAttributeText("standard_name", newDataObjectName.c_str());
    newDataObject.cdfVariable->setAttributeText("long_name", newDataObjectName.c_str());
    newDataObject.cdfVariable->setAttributeText("units", "1");
    if (!proc->attr.units.empty()) {
      newDataObject.cdfVariable->setAttributeText("units", proc->attr.units.c_str());
    }

    double attrData[1];
    attrData[0] = 0;
    newDataObject.cdfVariable->setAttribute("_FillValue", newDataObject.cdfVariable->getType(), attrData, 1);
    newDataObject.cdfVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);
    // Marks the data object of this processor. Overwrites the mark copied from an object added by an earlier add_dataobject.
    newDataObject.cdfVariable->setAttributeText(ADDDATAOBJECT_ATTR, std::to_string(proc->attr.postProcIndexInLayer).c_str());
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    // Store the value of a="..." into the data object of this processor. Not simply object 0: a later add_dataobject
    // inserts its object before it, which would leave this one unfilled.
    DataObject *dataObject = findAddedDataObject(dataSource, proc);
    if (dataObject == nullptr) {
      CDBError("Data object of %s [%s] not found", CDATAPOSTPROCESSOR_AddDataObject_ID, proc->attr.name.c_str());
      return 1;
    }

    float value = atof(proc->attr.a.c_str());
    size_t size = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    CDF::allocateData(CDF_FLOAT, &dataObject->cdfVariable->data, size);
    float *result = (float *)dataObject->cdfVariable->data;

    for (size_t i = 0; i < size; i++) {
      result[i] = value;
    }
  }

  return 0;
}