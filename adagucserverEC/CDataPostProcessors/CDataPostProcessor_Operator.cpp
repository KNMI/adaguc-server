#include "CDataPostProcessor_Operator.h"

/************************/
/*      CDPPOperator  */
/************************/

const char *CDPPOperator::getId() { return "operator"; }

int CDPPOperator::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("operator")) {
    if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
      if (dataSource->getNumDataObjects() < 2 && mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
        CDBError("2 variables are needed for operator, found %d", dataSource->getNumDataObjects());
        return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
      }
      return CDATAPOSTPROCESSOR_RUNBEFOREREADING;
    }
    return CDATAPOSTPROCESSOR_RUNAFTERREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPOperator::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
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
      newDataObject->cdfVariable->setAttributeText("units", proc->attr.units.c_str());
    }

    double attrData[1];
    attrData[0] = 0;
    newDataObject->cdfVariable->setAttribute("_FillValue", newDataObject->cdfVariable->getType(), attrData, 1);

    newDataObject->cdfVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    CDBDebug("CDATAPOSTPROCESSOR_RUNAFTERREADING::Applying OPERATOR");
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    dataSource->getDataObject(0)->cdfVariable->data = NULL;
    CDF::allocateData(CDF_FLOAT, &dataSource->getDataObject(0)->cdfVariable->data, l);

    float *result = (float *)dataSource->getDataObject(0)->cdfVariable->data;

    CDataSource::DataObject *dataObjectA, *dataObjectB;

    dataObjectA = dataSource->getDataObjectByName(proc->attr.a);
    if (dataObjectA == nullptr) {
      CDBError("Variable %s not found", proc->attr.a.c_str());
      return 1;
    }

    dataObjectB = dataSource->getDataObjectByName(proc->attr.b);
    if (dataObjectB == nullptr) {
      CDBError("Variable %s not found", proc->attr.b.c_str());
      return 1;
    }

    void *dataA = dataObjectA->cdfVariable->data;
    void *dataB = dataObjectB->cdfVariable->data;
    CDFType typeA = dataObjectA->cdfVariable->getType();
    CDFType typeB = dataObjectB->cdfVariable->getType();

    if (proc->attr.mode.equals("-")) {
      for (size_t j = 0; j < l; j++) {
        float a = getElement(dataA, typeA, j);
        float b = getElement(dataB, typeB, j);
        result[j] = a - b;
      }
    }
    if (proc->attr.mode.equals("+")) {
      for (size_t j = 0; j < l; j++) {
        float a = getElement(dataA, typeA, j);
        float b = getElement(dataB, typeB, j);
        result[j] = a + b;
      }
    }
    if (proc->attr.mode.equals("*")) {
      for (size_t j = 0; j < l; j++) {
        float a = getElement(dataA, typeA, j);
        float b = getElement(dataB, typeB, j);
        result[j] = b * a;
      }
    }
    if (proc->attr.mode.equals("/")) {
      for (size_t j = 0; j < l; j++) {
        float a = getElement(dataA, typeA, j);
        float b = getElement(dataB, typeB, j);
        if (b == 0) {
          result[j] = NAN;
        } else {
          result[j] = a / b;
        }
      }
    }
  }

  return 0;
}

float CDPPOperator::getElement(void *data, CDFType dataType, size_t index) {
  switch (dataType) {
  case CDF_CHAR:
    return ((const char *)data)[index];
    break;
  case CDF_BYTE:
    return ((const char *)data)[index];
    break;
  case CDF_UBYTE:
    return ((const unsigned char *)data)[index];
    break;
  case CDF_SHORT:
    return ((const short *)data)[index];
    break;
  case CDF_USHORT:
    return ((const ushort *)data)[index];
    break;
  case CDF_INT:
    return ((const int *)data)[index];
    break;
  case CDF_UINT:
    return ((const uint *)data)[index];
    break;
  case CDF_INT64:
    return ((const int64_t *)data)[index];
    break;
  case CDF_UINT64:
    return ((const uint64_t *)data)[index];
    break;
  case CDF_FLOAT:
    return ((const float *)data)[index];
    break;
  case CDF_DOUBLE:
    return ((const double *)data)[index];
    break;
  }
  return 0;
}