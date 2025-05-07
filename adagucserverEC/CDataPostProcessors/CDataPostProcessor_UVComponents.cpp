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

int CDDPUVComponents::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if ((isApplicable(proc, dataSource, mode) & mode) == false) {
    return -1;
  }

  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {

    if (dataSource->getDataObjectByName(CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_SPEED_COMPONENT) != nullptr) {
      CDBDebug("Datamodel seems already applied. Skipping!");
      return 0;
    }

    auto cdfObject = dataSource->getDataObject(0)->cdfObject;

    dataSource->getDataObject(0)->dataObjectName = CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_U_COMPONENT;
    dataSource->getDataObject(1)->dataObjectName = CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_V_COMPONENT;

    CDataSource::DataObject *ugridrel = dataSource->getDataObject(0)->clone();
    ugridrel->cdfObject = cdfObject;
    ugridrel->cdfVariable = dataSource->getDataObject(1)->cdfVariable->clone(CDF_FLOAT, U_COMPONENT_GRID_ABSOLUTE);
    ugridrel->variableName = ugridrel->cdfVariable->name;
    ugridrel->cdfVariable->setAttributeText("standard_name", "eastward_wind");
    ugridrel->cdfVariable->setAttributeText("long_name", "eastward_wind");
    ugridrel->cdfVariable->removeAttribute("short_name");
    ugridrel->cdfVariable->setAttributeText("note", "Created by UVCOMPONENTS data post processor. This is absolute eastward wind.");
    cdfObject->addVariable(ugridrel->cdfVariable);

    CDataSource::DataObject *vgridrel = dataSource->getDataObject(1)->clone();
    vgridrel->cdfObject = cdfObject;
    vgridrel->cdfVariable = dataSource->getDataObject(1)->cdfVariable->clone(CDF_FLOAT, V_COMPONENT_GRID_ABSOLUTE);
    vgridrel->variableName.copy(vgridrel->cdfVariable->name);

    vgridrel->cdfVariable->setAttributeText("standard_name", "northward_wind");
    vgridrel->cdfVariable->setAttributeText("long_name", "northward_wind");
    vgridrel->cdfVariable->removeAttribute("short_name");
    vgridrel->cdfVariable->setAttributeText("note", "Created by UVCOMPONENTS data post processor. This is absolute northward wind.");
    cdfObject->addVariable(vgridrel->cdfVariable);

    CDataSource::DataObject *speedObject = dataSource->getDataObject(0)->clone();
    speedObject->cdfObject = cdfObject;
    speedObject->cdfVariable = dataSource->getDataObject(1)->cdfVariable->clone(CDF_FLOAT, SPEED_COMPONENT);
    speedObject->variableName.copy(speedObject->cdfVariable->name);

    speedObject->cdfVariable->setAttributeText("standard_name", "wind_speed");
    speedObject->cdfVariable->setAttributeText("long_name", "Wind speed");
    speedObject->cdfVariable->removeAttribute("short_name");
    speedObject->cdfVariable->setAttributeText("note", "Created by UVCOMPONENTS data post processor");
    speedObject->dataObjectName = CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_SPEED_COMPONENT;
    cdfObject->addVariable(speedObject->cdfVariable);

    CDataSource::DataObject *directionObject = dataSource->getDataObject(0)->clone();
    directionObject->cdfObject = cdfObject;
    directionObject->cdfVariable = dataSource->getDataObject(0)->cdfVariable->clone(CDF_FLOAT, DIRECTION_COMPONENT);
    directionObject->variableName.copy(directionObject->cdfVariable->name);
    directionObject->cdfVariable->setAttributeText("units", "degrees");
    directionObject->cdfVariable->setAttributeText("standard_name", "wind_direction");
    directionObject->cdfVariable->setAttributeText("long_name", "Wind direction");
    directionObject->cdfVariable->removeAttribute("short_name");
    directionObject->cdfVariable->setAttributeText("note", "Created by UVCOMPONENTS data post processor");
    directionObject->dataObjectName = CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_DIRECTION_COMPONENT;

    cdfObject->addVariable(directionObject->cdfVariable);

    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), vgridrel);
    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), ugridrel);
    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), directionObject);
    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), speedObject);
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    CImageWarper warper;
    warper.initreproj(dataSource, dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);

    int dPixelExtent[4];
    dPixelExtent[0] = 0;
    dPixelExtent[1] = 0;
    dPixelExtent[2] = dataSource->dWidth;
    dPixelExtent[3] = dataSource->dHeight;

    CDataSource::DataObject *ugridrel = dataSource->getDataObject(U_COMPONENT_GRID_ABSOLUTE);
    CDataSource::DataObject *vgridrel = dataSource->getDataObject(V_COMPONENT_GRID_ABSOLUTE);
    CDataSource::DataObject *speedObject = dataSource->getDataObject(SPEED_COMPONENT);
    CDataSource::DataObject *directionObject = dataSource->getDataObject(DIRECTION_COMPONENT);

    ugridrel->cdfVariable->copy(dataSource->getDataObject(CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_U_COMPONENT)->cdfVariable);
    vgridrel->cdfVariable->copy(dataSource->getDataObject(CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_V_COMPONENT)->cdfVariable);

    directionObject->cdfVariable->allocateData(ugridrel->cdfVariable->getSize());
    speedObject->cdfVariable->allocateData(ugridrel->cdfVariable->getSize());

    float *uValues = (float *)ugridrel->cdfVariable->data;
    float *vValues = (float *)vgridrel->cdfVariable->data;
    float *directionData = (float *)directionObject->cdfVariable->data;
    float *speedData = (float *)speedObject->cdfVariable->data;

    applyUVConversion(&warper, dataSource, dPixelExtent, uValues, vValues);

    size_t size = speedObject->cdfVariable->getSize();

    for (size_t j = 0; j < size; j++) {
      speedData[j] = sqrt(uValues[j] * uValues[j] + vValues[j] * vValues[j]);
      directionData[j] = ((atan2(uValues[j], vValues[j])) * (180 / M_PI) + 180);
    }
  }

  return 0;
}
