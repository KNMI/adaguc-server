#include "CDataPostProcessor_WFP.h"
#include "CRequest.h"
#include "CGenericDataWarper.h"
#include <utils/LayerUtils.h>

/************************/
/*      CDPPWFP  */
/************************/
const char *CDPPWFP::className = "CDPPWFP";

const char *CDPPWFP::getId() { return "WFP"; }
int CDPPWFP::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("WFP")) {
    if (dataSource->getNumDataObjects() == 1 && mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
      // if (dataSource->getNumDataObjects() != 2 && dataSource->getNumDataObjects() != 3 && dataSource->getNumDataObjects() != 4 && dataSource->getNumDataObjects() != 5) {
      CDBError("2 variables are needed for WFP, found %d", dataSource->getNumDataObjects());
      return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
    }
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

CDataSource *CDPPWFP::getDataSource(CDataSource *dataSource, CT::string baseLayerName) {
  CDataSource *tempDataSource = new CDataSource();
  size_t additionalLayerNo = 0;
  for (size_t j = 0; j < dataSource->srvParams->cfg->Layer.size(); j++) {
    CT::string layerName = makeUniqueLayerName(dataSource->srvParams->cfg->Layer[j]);
    if (baseLayerName.equals(layerName)) {
      additionalLayerNo = j;
      break;
    }
  }
  tempDataSource->setCFGLayer(dataSource->srvParams, dataSource->srvParams->configObj->Configuration[0], dataSource->srvParams->cfg->Layer[additionalLayerNo], baseLayerName.c_str(), 0);
  return tempDataSource;
}

CDF::Variable *CDPPWFP::cloneVariable(CDF::Variable *varToClone, const char *name, int size) {
  CDF::Variable *var = new CDF::Variable(name, CDF_FLOAT, NULL, 0, false);
  var->setSize(size);
  for (size_t j = 0; j < varToClone->dimensionlinks.size(); j++) var->dimensionlinks.push_back(varToClone->dimensionlinks[j]);
  for (size_t j = 0; j < varToClone->attributes.size(); j++) var->attributes.push_back(new CDF::Attribute(varToClone->attributes[j]));
  var->removeAttribute("scale_factor");
  var->removeAttribute("add_offset");
  var->setAttributeText("standard_name", name);
  var->setAttributeText("long_name", name);
  var->setAttributeText("units", "1");
  float fill = -1;

  var->setAttribute("_FillValue", var->getType(), &fill, 1);
  var->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);
  return var;
}

int CDPPWFP::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if ((isApplicable(proc, dataSource, mode) & mode) == false) {
    return -1;
  }
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    if (dataSource->getDataObject(0)->cdfVariable->name.equals("WindSpeedWindparksOff")) return 0;
    CDF::Variable *varToClone = dataSource->getDataObject(0)->cdfVariable;
    dataSource->getDataObject(1)->cdfVariable->setAttributeText("long_name", "WindSpeedWindparksOn");

    /* Prepend foundWindSectors data object */
    CDataSource::DataObject *founrWindSectorsDataObject = new CDataSource::DataObject();
    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), founrWindSectorsDataObject);
    founrWindSectorsDataObject->cdfVariable = cloneVariable(varToClone, "foundWindSector", dataSource->dWidth * dataSource->dHeight);
    founrWindSectorsDataObject->variableName.copy("foundWindSector");
    founrWindSectorsDataObject->cdfObject = (CDFObject *)varToClone->getParentCDFObject();
    founrWindSectorsDataObject->cdfObject->addVariable(founrWindSectorsDataObject->cdfVariable);

    /* Prepend windSpeedDifference data object */
    CDataSource::DataObject *windSpeedDifferenceDataObject = new CDataSource::DataObject();
    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), windSpeedDifferenceDataObject);
    windSpeedDifferenceDataObject->cdfVariable = cloneVariable(varToClone, "windSpeedDifference", dataSource->dWidth * dataSource->dHeight);
    windSpeedDifferenceDataObject->cdfVariable->setAttributeText("units", "kts");
    windSpeedDifferenceDataObject->variableName.copy("windSpeedDifference");
    windSpeedDifferenceDataObject->cdfObject = (CDFObject *)varToClone->getParentCDFObject();
    windSpeedDifferenceDataObject->cdfObject->addVariable(windSpeedDifferenceDataObject->cdfVariable);

    /* Prepend WindSpeedWindparksOnImproved data object */
    CDataSource::DataObject *correctedWindFieldMinDataObject = new CDataSource::DataObject();
    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), correctedWindFieldMinDataObject);
    correctedWindFieldMinDataObject->cdfVariable = cloneVariable(varToClone, "WindSpeedWindparksOnImproved", dataSource->dWidth * dataSource->dHeight);
    correctedWindFieldMinDataObject->variableName.copy("WindSpeedWindparksOnImproved");
    correctedWindFieldMinDataObject->cdfVariable->setAttributeText("units", "kts");
    correctedWindFieldMinDataObject->cdfObject = (CDFObject *)varToClone->getParentCDFObject();
    correctedWindFieldMinDataObject->cdfObject->addVariable(correctedWindFieldMinDataObject->cdfVariable);
    /* Prepend WindSpeedWindparksOff data object */
    CDataSource::DataObject *correctedWindFieldDataObject = new CDataSource::DataObject();
    dataSource->getDataObjectsVector()->insert(dataSource->getDataObjectsVector()->begin(), correctedWindFieldDataObject);
    correctedWindFieldDataObject->cdfVariable = cloneVariable(varToClone, "WindSpeedWindparksOff", dataSource->dWidth * dataSource->dHeight);
    correctedWindFieldDataObject->variableName.copy("WindSpeedWindparksOff");
    correctedWindFieldDataObject->cdfVariable->setAttributeText("units", "kts");
    correctedWindFieldDataObject->cdfObject = (CDFObject *)varToClone->getParentCDFObject();
    correctedWindFieldDataObject->cdfObject->addVariable(correctedWindFieldDataObject->cdfVariable);
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    CDF::allocateData(dataSource->getDataObject(0)->cdfVariable->getType(), &dataSource->getDataObject(0)->cdfVariable->data, l);

    // float *windDirection = (float *)dataSource->getDataObject(2)->cdfVariable->data;
    // float *windSpeed = (float *)dataSource->getDataObject(3)->cdfVariable->data;

    CDataSource *tempDataSource = getDataSource(dataSource, proc->attr.name);
    CRequest::setDimValuesForDataSource(tempDataSource, dataSource->srvParams);

    CDFObject *windSectorscdfObject = new CDFObject();
    CDFReader *windSectorscdfReader = new CDFNetCDFReader();

    windSectorscdfObject->attachCDFReader(windSectorscdfReader);
    int status = windSectorscdfReader->open(tempDataSource->getFileName());
    CDF::Variable *windSpeedDifferenceVariable = windSectorscdfReader->cdfObject->getVariable(tempDataSource->getDataObject(0)->variableName);
    CDF::Variable *windSectorX = windSectorscdfReader->cdfObject->getVariable("x");
    CDF::Variable *windSectorY = windSectorscdfReader->cdfObject->getVariable("y");
    CDF::Variable *windSectors = windSectorscdfReader->cdfObject->getVariable("wind_sector");
    windSectorX->readData(CDF_FLOAT);
    windSectorY->readData(CDF_FLOAT);
    windSectors->readData(CDF_INT);
    float *windSectorXData = (float *)windSectorX->data;
    float *windSectorYData = (float *)windSectorY->data;

    // This is the variable to write To
    CDF::Variable *WindSpeedWindparksOff = dataSource->getDataObjectByName("WindSpeedWindparksOff")->cdfVariable;
    windSpeedDifferenceVariable->readData(CDF_FLOAT);
    CDF::fill(WindSpeedWindparksOff->data, WindSpeedWindparksOff->getType(), -1, (size_t)dataSource->dHeight * (size_t)dataSource->dWidth);

    Settings settings;
    settings.width = dataSource->dWidth;
    settings.height = dataSource->dHeight;
    settings.WindSpeedWindparksOff = (float *)dataSource->getDataObject(0)->cdfVariable->data;        // Array / grid to write TO
    settings.WindSpeedWindparksOnImproved = (float *)dataSource->getDataObject(1)->cdfVariable->data; // Array / grid to write TO
    settings.windSpeedDifference = (float *)dataSource->getDataObject(2)->cdfVariable->data;          // Array / grid to write TO
    settings.windSectors = (float *)dataSource->getDataObject(3)->cdfVariable->data;                  // Array / grid to write TO
    settings.destGridWindDirection = (float *)dataSource->getDataObject(4)->cdfVariable->data;        // Original wind direction from the model
    settings.destGridWindSpeed = (float *)dataSource->getDataObject(5)->cdfVariable->data;            // Original windspeed from the model
    settings.windSpeedDifferenceVariable = windSpeedDifferenceVariable;                               // CDF Variable to read FROM
    settings.windSectorVariable = windSectors;                                                        // CDF Variable with the wind sector lookup table

    auto wpdimlinks = windSpeedDifferenceVariable->dimensionlinks;
    settings.dimWindSectorQuantile = wpdimlinks[1]->getSize();
    settings.dimWindSectorHeightLevel = wpdimlinks[2]->getSize();
    settings.dimWindSectorY = wpdimlinks[3]->getSize();
    settings.dimWindSectorX = wpdimlinks[4]->getSize();

    float *windSectorDataField = (float *)windSpeedDifferenceVariable->data; // Array / grid to read FROM

    CGeoParams sourceGeo;

    sourceGeo.dWidth = windSpeedDifferenceVariable->getDimension("x")->length;
    sourceGeo.dHeight = windSpeedDifferenceVariable->getDimension("y")->length;
    sourceGeo.dfBBOX[0] = windSectorXData[0];
    sourceGeo.dfBBOX[3] = windSectorYData[0];
    sourceGeo.dfBBOX[2] = windSectorXData[sourceGeo.dWidth - 1];
    sourceGeo.dfBBOX[1] = windSectorYData[sourceGeo.dHeight - 1];
    sourceGeo.dfCellSizeX = (sourceGeo.dfBBOX[2] - sourceGeo.dfBBOX[0]) / sourceGeo.dWidth;
    sourceGeo.dfCellSizeY = (sourceGeo.dfBBOX[1] - sourceGeo.dfBBOX[3]) / sourceGeo.dHeight;
    sourceGeo.CRS = "+proj=lcc +lat_1=52.500000 +lat_2=52.500000 +lat_0=52.500000 +lon_0=.000000 +k_0=1.0 +x_0=-92962.569890 +y_0=230385.198586 +a=6371229.000000 +b=6371229.000000"; // TODO

    CGeoParams destGeo;
    destGeo.dWidth = dataSource->dWidth;
    destGeo.dHeight = dataSource->dHeight;
    destGeo.dfBBOX[0] = dataSource->dfBBOX[0];
    destGeo.dfBBOX[1] = dataSource->dfBBOX[1];
    destGeo.dfBBOX[2] = dataSource->dfBBOX[2];
    destGeo.dfBBOX[3] = dataSource->dfBBOX[3];
    destGeo.dfCellSizeX = dataSource->dfCellSizeX;
    destGeo.dfCellSizeY = dataSource->dfCellSizeY;
    destGeo.CRS = dataSource->nativeProj4;

    CImageWarper warper;

    status = warper.initreproj(sourceGeo.CRS, &destGeo, &dataSource->srvParams->cfg->Projection);
    if (status != 0) {
      CDBError("Unable to initialize projection");
      return 1;
    }
    GenericDataWarper genericDataWarper;

    genericDataWarper.render<float>(&warper, windSectorDataField, &sourceGeo, &destGeo, &settings, &drawFunction);
  }

  return 0;
}

void CDPPWFP::drawFunction(int x, int y, float, void *_settings) {
  Settings *settings = (Settings *)_settings;
  if (x >= 0 && y >= 0 && x < (int)settings->width && y < (int)settings->height) {
    float windSpeed = settings->destGridWindSpeed[x + y * settings->width];
    float windDir = settings->destGridWindDirection[x + y * settings->width];

    // Determine the windsector dimension value based on the WINS50 winddirection grid value
    int selectedS = -1;
    for (size_t sectorIndex = 0; sectorIndex < settings->windSectorVariable->getSize(); sectorIndex++) {
      int windSector = ((int *)settings->windSectorVariable->data)[sectorIndex] - 15; // TODO 15
      int nexWindSector = windSector + 30;                                            // TODO 30
      if (windDir >= windSector && windDir < nexWindSector) {
        selectedS = sectorIndex;
        break;
      }
    }
    // If the sector was not found, set a NAN
    if (selectedS == -1) {
      ((float *)settings->WindSpeedWindparksOff)[x + y * settings->width] = NAN;
      return;
    }

    /* Now calculate the wind difference*/
    size_t numX = settings->dimWindSectorX;
    size_t numY = settings->dimWindSectorY;
    size_t numZ = settings->dimWindSectorHeightLevel;
    size_t numQ = settings->dimWindSectorQuantile;

    size_t selectedQ = 1; // Second quantile, which is currently 0.95
    size_t selectedH = 0; // Currently only the first (10 meter)
    size_t selectedX = settings->sourceDataPX;
    size_t selectedY = settings->sourceDataPY;
    size_t gridLocationPointer = selectedX + selectedY * numX;
    size_t windHeightPointer = selectedH * numY * numX;
    size_t windQuantilePointer = selectedQ * numZ * numY * numX;
    size_t windSectorPointer = selectedS * numQ * numZ * numY * numX;

    CDF::Variable *windSpeedDifferenceVariable = settings->windSpeedDifferenceVariable;

    float windSpeedDifference = ((float *)windSpeedDifferenceVariable->data)[gridLocationPointer + windHeightPointer + windQuantilePointer + windSectorPointer];
    float MSTOKTS = 2;
    float correctionFactor = 1.15;

    if (windSpeedDifference < 1) windSpeedDifference = 0;

    float windSpeedDifferenceKTS = windSpeedDifference * MSTOKTS;
    float windSpeedDifferenceMinKTS = windSpeedDifference * MSTOKTS * correctionFactor;
    ((float *)settings->windSectors)[x + y * settings->width] = selectedS;
    ((float *)settings->windSpeedDifference)[x + y * settings->width] = windSpeedDifferenceKTS;

    ((float *)settings->WindSpeedWindparksOff)[x + y * settings->width] = windSpeed + windSpeedDifferenceKTS;
    ((float *)settings->WindSpeedWindparksOnImproved)[x + y * settings->width] = windSpeed - windSpeedDifferenceMinKTS;
  }
};