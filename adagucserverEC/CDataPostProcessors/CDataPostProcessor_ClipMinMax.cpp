#include "CDataPostProcessor_ClipMinMax.h"

/************************/
/*      CDPPClipMinMax  */
/************************/

const char *CDPPClipMinMax::getId() { return "clipminmax"; }
int CDPPClipMinMax::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm.equals("clipminmax")) {
    return CDATAPOSTPROCESSOR_RUNAFTERREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

template <class T> void clipData(T *data, size_t size, double min, double max) {
  for (size_t j = 0; j < size; j++) {
    if (data[j] < min) data[j] = min;
    if (data[j] > max) data[j] = max;
  }
}

int CDPPClipMinMax::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) != CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    return -1;
  }
  CDBDebug("Applying clipminmax");
  for (size_t varNr = 0; varNr < dataSource->getNumDataObjects(); varNr++) {
    const size_t s = dataSource->getDataObject(varNr)->cdfVariable->getSize();
    void *d = dataSource->getDataObject(varNr)->cdfVariable->data;
    double fa = 0, fb = 0;
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
    switch (dataSource->getDataObject(0)->cdfVariable->getType()) {
    case CDF_CHAR:
      clipData((char *)d, s, fa, fb);
      break;
    case CDF_BYTE:
      clipData((char *)d, s, fa, fb);
      break;
    case CDF_UBYTE:
      clipData((unsigned char *)d, s, fa, fb);
      break;
    case CDF_SHORT:
      clipData((short *)d, s, fa, fb);
      break;
    case CDF_USHORT:
      clipData((ushort *)d, s, fa, fb);
      break;
    case CDF_INT:
      clipData((int *)d, s, fa, fb);
      break;
    case CDF_UINT:
      clipData((uint *)d, s, fa, fb);
      break;
    case CDF_FLOAT:
      clipData((float *)d, s, fa, fb);
      break;
    case CDF_DOUBLE:
      clipData((double *)d, s, fa, fb);
      break;
    }
  }
  return 0;
}
