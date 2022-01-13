/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Geo Spatial Team gstf@knmi.nl
 * Date:     2022-01-12
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include "CConvertKNMIH5EchoToppen.h"
#include "CFillTriangle.h"
#include "CImageWarper.h"
#define CConvertKNMIH5EchoToppen_DEBUG

const char *CConvertKNMIH5EchoToppen::className = "CConvertKNMIH5EchoToppen";

int CConvertKNMIH5EchoToppen::checkIfKNMIH5EchoToppenFormat(CDFObject *cdfObject) {
  try {
    /* Check if the image1.statistics variable and stat_cell_number is set */
    cdfObject->getVariable("image1.statistics")->getAttribute("stat_cell_number");
    /* TODO: Return 1 in case it is not a point renderer*/
  } catch (int e) {
    return 1;
  }
  return 0;
}

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertKNMIH5EchoToppen::convertKNMIH5EchoToppenHeader(CDFObject *cdfObject) {
  // Check whether this is really an KNMIH5EchoToppenFormat file
  if (CConvertKNMIH5EchoToppen::checkIfKNMIH5EchoToppenFormat(cdfObject) == 1) return 1;
  CDBDebug("Using CConvertKNMIH5EchoToppen.h");
  // Default size of adaguc 2dField is 2x2
  int width = 2;
  int height = 2;


  double dfBBOX[] = {-180, -90, 180, 90};
  CDF::Variable *xVar = cdfObject->getVariableNE("x");
  CDF::Dimension *xDim = cdfObject->getDimensionNE("x");
  CDF::Variable *yVar = cdfObject->getVariableNE("y");
  CDF::Dimension *yDim = cdfObject->getDimensionNE("y");
  if ((xVar!=NULL)&&(yVar!=NULL)) {
    int w = xDim->getSize();
    int h = yDim->getSize();
    double llLon, llLat, urLon, urLat;
    llLon = xVar->getDataAt<double>(0);
    llLat = yVar->getDataAt<double>(0);
    urLon = xVar->getDataAt<double>(w-1);
    urLat = yVar->getDataAt<double>(h-1);
    if (llLat>urLat) {
      double swap=llLat;
      llLat=urLat;
      urLat=swap;
    }
    dfBBOX[0]=llLon; dfBBOX[1]=llLat;
    dfBBOX[2]=urLon; dfBBOX[3]=urLat;
  }
  CDBDebug("dfBBOX: %f %f %f %f", dfBBOX[0], dfBBOX[1], dfBBOX[2], dfBBOX[3]);

  double cellSizeX = (dfBBOX[2] - dfBBOX[0]) / double(width);
  double cellSizeY = (dfBBOX[3] - dfBBOX[1]) / double(height);
  double offsetX = dfBBOX[0];
  double offsetY = dfBBOX[1];
  // Add geo variables, only if they are not there already
  CDF::Dimension *dimX = cdfObject->getDimensionNE("xet");
  CDF::Dimension *dimY = cdfObject->getDimensionNE("yet");

  CDF::Variable *varX = cdfObject->getVariableNE("xet");
  CDF::Variable *varY = cdfObject->getVariableNE("yet");
  if (dimX == NULL || dimY == NULL || varX == NULL || varY == NULL) {
    // If not available, create new dimensions and variables (X,Y,T)
    // For x
    dimX = new CDF::Dimension();
    dimX->name = "xet";
    dimX->setSize(width);
    cdfObject->addDimension(dimX);
    varX = new CDF::Variable();
    varX->setType(CDF_DOUBLE);
    varX->name.copy("xet");
    varX->isDimension = true;
    varX->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(varX);
    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);

    // For y
    dimY = new CDF::Dimension();
    dimY->name = "yet";
    dimY->setSize(height);
    cdfObject->addDimension(dimY);
    varY = new CDF::Variable();
    varY->setType(CDF_DOUBLE);
    varY->name.copy("yet");
    varY->isDimension = true;
    varY->dimensionlinks.push_back(dimY);
    cdfObject->addVariable(varY);
    CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

    // Fill in the X and Y dimensions with the array of coordinates
    for (size_t j = 0; j < dimX->length; j++) {
      double x = offsetX + double(j) * cellSizeX + cellSizeX / 2;
      ((double *)varX->data)[j] = x;
    }
    for (size_t j = 0; j < dimY->length; j++) {
      double y = offsetY + double(j) * cellSizeY + cellSizeY / 2;
      ((double *)varY->data)[j] = y;
    }
  }

  CDF::Variable *echoToppenVar = new CDF::Variable();
  cdfObject->addVariable(echoToppenVar);

  CDF::Dimension *dimT;
  CDF::Variable *eth = cdfObject->getVariable("image1.image_data");
  for (size_t d=0; d<eth->dimensionlinks.size(); d++) {
    if (eth->dimensionlinks[d]->name.equals("time")) {
      dimT = eth->dimensionlinks[d];
    }
  }
  // // Assign X,Y,T dims

  // for (size_t d = 0; d < pointVar->dimensionlinks.size(); d++) {
  //   bool skip = false;
  //   if (pointVar->dimensionlinks[d]->name.equals("station") == true) {
  //     skip = true;
  //   }
  //   if (!skip) {
  //     echoToppenVar->dimensionlinks.push_back(pointVar->dimensionlinks[d]);
  //   }
  // }

  echoToppenVar->dimensionlinks.push_back(dimT);
  echoToppenVar->dimensionlinks.push_back(dimY);
  echoToppenVar->dimensionlinks.push_back(dimX);
  echoToppenVar->setType(CDF_FLOAT);

  echoToppenVar->name = "echotoppen";
  echoToppenVar->addAttribute(new CDF::Attribute("units", "FL (ft*100)"));
  return 0;
}

int calcFlightLevel(float height) {
    float feet=height*1000*3.281f;
    int roundedFeet=((int )(feet/1000+0.5)*10);
    return roundedFeet;
}

/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertKNMIH5EchoToppen::convertKNMIH5EchoToppenData(CDataSource *dataSource, int mode) {
  CDFObject *cdfObject0 = dataSource->getDataObject(0)->cdfObject;
  if (CConvertKNMIH5EchoToppen::checkIfKNMIH5EchoToppenFormat(cdfObject0) == 1) return 1;
  if (!dataSource->getDataObject(0)->cdfVariable->name.equals("echotoppen")) {
    CDBDebug("Skipping convertKNMIH5EchoToppenData");
    return 0;
  }
#ifdef CConvertKNMIH5EchoToppen_DEBUG
  CDBDebug("ConvertKNMIH5EchoToppenData");
#endif

  {
    CDF::Variable *echoToppenVar = dataSource->getDataObject(0)->cdfVariable;
    echoToppenVar->setAttributeText("grid_mapping", "projection");
    CDBDebug("CRS:%s", dataSource->srvParams->Geo->CRS.c_str());
    CImageWarper imageWarper;
    imageWarper.decodeCRS(&dataSource->nativeProj4, &dataSource->nativeEPSG, &dataSource->srvParams->cfg->Projection);
    // image1.image_data:grid_mapping = "projection" ;
    CDBDebug("nativeProj4: %s nativeEPSG: %s", dataSource->nativeProj4.c_str(), dataSource->nativeEPSG.c_str() );
  }
  // Make the width and height of the new 2D adaguc field the same as the viewing window
  dataSource->dWidth = dataSource->srvParams->Geo->dWidth;
  dataSource->dHeight = dataSource->srvParams->Geo->dHeight;

  if (dataSource->dWidth == 1 && dataSource->dHeight == 1) {
    dataSource->srvParams->Geo->dfBBOX[0] = dataSource->srvParams->Geo->dfBBOX[0];
    dataSource->srvParams->Geo->dfBBOX[1] = dataSource->srvParams->Geo->dfBBOX[1];
    dataSource->srvParams->Geo->dfBBOX[2] = dataSource->srvParams->Geo->dfBBOX[2];
    dataSource->srvParams->Geo->dfBBOX[3] = dataSource->srvParams->Geo->dfBBOX[3];
  }

  // Width needs to be at least 2 in this case.
  if (dataSource->dWidth == 1) dataSource->dWidth = 2;
  if (dataSource->dHeight == 1) dataSource->dHeight = 2;
  double cellSizeX = (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0]) / double(dataSource->dWidth);
  double cellSizeY = (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1]) / double(dataSource->dHeight);
  double offsetX = dataSource->srvParams->Geo->dfBBOX[0];
  double offsetY = dataSource->srvParams->Geo->dfBBOX[1];

  if (mode == CNETCDFREADER_MODE_OPEN_ALL) {

#ifdef CCONVERTADAGUCPOINT_DEBUG
    for (size_t d = 0; d < nrDataObjects; d++) {
      if (pointVar[d] != NULL) {
        CDBDebug("Drawing %s", new2DVar[d]->name.c_str());
      }
    }
#endif

    CDF::Dimension *dimX;
    CDF::Dimension *dimY;
    CDF::Variable *varX;
    CDF::Variable *varY;

    // Create new dimensions and variables (X,Y,T)
    dimX = cdfObject0->getDimension("xet");
    dimX->setSize(dataSource->dWidth);

    dimY = cdfObject0->getDimension("yet");
    dimY->setSize(dataSource->dHeight);

    varX = cdfObject0->getVariable("xet");
    varY = cdfObject0->getVariable("yet");

    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);
    CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

    // Fill in the X and Y dimensions with the array of coordinates
    for (size_t j = 0; j < dimX->length; j++) {
      double x = offsetX + double(j) * cellSizeX + cellSizeX / 2;
      ((double *)varX->data)[j] = x;
    }
    for (size_t j = 0; j < dimY->length; j++) {
      double y = offsetY + double(j) * cellSizeY + cellSizeY / 2;
      ((double *)varY->data)[j] = y;
    }
  }

  if (mode == CNETCDFREADER_MODE_OPEN_ALL) {
    double cellSizeX = (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0]) / double(dataSource->dWidth);
    double cellSizeY = (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1]) / double(dataSource->dHeight);
    double offsetX = dataSource->srvParams->Geo->dfBBOX[0];
    double offsetY = dataSource->srvParams->Geo->dfBBOX[1];
    CImageWarper imageWarper;

    imageWarper.initreproj(dataSource, dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);

    CImageWarper imageWarperEchoToppen;
    CT::string projectionString = cdfObject0->getVariable("projection")->getAttribute("proj4_params")->toString();
    CDBDebug("projectionString %s", projectionString.c_str());
    imageWarperEchoToppen.initreproj(projectionString.c_str(), dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);

    CDBDebug("dataSource->srvParams->Geo->dfBBOX left %f", dataSource->srvParams->Geo->dfBBOX[0]);
    CDBDebug("dataSource->srvParams->Geo->dfBBOX bottom %f", dataSource->srvParams->Geo->dfBBOX[1]);
    CDBDebug("dataSource->srvParams->Geo->dfBBOX right %f", dataSource->srvParams->Geo->dfBBOX[2]);
    CDBDebug("dataSource->srvParams->Geo->dfBBOX top %f", dataSource->srvParams->Geo->dfBBOX[3]);
    CDBDebug("dataSource->dWidth %d", dataSource->dWidth);
    CDBDebug("dataSource->dHeight %d", dataSource->dHeight);

    float left = cdfObject0->getVariable("x")->getDataAt<float>(0);
    float right = cdfObject0->getVariable("x")->getDataAt<float>(cdfObject0->getVariable("x")->getSize() - 1);
    float bottom = cdfObject0->getVariable("y")->getDataAt<float>(0);
    float top = cdfObject0->getVariable("y")->getDataAt<float>(cdfObject0->getVariable("y")->getSize() - 1);
    float cellsizeX = (right - left) / float(cdfObject0->getVariable("x")->getSize());
    float cellsizeY = (bottom - top) / float(cdfObject0->getVariable("y")->getSize());
    CDBDebug("HDFGrid left %f and %f cellsize %f %f", left, right, cellsizeX, cellsizeY);

    int stat_cell_number;
    cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_number")->getData(&stat_cell_number, 1);
    CT::string stat_cell_column=cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_column")->getDataAsString();
    CT::string stat_cell_row=cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_row")->getDataAsString();
    CT::string stat_cell_max=cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_max")->getDataAsString();
    CT::StackList<CT::stringref>cell_max=stat_cell_max.splitToStackReferences(" ");
    CT::StackList<CT::stringref>cell_column=stat_cell_column.splitToStackReferences(" ");
    CT::StackList<CT::stringref>cell_row=stat_cell_row.splitToStackReferences(" ");

    for (size_t k = 0; k<(size_t)stat_cell_number; k++) {
        CDBDebug("%d: %s", k, cell_column[k].c_str(), cell_row[k].c_str());
      /* Echotoppen grid coordinates / row and col */
      double col = CT::string(cell_column[k].c_str()).toDouble();
      double row = CT::string(cell_row[k].c_str()).toDouble();
      float v = calcFlightLevel(CT::string(cell_max[k].c_str()).toFloat());
      CDBDebug("%d [%f,%f]: %f", k, col, row, v);

      /* Put something at the corners for check */
    //   if (k < 4) {
    //     if (k == 0 || k == 1) {
    //       row = 0;
    //     } else {
    //       row = 764;
    //     }
    //     if (k == 0 || k == 2) {
    //       col = 0;
    //     } else {
    //       col = 699;
    //     }
    //   }

      /* Echotoppen projection coordinates */
      // CDBDebug("grid coords: h5X, h5Y %f, %f", col, row);
      double h5X = col / cellsizeX + left;
      double h5Y = row / cellsizeY + top;
      // CDBDebug("proj coords: h5X, h5Y %f, %f", h5X, h5Y);
      imageWarperEchoToppen.reprojpoint_inv(h5X, h5Y);
      // CDBDebug("h5X, h5Y %f, %f", h5X, h5Y);

      // imageWarper.reprojfromLatLon(h5X, h5Y);

      int dlon = int((h5X - offsetX) / cellSizeX);
      int dlat = int((h5Y - offsetY) / cellSizeY);

      CDBDebug("dlon %d dlat %d", dlon, dlat);

      dataSource->dataObjects[0]->points.push_back(PointDVWithLatLon(dlon, dlat, h5X, h5Y, v));
    }
    CDBDebug("points: %d", dataSource->dataObjects[0]->points.size());
  }
  return 0;
}
