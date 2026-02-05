#include "CDataPostProcessor_AXplusB.h"

const char *CDPPAXplusB::getId() { return CDATAPOSTPROCESSOR_AXPLUSB_ID; }

int CDPPAXplusB::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm.equals(CDATAPOSTPROCESSOR_AXPLUSB_ID)) {
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
    if (dataSource->getDataObject(varNr)->noFurtherProcessing) {
      continue;
    }
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