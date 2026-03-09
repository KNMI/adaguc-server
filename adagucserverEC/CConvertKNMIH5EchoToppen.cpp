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

#define CConvertKNMIH5EchoToppen_FillValue -1
#define CConvertKNMIH5EchoToppen_EchoToppenVar "echotops"

int CConvertKNMIH5EchoToppen::calcFlightLevel(float height) {
  float feet = height * 1000 * 3.281f;
  int roundedFeet = ((int)(feet / 1000 + 0.5) * 10);
  return roundedFeet;
}

int CConvertKNMIH5EchoToppen::checkIfKNMIH5EchoToppenFormat(CDFObject *cdfObject) {
  try {
    /* Check if the image1.statistics variable and stat_cell_number attribute is set */
    cdfObject->getVariable("image1.statistics")->getAttribute("stat_cell_number");
    /* Check if the geographic variable and geo_product_corners attribute is set */
    cdfObject->getVariable("geographic")->getAttribute("geo_product_corners");
  } catch (int e) {
    return 1;
  }
  return 0;
}

int CConvertKNMIH5EchoToppen::convertKNMIH5EchoToppenHeader(CDFObject *cdfObject) {
  /* Check whether this is really an KNMIH5EchoToppenFormat file */
  if (CConvertKNMIH5EchoToppen::checkIfKNMIH5EchoToppenFormat(cdfObject) == 1) return 1;

  /* Default size of adaguc 2d Field is 2x2 */
  int width = 2;
  int height = 2;

  /* Deterine product corners based on file metadata: */
  CT::string geo_product_corners = cdfObject->getVariable("geographic")->getAttribute("geo_product_corners")->getDataAsString();
  std::vector<CT::string> cell_max = geo_product_corners.split(" ");

  /* Figure out outer biggest bbox based on the 4 coordinate values */
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

  /* Add geo variables, only if they are not there already */
  CDF::Dimension *dimX = cdfObject->getDimensionNE("xet");
  CDF::Dimension *dimY = cdfObject->getDimensionNE("yet");
  CDF::Variable *varX = cdfObject->getVariableNE("xet");
  CDF::Variable *varY = cdfObject->getVariableNE("yet");

  /* If not available, create new dimensions and variables (X,Y,T)*/
  if (dimX == NULL || dimY == NULL || varX == NULL || varY == NULL) {

    /* Define X dimension, with length 2, indicating the initial 2Dd Grid of 2x2 pixels */
    dimX = new CDF::Dimension();
    dimX->name = "xet";
    dimX->setSize(width);
    cdfObject->addDimension(dimX);

    /* Define the X variable using the X dimension */
    varX = new CDF::Variable();
    varX->setType(CDF_DOUBLE);
    varX->name.copy("xet");
    varX->isDimension = true;
    varX->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(varX);
    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);

    /* Set the bbox in the data, since the virtual grid is 2x2 pixels we can directly apply the bbox */
    ((double *)varX->data)[0] = dfBBOX[0];
    ((double *)varX->data)[1] = dfBBOX[2];

    /* For y dimension */
    dimY = new CDF::Dimension();
    dimY->name = "yet";
    dimY->setSize(height);
    cdfObject->addDimension(dimY);

    /* Define the Y variable using the X dimension */
    varY = new CDF::Variable();
    varY->setType(CDF_DOUBLE);
    varY->name.copy("yet");
    varY->isDimension = true;
    varY->dimensionlinks.push_back(dimY);
    cdfObject->addVariable(varY);
    CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

    ((double *)varY->data)[0] = dfBBOX[1];
    ((double *)varY->data)[1] = dfBBOX[3];

    /* Use the same time dimension */
    CDF::Dimension *dimT = cdfObject->getDimensionNE("time");

    /* Define the echotoppen variable using the defined dimensions, and set the right attributes */
    CDF::Variable *echoToppenVar = new CDF::Variable();
    cdfObject->addVariable(echoToppenVar);
    echoToppenVar->setType(CDF_FLOAT);
    float fillValue[] = {CConvertKNMIH5EchoToppen_FillValue};
    echoToppenVar->setAttribute("_FillValue", echoToppenVar->getType(), fillValue, 1);
    echoToppenVar->setAttributeText("ADAGUC_POINT", "true");
    echoToppenVar->dimensionlinks.push_back(dimT);
    echoToppenVar->dimensionlinks.push_back(dimY);
    echoToppenVar->dimensionlinks.push_back(dimX);
    echoToppenVar->setType(CDF_FLOAT);
    echoToppenVar->name = CConvertKNMIH5EchoToppen_EchoToppenVar;
    echoToppenVar->addAttribute(new CDF::Attribute("units", "FL"));
    echoToppenVar->setAttributeText("grid_mapping", "projection");
  }
  return 0;
}

/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertKNMIH5EchoToppen::convertKNMIH5EchoToppenData(CDataSource *dataSource, int mode) {
  /* Obtain a pointer to the first cdfobject */
  CDFObject *cdfObject0 = dataSource->getDataObject(0)->cdfObject;

  /* Check if we should continue */
  if (CConvertKNMIH5EchoToppen::checkIfKNMIH5EchoToppenFormat(cdfObject0) == 1) return 1;

  /* In case echotoppen is not defined in the datasource, we should do nothing otherwise we might mess up the actual image data request */
  if (!dataSource->getDataObject(0)->cdfVariable->name.equals(CConvertKNMIH5EchoToppen_EchoToppenVar)) {
    CDBDebug("Skipping convertKNMIH5EchoToppenData");
    return 1;
  }

#ifdef CConvertKNMIH5EchoToppen_DEBUG
  CDBDebug("ConvertKNMIH5EchoToppenData");
#endif
  /* Only handled if the datareader also wants to read the actual data */
  if (mode == CNETCDFREADER_MODE_OPEN_ALL) {

    /* Make the width and height of the new 2D adaguc field the same as the viewing window */
    dataSource->dWidth = dataSource->srvParams->geoParams.width;
    dataSource->dHeight = dataSource->srvParams->geoParams.height;

    /* Width and height of the dataSource need to be at least 2 in this case. */
    if (dataSource->dWidth < 2) dataSource->dWidth = 2;
    if (dataSource->dHeight < 2) dataSource->dHeight = 2;

    /* Get the X and Y dimensions previousely defined and adjust them to the new settings and new grid (Grid in screenview space) */
    CDF::Dimension *dimX = cdfObject0->getDimension("xet");
    dimX->setSize(dataSource->dWidth);

    CDF::Dimension *dimY = cdfObject0->getDimension("yet");
    dimY->setSize(dataSource->dHeight);

    /* Get the X and Y variables from the cdfobject (previousely defined in the header function) */
    CDF::Variable *varX = cdfObject0->getVariable("xet");
    CDF::Variable *varY = cdfObject0->getVariable("yet");

    /* Re-allocate data for these coordinate variables with the new grid size */
    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);
    CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

    /* Get the echotoppen variable from the datasource */
    CDF::Variable *echoToppenVar = dataSource->getDataObject(0)->cdfVariable;

    /* Calculate the gridsize, allocate data and fill the data with a fillvalue */
    size_t fieldSize = dimX->length * dimY->length;
    echoToppenVar->setSize(fieldSize);
    CDF::allocateData(echoToppenVar->getType(), &(echoToppenVar->data), fieldSize);
    float fillValue = CConvertKNMIH5EchoToppen_FillValue;
    CDF::fill(echoToppenVar->data, echoToppenVar->getType(), fillValue, fieldSize);

    /* Calculate cellsize and offset of the echo toppen (ET) 2D virtual grid, using the same grid as the screenspace*/
    double cellSizeETX = dataSource->srvParams->geoParams.bbox.span().x / dataSource->dWidth;
    double cellSizeETY = dataSource->srvParams->geoParams.bbox.span().y / dataSource->dHeight;
    double offsetETX = dataSource->srvParams->geoParams.bbox.left;
    double offsetETY = dataSource->srvParams->geoParams.bbox.bottom;

    /* Fill in the X and Y dimensions with the array of coordinates */
    for (size_t j = 0; j < dimX->length; j++) {
      double x = offsetETX + double(j) * cellSizeETX + cellSizeETX / 2;
      ((double *)varX->data)[j] = x;
    }
    for (size_t j = 0; j < dimY->length; j++) {
      double y = offsetETY + double(j) * cellSizeETY + cellSizeETY / 2;
      ((double *)varY->data)[j] = y;
    }

    /* We now need the extent/bbox of the HDF5 image data itself. The echotoppen data is referenced in respect to this grid/projection */
    float top = cdfObject0->getVariable("y")->getDataAt<float>(cdfObject0->getVariable("y")->getSize() - 1);
    float left = cdfObject0->getVariable("x")->getDataAt<float>(0);
    float right = cdfObject0->getVariable("x")->getDataAt<float>(cdfObject0->getVariable("x")->getSize() - 1);
    float bottom = cdfObject0->getVariable("y")->getDataAt<float>(0);

    /* Define shorthand to bbox and cellsize of the HDF5 data grid */
    float fBBOXIM[] = {left, bottom, right, top};
    float cellSizeIMX = (fBBOXIM[2] - fBBOXIM[0]) / float(cdfObject0->getVariable("x")->getSize());
    float cellsizeIMY = (fBBOXIM[3] - fBBOXIM[1]) / float(cdfObject0->getVariable("y")->getSize());

    /* Now get the colums and rows as defined in the metadata attributes of the HDF5 file */
    int stat_cell_number = 0;
    cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_number")->getData(&stat_cell_number, 1);
    CT::string stat_cell_column = cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_column")->getDataAsString();
    CT::string stat_cell_row = cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_row")->getDataAsString();
    CT::string stat_cell_max = cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_max")->getDataAsString();

    /* Split based on whitespace character */
    std::vector<CT::string> cell_max = stat_cell_max.split(" ");
    std::vector<CT::string> cell_column = stat_cell_column.split(" ");
    std::vector<CT::string> cell_row = stat_cell_row.split(" ");

    /* Time to instantiate the imagewarper. This is needed to project from HDF5 projection space (polar sterographic) to screenspace and latlon coordinate space*/
    CImageWarper imageWarperEchoToppen;
    CT::string projectionString = cdfObject0->getVariable("projection")->getAttribute("proj4_params")->toString();

    imageWarperEchoToppen.initreproj(projectionString.c_str(), dataSource->srvParams->geoParams, &dataSource->srvParams->cfg->Projection);
    double axisScaling;
    std::tie(std::ignore, axisScaling) = imageWarperEchoToppen.fixProjection(projectionString);

    /* Now loop through all found rows and cols */
    for (size_t k = 0; k < (size_t)stat_cell_number; k++) {

      /* Echotoppen grid coordinates / row and col */
      double col = CT::string(cell_column[k].c_str()).toDouble();
      double row = CT::string(cell_row[k].c_str()).toDouble();

      /* Calculate the flight level based on the HDF5 echotoppen value */
      float v = calcFlightLevel(CT::string(cell_max[k].c_str()).toFloat());

      /* Echotoppen coordinate in HDF5 projection space (Polar Stereographic) */
      double h5X = col / cellSizeIMX + fBBOXIM[0];
      double h5Y = row / cellsizeIMY + fBBOXIM[1];

      h5X *= axisScaling;
      h5Y *= axisScaling;

      /* Convert from HDF5 projection to requested coordinate system (in the WMS request)  */
      imageWarperEchoToppen.reprojpoint_inv(h5X, h5Y);

      /* The request coordinate space needs to be transformed to pixel screenview space */
      int dlon = int((h5X - offsetETX) / (cellSizeETX == 0 ? 1 : cellSizeETX));
      int dlat = int((h5Y - offsetETY) / (cellSizeETY == 0 ? 1 : cellSizeETY));

      /* Convert from requested coordinate system (in the WMS request) to latlon */
      double lat = h5Y;
      double lon = h5X;
      imageWarperEchoToppen.reprojToLatLon(lon, lat);

      /* Finally add the found point to the datasource, these will become visibile when the point rendermethod is selected */
      dataSource->dataObjects[0]->points.push_back(PointDVWithLatLon(dlon, dlat, lon, lat, v));

      /* Also draw a dot on the virtual echotoppen grid, useful to see something in neartest neighbour rendermethod
         The AutoWMS defaults to nearest neightbour, so it will at least show the echotoppen as dots on a grid.
       */
      CConvertADAGUCPoint::drawDot(dlon, dlat, v, dimX->length, dimY->length, (float *)echoToppenVar->data);
    }
  }
  return 0;
}
