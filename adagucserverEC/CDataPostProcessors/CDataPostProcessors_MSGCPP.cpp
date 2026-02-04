#include "CDataPostProcessors_MSGCPP.h"

/************************/
/*CDPPMSGCPPVisibleMask */
/************************/

const char *CDPPMSGCPPVisibleMask::getId() { return "MSGCPP_VISIBLEMASK"; }
int CDPPMSGCPPVisibleMask::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("msgcppvisiblemask")) {
    if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
      if (dataSource->getNumDataObjects() != 2 && dataSource->getNumDataObjects() != 3) {
        CDBError("2 or 3 variables are needed for msgcppvisiblemask, found %lu", dataSource->getNumDataObjects());
        return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
      }
      return CDATAPOSTPROCESSOR_RUNBEFOREREADING;
    }
    return CDATAPOSTPROCESSOR_RUNAFTERREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPMSGCPPVisibleMask::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {

    if (dataSource->getDataObject(0)->cdfVariable->name.equals("mask")) return 0;
    CDBDebug("CDATAPOSTPROCESSOR_RUNBEFOREREADING::Applying msgcpp VISIBLE mask");
    CDF::Variable *varToClone = dataSource->getDataObject(0)->cdfVariable;

    CDataSource::DataObject *newDataObject = new CDataSource::DataObject();

    newDataObject->variableName.copy("mask");

    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), newDataObject);

    newDataObject->cdfVariable = new CDF::Variable();
    newDataObject->cdfObject = (CDFObject *)varToClone->getParentCDFObject();
    newDataObject->cdfObject->addVariable(newDataObject->cdfVariable);
    newDataObject->cdfVariable->setName("mask");
    newDataObject->cdfVariable->setType(CDF_SHORT);
    newDataObject->cdfVariable->setSize(dataSource->dWidth * dataSource->dHeight);

    for (size_t j = 0; j < varToClone->dimensionlinks.size(); j++) {
      newDataObject->cdfVariable->dimensionlinks.push_back(varToClone->dimensionlinks[j]);
    }

    for (size_t j = 0; j < varToClone->attributes.size(); j++) {
      newDataObject->cdfVariable->attributes.push_back(new CDF::Attribute(varToClone->attributes[j]));
    }

    newDataObject->cdfVariable->removeAttribute("scale_factor");
    newDataObject->cdfVariable->removeAttribute("add_offset");
    newDataObject->cdfVariable->setAttributeText("standard_name", "visible_mask status_flag");
    newDataObject->cdfVariable->setAttributeText("long_name", "Visible mask");
    newDataObject->cdfVariable->setAttributeText("units", "1");
    newDataObject->cdfVariable->removeAttribute("valid_range");
    newDataObject->cdfVariable->removeAttribute("flag_values");
    newDataObject->cdfVariable->removeAttribute("flag_meanings");
    short attrData[3];
    attrData[0] = -1;
    newDataObject->cdfVariable->setAttribute("_FillValue", CDF_SHORT, attrData, 1);

    attrData[0] = 0;
    attrData[1] = 1;
    newDataObject->cdfVariable->setAttribute("valid_range", CDF_SHORT, attrData, 2);
    newDataObject->cdfVariable->setAttribute("flag_values", CDF_SHORT, attrData, 2);
    newDataObject->cdfVariable->setAttributeText("flag_meanings", "no yes");

    newDataObject->cdfVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);

    // return 0;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    CDBDebug("Applying msgcppvisiblemask");
    short *mask = (short *)dataSource->getDataObject(0)->cdfVariable->data;
    float *sunz = (float *)dataSource->getDataObject(1)->cdfVariable->data; // sunz
    float *satz = (float *)dataSource->getDataObject(2)->cdfVariable->data;
    short fNosunz = (short)dataSource->getDataObject(0)->dfNodataValue;
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    float fa = 72, fb = 75;
    if (proc->attr.b.empty() == false) {
      CT::string b;
      b.copy(proc->attr.b.c_str());
      fb = b.toDouble();
    }
    if (proc->attr.a.empty() == false) {
      CT::string a;
      a.copy(proc->attr.a.c_str());
      fa = a.toDouble();
    }
    for (size_t j = 0; j < l; j++) {
      if ((satz[j] < fa && sunz[j] < fa) || (satz[j] > fb))
        mask[j] = fNosunz;
      else
        mask[j] = 1;
    }
  }
  return 0;
}

/************************/
/*CDPPMSGCPPHIWCMask */
/************************/

const char *CDPPMSGCPPHIWCMask::getId() { return "MSGCPP_HIWCMASK"; }

int CDPPMSGCPPHIWCMask::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("msgcpphiwcmask")) {
    if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
      if (dataSource->getNumDataObjects() != 4 && dataSource->getNumDataObjects() != 5) {
        CDBError("4 or 5 variables are needed for msgcpphiwcmask, found %lu", dataSource->getNumDataObjects());
        return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
      }
      return CDATAPOSTPROCESSOR_RUNBEFOREREADING;
    }
    return CDATAPOSTPROCESSOR_RUNAFTERREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPMSGCPPHIWCMask::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    if (dataSource->getDataObject(0)->cdfVariable->name.equals("hiwc")) return 0;
    CDBDebug("CDATAPOSTPROCESSOR_RUNBEFOREREADING::Applying msgcpp HIWC mask");
    CDF::Variable *varToClone = dataSource->getDataObject(0)->cdfVariable;

    CDataSource::DataObject *newDataObject = new CDataSource::DataObject();

    newDataObject->variableName.copy("hiwc");

    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), newDataObject);

    newDataObject->cdfVariable = new CDF::Variable();
    newDataObject->cdfObject = (CDFObject *)varToClone->getParentCDFObject();
    newDataObject->cdfObject->addVariable(newDataObject->cdfVariable);
    newDataObject->cdfVariable->setName("hiwc");
    newDataObject->cdfVariable->setType(CDF_SHORT);
    newDataObject->cdfVariable->setSize(dataSource->dWidth * dataSource->dHeight);

    for (size_t j = 0; j < varToClone->dimensionlinks.size(); j++) {
      newDataObject->cdfVariable->dimensionlinks.push_back(varToClone->dimensionlinks[j]);
    }

    for (size_t j = 0; j < varToClone->attributes.size(); j++) {
      newDataObject->cdfVariable->attributes.push_back(new CDF::Attribute(varToClone->attributes[j]));
    }

    newDataObject->cdfVariable->removeAttribute("scale_factor");
    newDataObject->cdfVariable->removeAttribute("add_offset");
    newDataObject->cdfVariable->setAttributeText("standard_name", "high_ice_water_content status_flag");
    newDataObject->cdfVariable->setAttributeText("long_name", "High ice water content");
    newDataObject->cdfVariable->setAttributeText("units", "1");
    newDataObject->cdfVariable->removeAttribute("valid_range");
    newDataObject->cdfVariable->removeAttribute("flag_values");
    newDataObject->cdfVariable->removeAttribute("flag_meanings");
    short attrData[3];
    attrData[0] = -1;
    newDataObject->cdfVariable->setAttribute("_FillValue", CDF_SHORT, attrData, 1);

    attrData[0] = 0;
    attrData[1] = 1;
    newDataObject->cdfVariable->setAttribute("valid_range", CDF_SHORT, attrData, 2);
    newDataObject->cdfVariable->setAttribute("flag_values", CDF_SHORT, attrData, 2);
    newDataObject->cdfVariable->setAttributeText("flag_meanings", "no yes");

    newDataObject->cdfVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);

    // return 0;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    CDBDebug("CDATAPOSTPROCESSOR_RUNAFTERREADING::Applying msgcpp HIWC mask");
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    // CDF::allocateData(dataSource->getDataObject(0)->cdfVariable->getType(),&dataSource->getDataObject(0)->cdfVariable->data,l);

    short *hiwc = (short *)dataSource->getDataObject(0)->cdfVariable->data;
    float *cph = (float *)dataSource->getDataObject(1)->cdfVariable->data;
    float *cwp = (float *)dataSource->getDataObject(2)->cdfVariable->data;
    float *ctt = (float *)dataSource->getDataObject(3)->cdfVariable->data;
    float *cot = (float *)dataSource->getDataObject(4)->cdfVariable->data;

    for (size_t j = 0; j < l; j++) {
      hiwc[j] = 0;
      if (cph[j] == 2) {
        if (cwp[j] > 0.1) {
          if (ctt[j] < 270) {
            if (cot[j] > 20) {
              hiwc[j] = 1;
            }
          }
        }
      }
    }
  }

  // dataSource->eraseDataObject(1);
  return 0;
}

/************************/
/*CDPPDATAMASK */
/************************/

const char *CDPPDATAMASK::getId() { return "datamask"; }
int CDPPDATAMASK::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("datamask")) {
    if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
      if (dataSource->getNumDataObjects() != 2 && dataSource->getNumDataObjects() != 3) {
        CDBError("2 or 3 variables are needed for datamask, found %lu", dataSource->getNumDataObjects());
        return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
      }
      return CDATAPOSTPROCESSOR_RUNBEFOREREADING;
    }
    return CDATAPOSTPROCESSOR_RUNAFTERREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

template <typename TT, typename SS>
void CDPPDATAMASK::DOIT<TT, SS>::doIt(void *newData, void *orginalData, void *maskData, double newDataNoDataValue, CDFType maskType, double a, double b, double c, int mode, size_t l) {
  switch (maskType) {
  case CDF_CHAR:
    DOIT<TT, char>::reallyDoIt(newData, orginalData, maskData, newDataNoDataValue, maskType, a, b, c, mode, l);
    break;
  case CDF_BYTE:
    DOIT<TT, char>::reallyDoIt(newData, orginalData, maskData, newDataNoDataValue, maskType, a, b, c, mode, l);
    break;
  case CDF_UBYTE:
    DOIT<TT, unsigned char>::reallyDoIt(newData, orginalData, maskData, newDataNoDataValue, maskType, a, b, c, mode, l);
    break;
  case CDF_SHORT:
    DOIT<TT, short>::reallyDoIt(newData, orginalData, maskData, newDataNoDataValue, maskType, a, b, c, mode, l);
    break;
  case CDF_USHORT:
    DOIT<TT, ushort>::reallyDoIt(newData, orginalData, maskData, newDataNoDataValue, maskType, a, b, c, mode, l);
    break;
  case CDF_INT:
    DOIT<TT, int>::reallyDoIt(newData, orginalData, maskData, newDataNoDataValue, maskType, a, b, c, mode, l);
    break;
  case CDF_UINT:
    DOIT<TT, uint>::reallyDoIt(newData, orginalData, maskData, newDataNoDataValue, maskType, a, b, c, mode, l);
    break;
  case CDF_FLOAT:
    DOIT<TT, float>::reallyDoIt(newData, orginalData, maskData, newDataNoDataValue, maskType, a, b, c, mode, l);
    break;
  case CDF_DOUBLE:
    DOIT<TT, double>::reallyDoIt(newData, orginalData, maskData, newDataNoDataValue, maskType, a, b, c, mode, l);
    break;
  }
}

template <typename TT, typename SS>
void CDPPDATAMASK::DOIT<TT, SS>::reallyDoIt(void *newData, void *orginalData, void *maskData, double newDataNoDataValue, CDFType, double a, double b, double c, int mode, size_t l) {
  // if_mask_includes_then_nodata_else_data
  if (mode == 0) {
    for (size_t j = 0; j < l; j++) {
      if (((SS *)maskData)[j] >= a && ((SS *)maskData)[j] <= b)
        ((TT *)newData)[j] = newDataNoDataValue;
      else
        ((TT *)newData)[j] = ((TT *)orginalData)[j];
    }
  }

  // if_mask_excludes_then_nodata_else_data
  if (mode == 1) {
    for (size_t j = 0; j < l; j++) {
      if (((SS *)maskData)[j] >= a && ((SS *)maskData)[j] <= b)
        ((TT *)newData)[j] = ((TT *)orginalData)[j];
      else
        ((TT *)newData)[j] = newDataNoDataValue;
    }
  }

  // if_mask_includes_then_valuec_else_data
  if (mode == 2) {
    for (size_t j = 0; j < l; j++) {
      if (((SS *)maskData)[j] >= a && ((SS *)maskData)[j] <= b)
        ((TT *)newData)[j] = c;
      else
        ((TT *)newData)[j] = ((TT *)orginalData)[j];
    }
  }

  // if_mask_excludes_then_valuec_else_data
  if (mode == 3) {
    for (size_t j = 0; j < l; j++) {
      if (((SS *)maskData)[j] >= a && ((SS *)maskData)[j] <= b)
        ((TT *)newData)[j] = ((TT *)orginalData)[j];
      else
        ((TT *)newData)[j] = c;
    }
  }

  // if_mask_includes_then_mask_else_data
  if (mode == 4) {
    for (size_t j = 0; j < l; j++) {
      if (((SS *)maskData)[j] >= a && ((SS *)maskData)[j] <= b)
        ((TT *)newData)[j] = ((SS *)maskData)[j];
      else
        ((TT *)newData)[j] = ((TT *)orginalData)[j];
    }
  }

  // if_mask_excludes_then_mask_else_data
  if (mode == 5) {
    for (size_t j = 0; j < l; j++) {
      if (((SS *)maskData)[j] >= a && ((SS *)maskData)[j] <= b)
        ((TT *)newData)[j] = ((TT *)orginalData)[j];
      else
        ((TT *)newData)[j] = ((SS *)maskData)[j];
    }
  }
}

int CDPPDATAMASK::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {

    if (dataSource->getDataObject(0)->cdfVariable->name.equals("masked")) return 0;
    CDBDebug("CDATAPOSTPROCESSOR_RUNBEFOREREADING::Applying datamask");
    CDF::Variable *varToClone = dataSource->getDataObject(0)->cdfVariable;

    CDataSource::DataObject *newDataObject = new CDataSource::DataObject();

    newDataObject->variableName.copy("masked");

    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), newDataObject);

    newDataObject->cdfVariable = new CDF::Variable();
    newDataObject->cdfObject = (CDFObject *)varToClone->getParentCDFObject();
    newDataObject->cdfObject->addVariable(newDataObject->cdfVariable);
    newDataObject->cdfVariable->setName("masked");
    CT::string text;
    text.print("{\"variable\":\"%s\",\"datapostproc\":\"%s\"}", "masked", this->getId());
    newDataObject->cdfVariable->removeAttribute("ADAGUC_DATAOBJECTID");
    newDataObject->cdfVariable->setAttributeText("ADAGUC_DATAOBJECTID", text.c_str());
    newDataObject->cdfVariable->setType(dataSource->getDataObject(1)->cdfVariable->getType());
    newDataObject->cdfVariable->setSize(dataSource->dWidth * dataSource->dHeight);

    for (size_t j = 0; j < varToClone->dimensionlinks.size(); j++) {
      newDataObject->cdfVariable->dimensionlinks.push_back(varToClone->dimensionlinks[j]);
    }

    for (size_t j = 0; j < varToClone->attributes.size(); j++) {
      newDataObject->cdfVariable->attributes.push_back(new CDF::Attribute(varToClone->attributes[j]));
    }

    newDataObject->cdfVariable->removeAttribute("scale_factor");
    newDataObject->cdfVariable->removeAttribute("add_offset");

    if (proc->attr.units.empty() == false) {
      newDataObject->cdfVariable->removeAttribute("units");
      newDataObject->setUnits(proc->attr.units.c_str());
      newDataObject->cdfVariable->setAttributeText("units", proc->attr.units.c_str());
    }
    if (proc->attr.name.empty() == false) {
      newDataObject->cdfVariable->removeAttribute("long_name");
      newDataObject->cdfVariable->setAttributeText("long_name", proc->attr.name.c_str());
    }
    newDataObject->cdfVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);

    // return 0;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    CDBDebug("Applying datamask");

    double fa = 0, fb = 1; // More or equal to a and less than b
    double fc = 0;
    if (proc->attr.a.empty() == false) {
      CT::string a;
      a.copy(proc->attr.a.c_str());
      fa = a.toDouble();
    }
    if (proc->attr.b.empty() == false) {
      CT::string b;
      b.copy(proc->attr.b.c_str());
      fb = b.toDouble();
    }
    if (proc->attr.c.empty() == false) {
      CT::string c;
      c.copy(proc->attr.c.c_str());
      fc = c.toDouble();
    }
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;

    int mode = 0; // replace with noDataValue

    if (proc->attr.mode.empty() == false) {
      if (proc->attr.mode.equals("if_mask_includes_then_nodata_else_data")) mode = 0;
      if (proc->attr.mode.equals("if_mask_excludes_then_nodata_else_data")) mode = 1;
      if (proc->attr.mode.equals("if_mask_includes_then_valuec_else_data")) mode = 2;
      if (proc->attr.mode.equals("if_mask_excludes_then_valuec_else_data")) mode = 3;
      if (proc->attr.mode.equals("if_mask_includes_then_mask_else_data")) mode = 4;
      if (proc->attr.mode.equals("if_mask_excludes_then_mask_else_data")) mode = 5;
    }

    void *newData = dataSource->getDataObject(0)->cdfVariable->data;
    void *orginalData = dataSource->getDataObject(1)->cdfVariable->data; // sunz
    void *maskData = dataSource->getDataObject(2)->cdfVariable->data;
    double newDataNoDataValue = (double)dataSource->getDataObject(1)->dfNodataValue;

    CDFType maskType = dataSource->getDataObject(2)->cdfVariable->getType();
    switch (dataSource->getDataObject(0)->cdfVariable->getType()) {
    case CDF_CHAR:
      DOIT<char, void>::doIt(newData, orginalData, maskData, newDataNoDataValue, maskType, fa, fb, fc, mode, l);
      break;
    case CDF_BYTE:
      DOIT<char, void>::doIt(newData, orginalData, maskData, newDataNoDataValue, maskType, fa, fb, fc, mode, l);
      break;
    case CDF_UBYTE:
      DOIT<unsigned char, void>::doIt(newData, orginalData, maskData, newDataNoDataValue, maskType, fa, fb, fc, mode, l);
      break;
    case CDF_SHORT:
      DOIT<short, void>::doIt(newData, orginalData, maskData, newDataNoDataValue, maskType, fa, fb, fc, mode, l);
      break;
    case CDF_USHORT:
      DOIT<ushort, void>::doIt(newData, orginalData, maskData, newDataNoDataValue, maskType, fa, fb, fc, mode, l);
      break;
    case CDF_INT:
      DOIT<int, void>::doIt(newData, orginalData, maskData, newDataNoDataValue, maskType, fa, fb, fc, mode, l);
      break;
    case CDF_UINT:
      DOIT<uint, void>::doIt(newData, orginalData, maskData, newDataNoDataValue, maskType, fa, fb, fc, mode, l);
      break;
    case CDF_FLOAT:
      DOIT<float, void>::doIt(newData, orginalData, maskData, newDataNoDataValue, maskType, fa, fb, fc, mode, l);
      break;
    case CDF_DOUBLE:
      DOIT<double, void>::doIt(newData, orginalData, maskData, newDataNoDataValue, maskType, fa, fb, fc, mode, l);
      break;
    }
  }
  return 0;
}