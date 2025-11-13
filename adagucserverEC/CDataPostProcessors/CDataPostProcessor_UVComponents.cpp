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
      CDBError("2 variables are needed for convert_uv_components, found %d", dataSource->getNumDataObjects());
      return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
    }
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

void adjustCDFModel(CDataSource *dataSource) {

  auto cdfObject = dataSource->getDataObject(0)->cdfObject;
  auto globAttr = cdfObject->getAttributeNE(CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ID);
  if (globAttr != nullptr && (globAttr->getDataAsString().equals("metadata") || globAttr->getDataAsString().equals("applied"))) {
    return;
  }

  auto varUComponent = dataSource->getDataObject(0)->cdfVariable;
  auto varVComponent = dataSource->getDataObject(1)->cdfVariable;

  // Make dimensions the same if possible
  if (varUComponent->dimensionlinks.size() == varVComponent->dimensionlinks.size()) {
    bool isSame = true;
    for (size_t i = 0; i < varUComponent->dimensionlinks.size(); i += 1) {
      if (varUComponent->dimensionlinks[i]->getSize() != varVComponent->dimensionlinks[i]->getSize()) {
        isSame = false;
      }
    }
    if (isSame) {
      varVComponent->dimensionlinks.clear();
      varVComponent->dimensionlinks = varUComponent->dimensionlinks;
    }
  }

  CDF::Variable *ugridabsoluteCdfVariable = dataSource->getDataObject(0)->cdfVariable->clone(CDF_FLOAT, U_COMPONENT_GRID_ABSOLUTE);
  ugridabsoluteCdfVariable->setAttributeText("standard_name", "eastward_wind");
  ugridabsoluteCdfVariable->setAttributeText("long_name", "eastward_wind");
  ugridabsoluteCdfVariable->removeAttribute("short_name");
  ugridabsoluteCdfVariable->setAttributeText("note", "Created by convert_uv_components data post processor. This is absolute eastward wind.");
  cdfObject->addVariable(ugridabsoluteCdfVariable);

  CDF::Variable *vgridabsoluteCdfVariable = dataSource->getDataObject(1)->cdfVariable->clone(CDF_FLOAT, V_COMPONENT_GRID_ABSOLUTE);
  vgridabsoluteCdfVariable->setAttributeText("standard_name", "northward_wind");
  vgridabsoluteCdfVariable->setAttributeText("long_name", "northward_wind");
  vgridabsoluteCdfVariable->removeAttribute("short_name");
  vgridabsoluteCdfVariable->setAttributeText("note", "Created by convert_uv_components data post processor. This is absolute northward wind.");
  cdfObject->addVariable(vgridabsoluteCdfVariable);

  CDF::Variable *speedObjectCdfVariable = dataSource->getDataObject(0)->cdfVariable->clone(CDF_FLOAT, SPEED_COMPONENT);
  speedObjectCdfVariable->setAttributeText("standard_name", "wind_speed");
  speedObjectCdfVariable->setAttributeText("long_name", "Wind speed");
  speedObjectCdfVariable->removeAttribute("short_name");
  speedObjectCdfVariable->setAttributeText("note", "Created by convert_uv_components data post processor");
  cdfObject->addVariable(speedObjectCdfVariable);

  CDF::Variable *directionObjectCdfVariable = dataSource->getDataObject(0)->cdfVariable->clone(CDF_FLOAT, DIRECTION_COMPONENT);
  directionObjectCdfVariable->setAttributeText("units", "degrees");
  directionObjectCdfVariable->setAttributeText("standard_name", "wind_direction");
  directionObjectCdfVariable->setAttributeText("long_name", "Wind direction");
  directionObjectCdfVariable->removeAttribute("short_name");
  directionObjectCdfVariable->setAttributeText("note", "Created by convert_uv_components data post processor");
  cdfObject->addVariable(directionObjectCdfVariable);
  cdfObject->setAttributeText(CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ID, "metadata");
}

void addDataObject(CDataSource *dataSource) {
  if (dataSource->getNumDataObjects() == 6) {
    // CDBDebug("DataSource has already 6 dataObjects. Skipping further.");
    return;
  }

  dataSource->getDataObject(0)->dataObjectName = CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_U_COMPONENT;
  dataSource->getDataObject(1)->dataObjectName = CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_V_COMPONENT;

  auto cdfObject = dataSource->getDataObject(0)->cdfObject;
  CDataSource::DataObject *ugridabsolute = dataSource->getDataObject(0)->clone();
  ugridabsolute->noFurtherProcessing = true;
  ugridabsolute->cdfVariable = cdfObject->getVariable(U_COMPONENT_GRID_ABSOLUTE);
  ugridabsolute->variableName = U_COMPONENT_GRID_ABSOLUTE;
  ugridabsolute->dataObjectName = U_COMPONENT_GRID_ABSOLUTE;

  CDataSource::DataObject *vgridabsolute = dataSource->getDataObject(0)->clone();
  vgridabsolute->noFurtherProcessing = true;
  vgridabsolute->cdfVariable = cdfObject->getVariable(V_COMPONENT_GRID_ABSOLUTE);
  vgridabsolute->variableName = V_COMPONENT_GRID_ABSOLUTE;
  vgridabsolute->dataObjectName = V_COMPONENT_GRID_ABSOLUTE;

  CDataSource::DataObject *directionObject = dataSource->getDataObject(0)->clone();
  directionObject->noFurtherProcessing = true;
  directionObject->cdfVariable = cdfObject->getVariable(DIRECTION_COMPONENT);
  directionObject->variableName = DIRECTION_COMPONENT;
  directionObject->dataObjectName = DIRECTION_COMPONENT;
  directionObject->setUnits("degrees");

  CDataSource::DataObject *speedObject = dataSource->getDataObject(0)->clone();
  speedObject->noFurtherProcessing = true;
  speedObject->cdfVariable = cdfObject->getVariable(SPEED_COMPONENT);
  speedObject->variableName = SPEED_COMPONENT;
  speedObject->dataObjectName = SPEED_COMPONENT;

  dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), vgridabsolute);
  dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), ugridabsolute);
  dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), directionObject);
  dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), speedObject);
}

int CDDPUVComponents::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if ((isApplicable(proc, dataSource, mode) & mode) == false) {
    return -1;
  }
  adjustCDFModel(dataSource);
  addDataObject(dataSource);
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    CDBDebug("Applying CDDPUVComponents after reading");

    CImageWarper warper;
    if (dataSource->srvParams->geoParams.crs.empty()) {
      dataSource->srvParams->geoParams.crs = "EPSG:4236";
    }
    warper.initreproj(dataSource, dataSource->srvParams->geoParams, &dataSource->srvParams->cfg->Projection);

    int dPixelExtent[4];
    dPixelExtent[0] = 0;
    dPixelExtent[1] = 0;
    dPixelExtent[2] = dataSource->dWidth;
    dPixelExtent[3] = dataSource->dHeight;

    CDataSource::DataObject *ugridabsolute = dataSource->getDataObjectByName(U_COMPONENT_GRID_ABSOLUTE);
    CDataSource::DataObject *vgridabsolute = dataSource->getDataObjectByName(V_COMPONENT_GRID_ABSOLUTE);
    CDataSource::DataObject *speedObject = dataSource->getDataObjectByName(SPEED_COMPONENT);
    CDataSource::DataObject *directionObject = dataSource->getDataObjectByName(DIRECTION_COMPONENT);

    ugridabsolute->cdfVariable->copy(dataSource->getDataObjectByName(CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_U_COMPONENT)->cdfVariable);
    vgridabsolute->cdfVariable->copy(dataSource->getDataObjectByName(CDATAPOSTPROCESSOR_CDDPUVCOMPONENTS_ORG_V_COMPONENT)->cdfVariable);

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

int CDDPUVComponents::execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) {
  CDBError("Not implemented");
  return 1;
}
