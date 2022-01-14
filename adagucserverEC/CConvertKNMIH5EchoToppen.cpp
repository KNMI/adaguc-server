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
#include "CConvertADAGUCPoint.h"
#define CConvertKNMIH5EchoToppen_DEBUG

const char *CConvertKNMIH5EchoToppen::className = "CConvertKNMIH5EchoToppen";

int CConvertKNMIH5EchoToppen::checkIfKNMIH5EchoToppenFormat(CDFObject *cdfObject) {
  try {
    /* Check if the image1.statistics variable and stat_cell_number is set */
    cdfObject->getVariable("image1.statistics")->getAttribute("stat_cell_number");
    cdfObject->getVariable("geographic")->getAttribute("geo_product_corners");
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

  CDBDebug("Starting convertKNMIH5EchoToppenHeader");

  // Default size of adaguc 2d Field is 2x2
  int width = 2;
  int height = 2;

  // Deterimine product corners based on file metadata:

  CT::string geo_product_corners = cdfObject->getVariable("geographic")->getAttribute("geo_product_corners")->getDataAsString();
  CT::StackList<CT::stringref> cell_max = geo_product_corners.splitToStackReferences(" ");

  double minX = 1000, maxX = -1000, minY = 1000, maxY = -1000;
  for (size_t j = 0; j < 4; j++) {
    double x = (CT::string(cell_max[j * 2].c_str())).toDouble();
    double y = (CT::string(cell_max[j * 2 + 1].c_str())).toDouble();
    if (minX > x) minX = x;
    if (minY > y) minY = y;
    if (maxX < x) maxX = x;
    if (maxY < y) maxY = y;
  }
  double dfBBOX[] = {minX, minY, maxX, maxY};

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

  CDF::Dimension *dimT = cdfObject->getDimensionNE("time");

  CDF::Variable *echoToppenVar = new CDF::Variable();
  cdfObject->addVariable(echoToppenVar);

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
  echoToppenVar->setAttributeText("grid_mapping", "projection");
  return 0;
}

int calcFlightLevel(float height) {
  float feet = height * 1000 * 3.281f;
  int roundedFeet = ((int)(feet / 1000 + 0.5) * 10);
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
    return 1;
  }

#ifdef CConvertKNMIH5EchoToppen_DEBUG
  CDBDebug("ConvertKNMIH5EchoToppenData");
#endif

  // CDF::Variable *echoToppenVar = dataSource->getDataObject(0)->cdfVariable;

  CImageWarper imageWarper;
  imageWarper.decodeCRS(&dataSource->nativeProj4, &dataSource->nativeEPSG, &dataSource->srvParams->cfg->Projection);

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
    CDF::Variable *echoToppenVar = dataSource->getDataObject(0)->cdfVariable;
    size_t fieldSize = dimX->length * dimY->length;
    echoToppenVar->setSize(fieldSize);
    CDF::allocateData(echoToppenVar->getType(), &(echoToppenVar->data), fieldSize);

    // Fill in the X and Y dimensions with the array of coordinates
    for (size_t j = 0; j < dimX->length; j++) {
      double x = offsetX + double(j) * cellSizeX + cellSizeX / 2;
      ((double *)varX->data)[j] = x;
    }
    for (size_t j = 0; j < dimY->length; j++) {
      double y = offsetY + double(j) * cellSizeY + cellSizeY / 2;
      ((double *)varY->data)[j] = y;
    }

    double cellSizeX = (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0]) / double(dataSource->dWidth);
    double cellSizeY = (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1]) / double(dataSource->dHeight);
    double offsetX = dataSource->srvParams->Geo->dfBBOX[0];
    double offsetY = dataSource->srvParams->Geo->dfBBOX[1];
    CImageWarper imageWarper;

    imageWarper.initreproj(dataSource, dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);

    CImageWarper imageWarperEchoToppen;
    CT::string projectionString = cdfObject0->getVariable("projection")->getAttribute("proj4_params")->toString();
    imageWarperEchoToppen.initreproj(projectionString.c_str(), dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);

    float left = cdfObject0->getVariable("x")->getDataAt<float>(0);
    float right = cdfObject0->getVariable("x")->getDataAt<float>(cdfObject0->getVariable("x")->getSize() - 1);
    float bottom = cdfObject0->getVariable("y")->getDataAt<float>(0);
    float top = cdfObject0->getVariable("y")->getDataAt<float>(cdfObject0->getVariable("y")->getSize() - 1);
    float cellsizeX = (right - left) / float(cdfObject0->getVariable("x")->getSize());
    float cellsizeY = (bottom - top) / float(cdfObject0->getVariable("y")->getSize());

    int stat_cell_number;
    cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_number")->getData(&stat_cell_number, 1);
    CT::string stat_cell_column = cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_column")->getDataAsString();
    CT::string stat_cell_row = cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_row")->getDataAsString();
    CT::string stat_cell_max = cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_max")->getDataAsString();
    CT::StackList<CT::stringref> cell_max = stat_cell_max.splitToStackReferences(" ");
    CT::StackList<CT::stringref> cell_column = stat_cell_column.splitToStackReferences(" ");
    CT::StackList<CT::stringref> cell_row = stat_cell_row.splitToStackReferences(" ");

    for (size_t k = 0; k < (size_t)stat_cell_number; k++) {
      /* Echotoppen grid coordinates / row and col */
      double col = CT::string(cell_column[k].c_str()).toDouble();
      double row = cdfObject0->getVariable("y")->getSize() - CT::string(cell_row[k].c_str()).toDouble();
      float v = calcFlightLevel(CT::string(cell_max[k].c_str()).toFloat());

      double h5X = col / cellsizeX + left;
      double h5Y = row / cellsizeY + top;
      imageWarperEchoToppen.reprojpoint_inv(h5X, h5Y);

      int dlon = int((h5X - offsetX) / cellSizeX);
      int dlat = int((h5Y - offsetY) / cellSizeY);

      dataSource->dataObjects[0]->points.push_back(PointDVWithLatLon(dlon, dlat, h5X, h5Y, v));
      CConvertADAGUCPoint::drawDot(dlon, dlat, v, dimX->length, dimY->length, (float *)echoToppenVar->data);
    }
  }
  return 0;
}
