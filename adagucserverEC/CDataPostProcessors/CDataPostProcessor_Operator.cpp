#include "CDataPostProcessor_Operator.h"

/************************/
/*      CDPPOperator  */
/************************/
const char *CDPPOperator::className = "CDPPOperator";

const char *CDPPOperator::getId() { return "operator"; }

int CDPPOperator::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("operator")) {
    if (dataSource->getNumDataObjects() < 2 && mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
      CDBError("2 variables are needed for operator, found %d", dataSource->getNumDataObjects());
      return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
    }
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPOperator::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if ((isApplicable(proc, dataSource, mode) & mode) == false) {
    return -1;
  }
  CDBDebug("Applying Operator");
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    CT::string newDataObjectName = proc->attr.name;
    if (newDataObjectName.empty()) {
      newDataObjectName = "result";
    }
    if (dataSource->getDataObject(0)->cdfVariable->name.equals(newDataObjectName.c_str())) return 0;
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
      newDataObject->cdfVariable->setAttributeText("units", proc->attr.units);
    }

    short attrData[3];
    attrData[0] = -1;
    newDataObject->cdfVariable->setAttribute("_FillValue", newDataObject->cdfVariable->getType(), attrData, 1);

    newDataObject->cdfVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    CDBDebug("CDATAPOSTPROCESSOR_RUNAFTERREADING::Applying OPERATOR");
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    CDF::allocateData(dataSource->getDataObject(0)->cdfVariable->getType(), &dataSource->getDataObject(0)->cdfVariable->data, l);

    float *result = (float *)dataSource->getDataObject(0)->cdfVariable->data;

    CDataSource::DataObject *dataObjectA, *dataObjectB;
    try {
      dataObjectA = dataSource->getDataObject(proc->attr.a);
      dataObjectB = dataSource->getDataObject(proc->attr.b);
    } catch (int e) {
      return 1;
    }

    float *a = (float *)dataObjectA->cdfVariable->data;
    float *b = (float *)dataObjectB->cdfVariable->data;

    if (proc->attr.mode.equals("substract")) {
      for (size_t j = 0; j < l; j++) {
        result[j] = a[j] - b[j];
      }
    }

    if (proc->attr.mode.equals("-")) {
      for (size_t j = 0; j < l; j++) {
        result[j] = a[j] - b[j];
      }
    }
    if (proc->attr.mode.equals("+")) {
      for (size_t j = 0; j < l; j++) {
        result[j] = a[j] + b[j];
      }
    }
    if (proc->attr.mode.equals("*")) {
      for (size_t j = 0; j < l; j++) {
        result[j] = b[j] * a[j];
      }
    }
    if (proc->attr.mode.equals("/")) {
      for (size_t j = 0; j < l; j++) {
        if (a[j] == 0) {
          result[j] = NAN;
        } else {
          result[j] = a[j] / b[j];
        }
      }
    }
  }
  // dataSource->eraseDataObject(1);
  return 0;
}