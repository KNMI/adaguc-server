#include "CDataPostProcessor.h"
#include "CRequest.h"
#include "CDataPostProcessor_IncludeLayer.h"
#include "CDataPostProcessor_ClipMinMax.h"
#include "CDataPostProcessor_Operator.h"
#include "CDataPostProcessor_SolarTerminator.h"

void writeLogFileLocal(const char *msg) {
  char *logfile = getenv("ADAGUC_LOGFILE");
  if (logfile != NULL) {
    FILE *pFile = NULL;
    pFile = fopen(logfile, "a");
    if (pFile != NULL) {
      //     setvbuf(pFile, NULL, _IONBF, 0);
      fputs(msg, pFile);
      if (strncmp(msg, "[D:", 3) == 0 || strncmp(msg, "[W:", 3) == 0 || strncmp(msg, "[E:", 3) == 0) {
        time_t myTime = time(NULL);
        tm *myUsableTime = localtime(&myTime);
        char szTemp[128];
        snprintf(szTemp, 127, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ ", myUsableTime->tm_year + 1900, myUsableTime->tm_mon + 1, myUsableTime->tm_mday, myUsableTime->tm_hour, myUsableTime->tm_min,
                 myUsableTime->tm_sec);
        fputs(szTemp, pFile);
      }
      fclose(pFile);
    } else
      fprintf(stderr, "Unable to write logfile %s\n", logfile);
  }
}

extern CDPPExecutor cdppExecutorInstance;
CDPPExecutor cdppExecutorInstance;
CDPPExecutor *CDataPostProcessor::getCDPPExecutor() { return &cdppExecutorInstance; }

/************************/
/*      CDPPAXplusB     */
/************************/
const char *CDPPAXplusB::className = "CDPPAXplusB";

const char *CDPPAXplusB::getId() { return "AX+B"; }
int CDPPAXplusB::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm.equals("ax+b")) {
    return CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}
int CDPPAXplusB::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) != CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    return -1;
  }
  if (dataSource->formatConverterActive) {
    /**
     * See issue https://github.com/KNMI/adaguc-server/issues/155
     *
     * Data post processor is meant for grids, not for other types of data like point times series.
     */
    CDBError("CDPPAXplusB not possible when a format converter is active.");
    return -1;
  }
  // CDBDebug("Applying ax+b");
  for (size_t varNr = 0; varNr < dataSource->getNumDataObjects(); varNr++) {
    dataSource->getDataObject(varNr)->hasScaleOffset = true;
    dataSource->getDataObject(varNr)->cdfVariable->setType(CDF_DOUBLE);

    // Apply offset
    if (proc->attr.b.empty() == false) {
      CDF::Attribute *add_offset = dataSource->getDataObject(varNr)->cdfVariable->getAttributeNE("add_offset");
      if (add_offset == NULL) {
        dataSource->getDataObject(varNr)->dfadd_offset = proc->attr.b.toDouble();
      } else {
        dataSource->getDataObject(varNr)->dfadd_offset += proc->attr.b.toDouble();
      }
    }
    // Apply scale
    if (proc->attr.a.empty() == false) {
      CDF::Attribute *scale_factor = dataSource->getDataObject(varNr)->cdfVariable->getAttributeNE("scale_factor");
      if (scale_factor == NULL) {
        dataSource->getDataObject(varNr)->dfscale_factor = proc->attr.a.toDouble();
      } else {
        dataSource->getDataObject(varNr)->dfscale_factor *= proc->attr.a.toDouble();
      }
    }
    if (proc->attr.units.empty() == false) {
      dataSource->getDataObject(varNr)->setUnits(proc->attr.units.c_str());
    }
  }
  return 0;
}

/************************/
/*CDPPDATAMASK */
/************************/
const char *CDPPDATAMASK::className = "CDPPDATAMASK";

const char *CDPPDATAMASK::getId() { return "datamask"; }
int CDPPDATAMASK::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("datamask")) {
    if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
      if (dataSource->getNumDataObjects() != 2 && dataSource->getNumDataObjects() != 3) {
        CDBError("2 or 3 variables are needed for datamask, found %d", dataSource->getNumDataObjects());
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

/************************/
/*CDPPMSGCPPVisibleMask */
/************************/
const char *CDPPMSGCPPVisibleMask::className = "CDPPMSGCPPVisibleMask";

const char *CDPPMSGCPPVisibleMask::getId() { return "MSGCPP_VISIBLEMASK"; }
int CDPPMSGCPPVisibleMask::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("msgcppvisiblemask")) {
    if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
      if (dataSource->getNumDataObjects() != 2 && dataSource->getNumDataObjects() != 3) {
        CDBError("2 or 3 variables are needed for msgcppvisiblemask, found %d", dataSource->getNumDataObjects());
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
    newDataObject->cdfVariable->setAttribute("_FillValue", newDataObject->cdfVariable->getType(), attrData, 1);

    attrData[0] = 0;
    attrData[1] = 1;
    newDataObject->cdfVariable->setAttribute("valid_range", newDataObject->cdfVariable->getType(), attrData, 2);
    newDataObject->cdfVariable->setAttribute("flag_values", newDataObject->cdfVariable->getType(), attrData, 2);
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
const char *CDPPMSGCPPHIWCMask::className = "CDPPMSGCPPHIWCMask";

const char *CDPPMSGCPPHIWCMask::getId() { return "MSGCPP_HIWCMASK"; }

int CDPPMSGCPPHIWCMask::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("msgcpphiwcmask")) {
    if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
      if (dataSource->getNumDataObjects() != 4 && dataSource->getNumDataObjects() != 5) {
        CDBError("4 or 5 variables are needed for msgcpphiwcmask, found %d", dataSource->getNumDataObjects());
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
    newDataObject->cdfVariable->setAttribute("_FillValue", newDataObject->cdfVariable->getType(), attrData, 1);

    attrData[0] = 0;
    attrData[1] = 1;
    newDataObject->cdfVariable->setAttribute("valid_range", newDataObject->cdfVariable->getType(), attrData, 2);
    newDataObject->cdfVariable->setAttribute("flag_values", newDataObject->cdfVariable->getType(), attrData, 2);
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
            int status = 0;
            // Check if the postProcessor is of type SolarTerminator
            if (CDPPSolarTerminator *solarTerminator = dynamic_cast<CDPPSolarTerminator *>(dataPostProcessorList->get(procId))) {
              // Call a different method specific to SolarTerminator
              status = solarTerminator->execute(proc, dataSource, CDATAPOSTPROCESSOR_RUNBEFOREREADING);
            } else {
              // Call the regular execute method for other types
              CDBDebug("NORMAL POSTPROCESSOR");
              status = dataPostProcessorList->get(procId)->execute(proc, dataSource, CDATAPOSTPROCESSOR_RUNBEFOREREADING);
            }

            // int status = dataPostProcessorList->get(procId)->execute(proc, dataSource, CDATAPOSTPROCESSOR_RUNBEFOREREADING, timestamp);
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
            int status = 0;
            // Check if the postProcessor is of type SolarTerminator
            if (CDPPSolarTerminator *solarTerminator = dynamic_cast<CDPPSolarTerminator *>(dataPostProcessorList->get(procId))) {
              // Call a different method specific to SolarTerminator
              CDBDebug("SPECIAL CALL FOR SOLAR TERMINATOR RUNAFTERREADING");
              status = solarTerminator->execute(proc, dataSource, CDATAPOSTPROCESSOR_RUNAFTERREADING);
            } else {
              // Call the regular execute method for other types
              CDBDebug("NORMAL POSTPROCESSOR");
              status = dataPostProcessorList->get(procId)->execute(proc, dataSource, CDATAPOSTPROCESSOR_RUNAFTERREADING);
            }
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

int CDPPExecutor::executeProcessors(CDataSource *dataSource, int mode, double *data, size_t numItems) { return executeProcessors(dataSource, mode, data, numItems, 0); }
int CDPPExecutor::executeProcessors(CDataSource *dataSource, int mode, double *data, size_t numItems, double timestamp) {
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
            CDBDebug("!!!!!!!!!!!!!!!!!Executing all processors with timestamp %f", timestamp);
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

/************************/
/*      CDPPBeaufort     */
/************************/
const char *CDPPBeaufort::className = "CDPPBeaufort";

const char *CDPPBeaufort::getId() { return "beaufort"; }
int CDPPBeaufort::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("beaufort")) {
    if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
      if (dataSource->getNumDataObjects() != 1 && dataSource->getNumDataObjects() != 2) {
        CDBError("1 or 2 variables are needed for beaufort, found %d", dataSource->getNumDataObjects());
        return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
      }
      return CDATAPOSTPROCESSOR_RUNAFTERREADING;
    }
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

float CDPPBeaufort::getBeaufort(float speed) {
  int bft;
  if (speed < 0.3) {
    bft = 0;
  } else if (speed < 1.6) {
    bft = 1;
  } else if (speed < 3.4) {
    bft = 2;
  } else if (speed < 5.5) {
    bft = 3;
  } else if (speed < 8.0) {
    bft = 4;
  } else if (speed < 10.8) {
    bft = 5;
  } else if (speed < 13.9) {
    bft = 6;
  } else if (speed < 17.2) {
    bft = 7;
  } else if (speed < 20.8) {
    bft = 8;
  } else if (speed < 24.5) {
    bft = 9;
  } else if (speed < 28.5) {
    bft = 10;
  } else if (speed < 32.6) {
    bft = 11;
  } else {
    bft = 12;
  }
  //  CDBDebug("bft(%f)=%d", speed, bft);
  return bft;
}
int CDPPBeaufort::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  CDBDebug("Applying beaufort %d", mode == CDATAPOSTPROCESSOR_RUNAFTERREADING);
  float factor = 1;
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    if (dataSource->getNumDataObjects() == 1) {
      CDBDebug("Applying beaufort for 1 element");
      if (dataSource->getDataObject(0)->getUnits().equals("knot") || dataSource->getDataObject(0)->getUnits().equals("kt")) {
        factor = 1852. / 3600;
      }
      CDBDebug("Applying beaufort for 1 element with factor %f", factor);
      dataSource->getDataObject(0)->setUnits("bft");
      size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
      float *src = (float *)dataSource->getDataObject(0)->cdfVariable->data;
      float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
      for (size_t cnt = 0; cnt < l; cnt++) {
        float speed = *src;
        if (speed == speed) {
          if (speed != noDataValue) {
            *src = getBeaufort(factor * speed);
          }
        }
        src++;
      }
      // Convert point data if needed
      size_t nrPoints = dataSource->getDataObject(0)->points.size();
      CDBDebug("(1): %d points", nrPoints);

      for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
        float speed = (float)dataSource->getDataObject(0)->points[pointNo].v;
        if (speed == speed) {
          if (speed != noDataValue) {
            dataSource->getDataObject(0)->points[pointNo].v = getBeaufort(factor * speed);
          }
        }
      }
    }
    if (dataSource->getNumDataObjects() == 2) {
      CDBDebug("Applying beaufort for 2 elements %s %s", dataSource->getDataObject(0)->getUnits().c_str(), dataSource->getDataObject(1)->getUnits().c_str());
      if ((dataSource->getDataObject(0)->getUnits().equals("m/s") || dataSource->getDataObject(0)->getUnits().equals("m s-1")) && dataSource->getDataObject(1)->getUnits().equals("degree")) {
        // This is a (wind speed,direction) pair
        dataSource->getDataObject(0)->setUnits("bft");
        size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
        float *src = (float *)dataSource->getDataObject(0)->cdfVariable->data;
        float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
        for (size_t cnt = 0; cnt < l; cnt++) {
          float speed = *src;
          if (speed == speed) {
            if (speed != noDataValue) {
              *src = getBeaufort(factor * speed);
            }
          }
          src++;
        }
        // Convert point data if needed
        size_t nrPoints = dataSource->getDataObject(0)->points.size();
        CDBDebug("(2): %d points", nrPoints);
        for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
          float speed = dataSource->getDataObject(0)->points[pointNo].v;
          if (speed == speed) {
            if (speed != noDataValue) {
              dataSource->getDataObject(0)->points[pointNo].v = getBeaufort(factor * speed);
            }
          }
        }
      }
      if ((dataSource->getDataObject(0)->getUnits().equals("m/s") || dataSource->getDataObject(0)->getUnits().equals("m s-1")) &&
          (dataSource->getDataObject(1)->getUnits().equals("m/s") || dataSource->getDataObject(1)->getUnits().equals("m s-1"))) {
        // This is a (u,v) pair
        dataSource->getDataObject(0)->setUnits("bft");

        size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
        float *srcu = (float *)dataSource->getDataObject(0)->cdfVariable->data;
        float *srcv = (float *)dataSource->getDataObject(1)->cdfVariable->data;
        float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
        float speed;
        float speedu;
        float speedv;
        for (size_t cnt = 0; cnt < l; cnt++) {
          speedu = *srcu;
          speedv = *srcv;
          if ((speedu == speedu) && (speedv == speedv)) {
            if ((speedu != noDataValue) && (speedv != noDataValue)) {
              speed = factor * hypot(speedu, speedv);
              *srcu = getBeaufort(speed);
            } else {
              *srcu = noDataValue;
            }
          }
          srcu++;
          srcv++;
        }
        // Convert point data if needed
        size_t nrPoints = dataSource->getDataObject(0)->points.size();
        CDBDebug("(2): %d points", nrPoints);
        for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
          speedu = dataSource->getDataObject(0)->points[pointNo].v;
          speedv = dataSource->getDataObject(1)->points[pointNo].v;
          if ((speedu == speedu) && (speedv == speedv)) {
            if ((speedu != noDataValue) && (speedv != noDataValue)) {
              speed = factor * hypot(speedu, speedv);
              dataSource->getDataObject(0)->points[pointNo].v = getBeaufort(speed);
            } else {
              dataSource->getDataObject(0)->points[pointNo].v = noDataValue;
            }
          }
        }
        CDBDebug("Deleting dataObject(1))");
        delete (dataSource->getDataObject(1));
        dataSource->getDataObjectsVector()->erase(dataSource->getDataObjectsVector()->begin() + 1); // Remove second element
      }
    }
  }
  return 0;
}

/************************/
/*      CDPPToKnots     */
/************************/
const char *CDPPToKnots::className = "CDPPToToKnots";

const char *CDPPToKnots::getId() { return "toknots"; }
int CDPPToKnots::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int) {
  if (proc->attr.algorithm.equals("toknots")) {
    if (CDATAPOSTPROCESSOR_RUNAFTERREADING) {
      if (dataSource->getNumDataObjects() != 1 && dataSource->getNumDataObjects() != 2) {
        CDBError("1 or 2 variables are needed for toknots, found %d", dataSource->getNumDataObjects());
        return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
      }
      return CDATAPOSTPROCESSOR_RUNAFTERREADING;
    }
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPToKnots::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  CDBDebug("Applying toknots %d", mode == CDATAPOSTPROCESSOR_RUNAFTERREADING);
  float factor = 1;
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    if (dataSource->getNumDataObjects() == 1) {
      CDBDebug("Applying toknots for 1 element");
      if (dataSource->getDataObject(0)->getUnits().equals("m/s") || dataSource->getDataObject(0)->getUnits().equals("m s-1")) {
        factor = 3600 / 1852.;
        CDBDebug("Applying toknots for 1 element with factor %f to grid", factor);
        dataSource->getDataObject(0)->setUnits("kts");
        size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
        float *src = (float *)dataSource->getDataObject(0)->cdfVariable->data;
        float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
        for (size_t cnt = 0; cnt < l; cnt++) {
          float speed = *src;
          if (speed == speed) {
            if (speed != noDataValue) {
              *src = factor * speed;
            }
          }
          src++;
        }

        // Convert point data if needed
        size_t nrPoints = dataSource->getDataObject(0)->points.size();
        CDBDebug("(1): %d points", nrPoints);
        for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
          float speed = (float)dataSource->getDataObject(0)->points[pointNo].v;
          if (speed == speed) {
            if (speed != noDataValue) {
              dataSource->getDataObject(0)->points[pointNo].v = factor * speed;
            }
          }
        }
      }
    }
    if (dataSource->getNumDataObjects() == 2) {
      CDBDebug("Applying toknots for 2 elements %s %s", dataSource->getDataObject(0)->getUnits().c_str(), dataSource->getDataObject(1)->getUnits().c_str());
      if ((dataSource->getDataObject(0)->getUnits().equals("m/s") || dataSource->getDataObject(0)->getUnits().equals("m s-1")) && dataSource->getDataObject(1)->getUnits().equals("degree")) {
        factor = 3600 / 1852.;
        // This is a (wind speed,direction) pair
        dataSource->getDataObject(0)->setUnits("kts");
        size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
        float *src = (float *)dataSource->getDataObject(0)->cdfVariable->data;
        float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
        for (size_t cnt = 0; cnt < l; cnt++) {
          float speed = *src;
          if (speed == speed) {
            if (speed != noDataValue) {
              *src = factor * speed;
            }
          }
          src++;
        }
        // Convert point data if needed
        size_t nrPoints = dataSource->getDataObject(0)->points.size();
        CDBDebug("(2): %d points", nrPoints);
        for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
          float speed = dataSource->getDataObject(0)->points[pointNo].v;
          if (speed == speed) {
            if (speed != noDataValue) {
              dataSource->getDataObject(0)->points[pointNo].v = factor * speed;
            }
          }
        }
      }
      if ((dataSource->getDataObject(0)->getUnits().equals("m/s") || dataSource->getDataObject(0)->getUnits().equals("m s-1")) &&
          (dataSource->getDataObject(1)->getUnits().equals("m/s") || dataSource->getDataObject(1)->getUnits().equals("m s-1"))) {
        // This is a (u,v) pair
        dataSource->getDataObject(0)->setUnits("kts");

        size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
        float *srcu = (float *)dataSource->getDataObject(0)->cdfVariable->data;
        float *srcv = (float *)dataSource->getDataObject(1)->cdfVariable->data;
        float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
        float speed;
        float speedu;
        float speedv;
        for (size_t cnt = 0; cnt < l; cnt++) {
          speedu = *srcu;
          speedv = *srcv;
          if ((speedu == speedu) && (speedv == speedv)) {
            if ((speedu != noDataValue) && (speedv != noDataValue)) {
              speed = factor * hypot(speedu, speedv);
              *srcu = speed;
            } else {
              *srcu = noDataValue;
            }
          }
          srcu++;
          srcv++;
        }
        // Convert point data if needed
        size_t nrPoints = dataSource->getDataObject(0)->points.size();
        CDBDebug("(2): %d points", nrPoints);
        for (size_t pointNo = 0; pointNo < nrPoints; pointNo++) {
          speedu = dataSource->getDataObject(0)->points[pointNo].v;
          speedv = dataSource->getDataObject(1)->points[pointNo].v;
          if ((speedu == speedu) && (speedv == speedv)) {
            if ((speedu != noDataValue) && (speedv != noDataValue)) {
              speed = factor * hypot(speedu, speedv);
              dataSource->getDataObject(0)->points[pointNo].v = speed;
            } else {
              dataSource->getDataObject(0)->points[pointNo].v = noDataValue;
            }
          }
        }
        CDBDebug("Deleting dataObject(1))");
        delete (dataSource->getDataObject(1));
        dataSource->getDataObjectsVector()->erase(dataSource->getDataObjectsVector()->begin() + 1); // Remove second element
      }
    }
  }
  return 0;
}
/************************/
/*      CDPDBZtoRR     */
/************************/
const char *CDPDBZtoRR::className = "CDPDBZtoRR";

const char *CDPDBZtoRR::getId() { return "dbztorr"; }
int CDPDBZtoRR::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm.equals("dbztorr")) {
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

float CDPDBZtoRR::getRR(float dbZ) {
  // TODO: Check why -32 as input does not return 0
  return pow((pow(10, dbZ / 10.) / 200), 1 / 1.6);
}

int CDPDBZtoRR::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numItems) {
  CDBDebug("CDPDBZtoRR");
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    dataSource->getDataObject(0)->setUnits("mm/hr");
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
    for (size_t j = 0; j < numItems; j++) {
      if (data[j] == data[j]) {       // Check if NaN
        if (data[j] != noDataValue) { // Check if equal to nodatavalue of the datasource
          data[j] = getRR(data[j]);
        }
      }
    }
  }
  return 0;
}

int CDPDBZtoRR::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    dataSource->getDataObject(0)->setUnits("mm/hr");
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    float *src = (float *)dataSource->getDataObject(0)->cdfVariable->data;
    float noDataValue = dataSource->getDataObject(0)->dfNodataValue;
    for (size_t cnt = 0; cnt < l; cnt++) {
      float dbZ = *src;
      if (dbZ == dbZ) {
        if (dbZ != noDataValue) {
          *src = getRR(dbZ);
        }
      }
      src++;
    }
  }
  return 0;
}

/************************/
/*      CDPPAddFeatures     */
/************************/
const char *CDPPAddFeatures::className = "CDPPAddFeatures";

const char *CDPPAddFeatures::getId() { return "addfeatures"; }
int CDPPAddFeatures::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm.equals("addfeatures")) {
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPAddFeatures::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    //    dataSource->getDataObject(0)->cdfVariable->setAttributeText("units","mm/hr");
    //    dataSource->getDataObject(0)->setUnits("mm/hr");
    try {
      if (dataSource->getDataObject(0)->cdfVariable->getAttribute("ADAGUC_GEOJSONPOINT")) return 0;
    } catch (int a) {
    }
    CDBDebug("CDATAPOSTPROCESSOR_RUNBEFOREREADING::Adding features from GEOJson");
    CDF::Variable *varToClone = dataSource->getDataObject(0)->cdfVariable;

    CDataSource::DataObject *newDataObject = new CDataSource::DataObject();

    newDataObject->variableName.copy("indexes");
    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin() + 1, newDataObject);

    newDataObject->cdfVariable = new CDF::Variable();
    newDataObject->cdfObject = (CDFObject *)varToClone->getParentCDFObject();
    newDataObject->cdfObject->addVariable(newDataObject->cdfVariable);
    newDataObject->cdfVariable->setName("indexes");
    newDataObject->cdfVariable->setType(CDF_USHORT);
    newDataObject->cdfVariable->setSize(dataSource->dWidth * dataSource->dHeight);

    for (size_t j = 0; j < varToClone->dimensionlinks.size(); j++) {
      newDataObject->cdfVariable->dimensionlinks.push_back(varToClone->dimensionlinks[j]);
    }
    newDataObject->cdfVariable->removeAttribute("standard_name");
    newDataObject->cdfVariable->removeAttribute("_FillValue");

    newDataObject->cdfVariable->setAttributeText("standard_name", "indexes");
    newDataObject->cdfVariable->setAttributeText("long_name", "indexes");
    newDataObject->cdfVariable->setAttributeText("units", "1");
    newDataObject->cdfVariable->setAttributeText("ADAGUC_GEOJSONPOINT", "1");
    dataSource->getDataObject(0)->cdfVariable->setAttributeText("ADAGUC_GEOJSONPOINT", "1");

    unsigned short sf = 65535u;
    newDataObject->cdfVariable->setAttribute("_FillValue", newDataObject->cdfVariable->getType(), &sf, 1);
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    CDataSource featureDataSource;
    if (featureDataSource.setCFGLayer(dataSource->srvParams, dataSource->srvParams->configObj->Configuration[0], dataSource->srvParams->cfg->Layer[0], NULL, 0) != 0) {
      return 1;
    }
    featureDataSource.addStep(proc->attr.a.c_str(), NULL); // Set filename
    CDataReader reader;
    CDBDebug("Opening %s", featureDataSource.getFileName());
    int status = reader.open(&featureDataSource, CNETCDFREADER_MODE_OPEN_ALL);
    //   CDBDebug("fds: %s", CDF::dump(featureDataSource.getDataObject(0)->cdfObject).c_str());

    if (status != 0) {
      CDBDebug("Can't open file %s", proc->attr.a.c_str());
      return 1;
    } else {
      CDF::Variable *fvar = featureDataSource.getDataObject(0)->cdfObject->getVariableNE("featureids");
      size_t nrFeatures = 0;
      if (fvar == NULL) {
        CDBDebug("featureids not found");
      } else {
        //       CDBDebug("featureids found %d %d", fvar->getType(), fvar->dimensionlinks[0]->getSize());
        size_t start = 0;
        nrFeatures = fvar->dimensionlinks[0]->getSize();
        ptrdiff_t stride = 1;
        fvar->readData(CDF_STRING, &start, &nrFeatures, &stride, false);
        //         for (size_t i=0; i<nrFeatures; i++) {
        //           CDBDebug(">> %s", ((char **)fvar->data)[i]);
        //         }
      }
      char **names = (char **)fvar->data;

      float destNoDataValue = dataSource->getDataObject(0)->dfNodataValue;

      std::vector<std::string> valueMap;
      size_t nrPoints = dataSource->getDataObject(0)->points.size();
      float valueForFeatureNr[nrFeatures];
      for (size_t f = 0; f < nrFeatures; f++) {
        valueForFeatureNr[f] = destNoDataValue;
        const char *name = names[f];
        bool found = false;
        for (size_t p = 0; p < nrPoints && !found; p++) {
          for (size_t c = 0; c < dataSource->getDataObject(0)->points[p].paramList.size() && !found; c++) {
            CKeyValue ckv = dataSource->getDataObject(0)->points[p].paramList[c];
            //            CDBDebug("ckv: %s %s", ckv.key.c_str(), ckv.value.c_str());
            if (ckv.key.equals("station")) {
              CT::string station = ckv.value;
              //              CDBDebug("  comparing %s %s", station.c_str(), name);
              if (strcmp(station.c_str(), name) == 0) {
                valueForFeatureNr[f] = dataSource->getDataObject(0)->points[p].v;
                //                CDBDebug("Found %s as %d (%f)", name, f, valueForFeatureNr[f]);
                found = true;
              }
            }
          }
        }
      }

      CDF::allocateData(dataSource->getDataObject(1)->cdfVariable->getType(), &dataSource->getDataObject(1)->cdfVariable->data, dataSource->dWidth * dataSource->dHeight); // For a 2D field
      // Copy the gridded values from the geojson grid to the point data's grid
      size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
      unsigned short *src = (unsigned short *)featureDataSource.getDataObject(0)->cdfVariable->data;
      float *dest = (float *)dataSource->getDataObject(0)->cdfVariable->data;
      unsigned short noDataValue = featureDataSource.getDataObject(0)->dfNodataValue;
      unsigned short *indexDest = (unsigned short *)dataSource->getDataObject(1)->cdfVariable->data;

      //     size_t nrFeatures=valueMap.size();
      for (size_t cnt = 0; cnt < l; cnt++) {
        unsigned short featureNr = *src; // index of station in GeoJSON file
        *dest = destNoDataValue;
        *indexDest = 65535u;
        //         if (cnt<30) {
        //           CDBDebug("cnt=%d %d %f", cnt, featureNr, (featureNr!=noDataValue)?featureNr:-9999999);
        //         }
        if (featureNr != noDataValue) {
          *dest = valueForFeatureNr[featureNr];
          *indexDest = featureNr;
        }
        src++;
        dest++;
        indexDest++;
      }
      CDBDebug("Setting ADAGUC_SKIP_POINTS");
      dataSource->getDataObject(0)->cdfVariable->setAttributeText("ADAGUC_SKIP_POINTS", "1");
      dataSource->getDataObject(1)->cdfVariable->setAttributeText("ADAGUC_SKIP_POINTS", "1");
    }
  }
  return 0;
}
