#include "CDataPostProcessor_HighLowAreaDetector.h"

/************************/
/*  CDPPHighLowAreaDetector  */
/************************/

/**Example:
 *
  <Style name="highlow_areas">
    <DataPostProc algorithm="highlowareadetector"/>
  </Style>


  <Layer type="database">
    <Name>air_pressure_at_sea_level</Name>
    <Title>High/Low pressure areas</Title>
    <Variable>air_pressure_at_sea_level</Variable>
    <FilePath filter="">{ADAGUC_PATH}data/datasets/pressure-for-h-and-l-detection.nc</FilePath>
    <Dimension name="time" units="ISO8601" default="min">time</Dimension>
    <Styles>highlow_areas</Styles>
  </Layer>
 */

const char *CDPPHighLowAreaDetector::getId() { return CDATAPOSTPROCESSOR_HighLowAreaDetector_ID; }
int CDPPHighLowAreaDetector::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *, int) {
  if (proc->attr.algorithm == (CDATAPOSTPROCESSOR_HighLowAreaDetector_ID)) {
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPHighLowAreaDetector::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *, size_t) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  CDBError("Not implemented yet");
  return 1;
}

int CDPPHighLowAreaDetector::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (isApplicable(proc, dataSource, mode) == false) {
    return -1;
  }
  if (mode != CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    return 0;
  }

  CDBDebug(CDATAPOSTPROCESSOR_HighLowAreaDetector_ID);

  CDBDebug("Not implemented yet");
  return 0;
}
