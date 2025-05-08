#include "CDataPostProcessor_UVComponents.h"
#include "CRequest.h"
#include "CGenericDataWarper.h"
#include <utils/LayerUtils.h>
#include <CImgRenderFieldVectors.h>

/************************/
/*      CDDPUVComponents  */
/************************/
const char *CDDPUVComponents::className = "CDDPUVComponents";

const char *CDDPUVComponents::getId() { return CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ID; }

int CDDPUVComponents::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals(getId())) {
    if (dataSource->getNumDataObjects() == 1 && mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
      CDBError("2 variables are needed for UVCOMPONENTS, found %d", dataSource->getNumDataObjects());
      return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
    }
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int executeBeforeReading(CDataSource *dataSource) {
  if (dataSource->getDataObjectByName(CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_SPEED_COMPONENT) != nullptr) {
    CDBDebug("Datamodel seems already applied. Skipping!");
    return 0;
  }

  auto cdfObject = dataSource->getDataObject(0)->cdfObject;

  dataSource->getDataObject(0)->dataObjectName = CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_U_COMPONENT;
  dataSource->getDataObject(1)->dataObjectName = CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_V_COMPONENT;

  CDataSource::DataObject *ugridabsolute = dataSource->getDataObject(0)->clone(CDF_FLOAT, U_COMPONENT_GRID_ABSOLUTE);
  ugridabsolute->noFurtherProcessing = true;
  ugridabsolute->cdfVariable->setAttributeText("standard_name", "eastward_wind");
  ugridabsolute->cdfVariable->setAttributeText("long_name", "eastward_wind");
  ugridabsolute->cdfVariable->removeAttribute("short_name");
  ugridabsolute->cdfVariable->setAttributeText("note", "Created by UVCOMPONENTS data post processor. This is absolute eastward wind.");
  cdfObject->addVariable(ugridabsolute->cdfVariable);

  CDataSource::DataObject *vgridabsolute = dataSource->getDataObject(1)->clone(CDF_FLOAT, V_COMPONENT_GRID_ABSOLUTE);
  vgridabsolute->noFurtherProcessing = true;
  vgridabsolute->cdfVariable->setAttributeText("standard_name", "northward_wind");
  vgridabsolute->cdfVariable->setAttributeText("long_name", "northward_wind");
  vgridabsolute->cdfVariable->removeAttribute("short_name");
  vgridabsolute->cdfVariable->setAttributeText("note", "Created by UVCOMPONENTS data post processor. This is absolute northward wind.");
  cdfObject->addVariable(vgridabsolute->cdfVariable);

  CDataSource::DataObject *speedObject = dataSource->getDataObject(0)->clone(CDF_FLOAT, SPEED_COMPONENT);
  speedObject->noFurtherProcessing = true;
  speedObject->cdfVariable->setAttributeText("standard_name", "wind_speed");
  speedObject->cdfVariable->setAttributeText("long_name", "Wind speed");
  speedObject->cdfVariable->removeAttribute("short_name");
  speedObject->cdfVariable->setAttributeText("note", "Created by UVCOMPONENTS data post processor");
  speedObject->dataObjectName = CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_SPEED_COMPONENT;
  cdfObject->addVariable(speedObject->cdfVariable);

  CDataSource::DataObject *directionObject = dataSource->getDataObject(0)->clone(CDF_FLOAT, DIRECTION_COMPONENT);
  directionObject->noFurtherProcessing = true;
  directionObject->setUnits("degrees");
  directionObject->cdfVariable->setAttributeText("units", "degrees");
  directionObject->cdfVariable->setAttributeText("standard_name", "wind_direction");
  directionObject->cdfVariable->setAttributeText("long_name", "Wind direction");
  directionObject->cdfVariable->removeAttribute("short_name");
  directionObject->cdfVariable->setAttributeText("note", "Created by UVCOMPONENTS data post processor");
  directionObject->dataObjectName = CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_DIRECTION_COMPONENT;
  cdfObject->addVariable(directionObject->cdfVariable);

  dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), vgridabsolute);
  dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), ugridabsolute);
  dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), directionObject);
  dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), speedObject);

  cdfObject->setAttributeText(CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ID, "true");
  return 0;
}

int CDDPUVComponents::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if ((isApplicable(proc, dataSource, mode) & mode) == false) {
    return -1;
  }
  CDBDebug("Applying CDDPUVComponents for grid with mode %d", mode);
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    return executeBeforeReading(dataSource);
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    CImageWarper warper;
    if (dataSource->srvParams->Geo->CRS.empty()) {
      dataSource->srvParams->Geo->CRS = "EPSG:4236";
    }
    warper.initreproj(dataSource, dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);

    int dPixelExtent[4];
    dPixelExtent[0] = 0;
    dPixelExtent[1] = 0;
    dPixelExtent[2] = dataSource->dWidth;
    dPixelExtent[3] = dataSource->dHeight;

    CDataSource::DataObject *ugridabsolute = dataSource->getDataObject(U_COMPONENT_GRID_ABSOLUTE);
    CDataSource::DataObject *vgridabsolute = dataSource->getDataObject(V_COMPONENT_GRID_ABSOLUTE);
    CDataSource::DataObject *speedObject = dataSource->getDataObject(SPEED_COMPONENT);
    CDataSource::DataObject *directionObject = dataSource->getDataObject(DIRECTION_COMPONENT);

    ugridabsolute->cdfVariable->copy(dataSource->getDataObject(CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_U_COMPONENT)->cdfVariable);
    vgridabsolute->cdfVariable->copy(dataSource->getDataObject(CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_V_COMPONENT)->cdfVariable);

    size_t size = speedObject->cdfVariable->getSize();
    CDBDebug("Size %d", size);

    directionObject->cdfVariable->allocateData(size);
    speedObject->cdfVariable->allocateData(size);

    float *uValues = (float *)ugridabsolute->cdfVariable->data;
    float *vValues = (float *)vgridabsolute->cdfVariable->data;
    float *directionData = (float *)directionObject->cdfVariable->data;
    float *speedData = (float *)speedObject->cdfVariable->data;

    applyUVConversion(&warper, dataSource, dPixelExtent, uValues, vValues);

    for (size_t j = 0; j < size; j++) {
      speedData[j] = sqrt(uValues[j] * uValues[j] + vValues[j] * vValues[j]);
      directionData[j] = ((atan2(uValues[j], vValues[j])) * (180 / M_PI) + 180);
    }
  }

  return 0;
}

int a = 0;

int CDDPUVComponents::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t size) {
  if ((isApplicable(proc, dataSource, mode) & mode) == false) {
    return -1;
  }
  CDBDebug("Applying CDDPUVComponents for timeseries");
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    return executeBeforeReading(dataSource);
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    // TODO: FIX TIME SERIES PROCESSOR
    for (size_t j = 0; j < size; j++) {
      data[j] = j + a * size;
    }
  }
  a++;
  return 0;
}
