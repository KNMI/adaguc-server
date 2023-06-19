#include "CDataPostProcessor_SolarTerminator.h"

/************************/
/*      CDPPSolarTerminator  */
/************************/
const char *CDPPSolarTerminator::className = "CDPPSolarTerminator";

const char *CDPPSolarTerminator::getId() { return "solarterminator"; }

int CDPPSolarTerminator::terminator(double geox, double geoy, double epochtime) {}

int CDPPSolarTerminator::isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if (proc->attr.algorithm.equals("solarterminator")) {
    if (dataSource->getNumDataObjects() < 1 && mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
      CDBError("1 variable is needed for solarterminator, found %d", dataSource->getNumDataObjects());
      return CDATAPOSTPROCESSOR_CONSTRAINTSNOTMET;
    }
    return CDATAPOSTPROCESSOR_RUNAFTERREADING | CDATAPOSTPROCESSOR_RUNBEFOREREADING;
  }
  return CDATAPOSTPROCESSOR_NOTAPPLICABLE;
}

int CDPPSolarTerminator::execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode) {
  if ((isApplicable(proc, dataSource, mode) & mode) == false) {
    return -1;
  }
  CDBDebug("Applying SolarTerminator");
  if (mode == CDATAPOSTPROCESSOR_RUNBEFOREREADING) {
    CT::string newVariableName = proc->attr.name;
    if (newVariableName.empty()) {
      newVariableName = "SolT";
    }
    if (dataSource->getDataObject(0)->cdfVariable->name.equals(newVariableName.c_str())) return 0;

    CDataSource::DataObject *newDataObject = dataSource->getDataObject(0);
    newDataObject->variableName.copy(newVariableName.c_str());

    // Copy bounding box of screen
    auto *geo = dataSource->srvParams->Geo;
    double dfBBOX[] = {geo->dfBBOX[0], geo->dfBBOX[1], geo->dfBBOX[2], geo->dfBBOX[3]};
    size_t width = geo->dWidth;
    size_t height = geo->dHeight;
    dataSource->nativeProj4 = geo->CRS;
    dataSource->dWidth = geo->dWidth;
    dataSource->dHeight = geo->dHeight;
    dataSource->dfBBOX[0] = geo->dfBBOX[0];
    dataSource->dfBBOX[1] = geo->dfBBOX[1];
    dataSource->dfBBOX[2] = geo->dfBBOX[2];
    dataSource->dfBBOX[3] = geo->dfBBOX[3];
    /* Add geo variables, only if they are not there already */
    CDF::Dimension *dimX = newDataObject->cdfObject->getDimensionNE("xet");
    CDF::Dimension *dimY = newDataObject->cdfObject->getDimensionNE("yet");
    CDF::Variable *varX = newDataObject->cdfObject->getVariableNE("xet");
    CDF::Variable *varY = newDataObject->cdfObject->getVariableNE("yet");

    /* If not available, create new dimensions and variables (X,Y,T)*/
    if (dimX == NULL || dimY == NULL || varX == NULL || varY == NULL) {

      /* Define X dimension, with length 2, indicating the initial 2Dd Grid of 2x2 pixels */

      dimX = new CDF::Dimension();
      dimX->name = "xet";
      dimX->setSize(width);
      newDataObject->cdfObject->addDimension(dimX);

      /* Define the X variable using the X dimension */
      varX = new CDF::Variable();
      varX->setType(CDF_DOUBLE);
      varX->name.copy("xet");
      varX->isDimension = true;
      varX->dimensionlinks.push_back(dimX);
      newDataObject->cdfObject->addVariable(varX);
      CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);

      /* Set the bbox in the data, since the virtual grid is 2x2 pixels we can directly apply the bbox */
      ((double *)varX->data)[0] = dfBBOX[0];
      ((double *)varX->data)[1] = dfBBOX[2];

      /* For y dimension */
      dimY = new CDF::Dimension();
      dimY->name = "yet";
      dimY->setSize(height);
      newDataObject->cdfObject->addDimension(dimY);

      /* Define the Y variable using the X dimension */
      varY = new CDF::Variable();
      varY->setType(CDF_DOUBLE);
      varY->name.copy("yet");
      varY->isDimension = true;
      varY->dimensionlinks.push_back(dimY);
      newDataObject->cdfObject->addVariable(varY);
      CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

      ((double *)varY->data)[0] = dfBBOX[1];
      ((double *)varY->data)[1] = dfBBOX[3];

      /* Define the echotoppen variable using the defined dimensions, and set the right attributes */
      CDF::Variable *solTVar = new CDF::Variable();
      newDataObject->cdfObject->addVariable(solTVar);
      solTVar->setType(CDF_FLOAT);
      float fillValue[] = {-1};
      solTVar->setAttribute("_FillValue", solTVar->getType(), fillValue, 1);
      solTVar->dimensionlinks.push_back(dimY);
      solTVar->dimensionlinks.push_back(dimX);
      solTVar->setType(CDF_FLOAT);
      solTVar->name = "SolT";
      solTVar->addAttribute(new CDF::Attribute("units", "FL (ft*100)"));
      solTVar->setAttributeText("grid_mapping", "projection");
      newDataObject->cdfVariable = solTVar;

      newDataObject->cdfVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);

      newDataObject->cdfVariable->setSize(dataSource->dWidth * dataSource->dHeight);

      /* Make the width and height of the new 2D adaguc field the same as the viewing window */
      dataSource->dWidth = dataSource->srvParams->Geo->dWidth;
      dataSource->dHeight = dataSource->srvParams->Geo->dHeight;

      /* Width and height of the dataSource need to be at least 2 in this case. */
      if (dataSource->dWidth < 2) dataSource->dWidth = 2;
      if (dataSource->dHeight < 2) dataSource->dHeight = 2;

      /* Get the X and Y dimensions previousely defined and adjust them to the new settings and new grid (Grid in screenview space) */
      CDF::Dimension *dimX = newDataObject->cdfObject->getDimension("xet");
      dimX->setSize(dataSource->dWidth);

      CDF::Dimension *dimY = newDataObject->cdfObject->getDimension("yet");
      dimY->setSize(dataSource->dHeight);

      /* Get the X and Y variables from the cdfobject (previousely defined in the header function) */
      CDF::Variable *varX = newDataObject->cdfObject->getVariable("xet");
      CDF::Variable *varY = newDataObject->cdfObject->getVariable("yet");

      /* Re-allocate data for these coordinate variables with the new grid size */
      CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);
      CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

      /* Get the echotoppen variable from the datasource */
      CDF::Variable *echoToppenVar = dataSource->getDataObject(0)->cdfVariable;

      /* Calculate the gridsize, allocate data and fill the data with a fillvalue */
      size_t fieldSize = dimX->length * dimY->length;
      echoToppenVar->setSize(fieldSize);
      CDF::allocateData(echoToppenVar->getType(), &(echoToppenVar->data), fieldSize);
      CDF::fill(echoToppenVar->data, echoToppenVar->getType(), fillValue[0], fieldSize);

      /* Calculate cellsize and offset of the echo toppen (ET) 2D virtual grid, using the same grid as the screenspace*/
      double cellSizeETX = (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0]) / double(dataSource->dWidth);
      double cellSizeETY = (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1]) / double(dataSource->dHeight);
      double offsetETX = dataSource->srvParams->Geo->dfBBOX[0];
      double offsetETY = dataSource->srvParams->Geo->dfBBOX[1];

      /* Fill in the X and Y dimensions with the array of coordinates */
      for (size_t j = 0; j < dimX->length; j++) {
        double x = offsetETX + double(j) * cellSizeETX + cellSizeETX / 2;
        ((double *)varX->data)[j] = x;
      }
      for (size_t j = 0; j < dimY->length; j++) {
        double y = offsetETY + double(j) * cellSizeETY + cellSizeETY / 2;
        ((double *)varY->data)[j] = y;
      }
    }
  }
  if (mode == CDATAPOSTPROCESSOR_RUNAFTERREADING) {
    CDBDebug("CDATAPOSTPROCESSOR_RUNAFTERREADING::Applying SOLARTERMINATOR");
    CDBDebug("Applying SolarTerminator 1");
    size_t l = (size_t)dataSource->dHeight * (size_t)dataSource->dWidth;
    CDF::allocateData(dataSource->getDataObject(0)->cdfVariable->getType(), &dataSource->getDataObject(0)->cdfVariable->data, l);

    float *result = (float *)dataSource->getDataObject(0)->cdfVariable->data;

    CDBDebug("Applying SolarTerminator 2");

    for (size_t j = 0; j < l; j++) {
      int px = j % dataSource->dWidth;
      int py = j / dataSource->dWidth;
      // Projection coordinates (works in EPSG 4326)
      double geox = ((dataSource->dfBBOX[2] - dataSource->dfBBOX[0]) / dataSource->dWidth) * px + dataSource->dfBBOX[0];
      double geoy = ((dataSource->dfBBOX[1] - dataSource->dfBBOX[3]) / dataSource->dHeight) * py + dataSource->dfBBOX[3];
      // if not using lat/lon, reproject

      result[j] = geoy;
    }
  }
  // dataSource->eraseDataObject(1);
  return 0;
}