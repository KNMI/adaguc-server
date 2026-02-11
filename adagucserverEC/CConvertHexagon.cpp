/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
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

#include "CConvertHexagon.h"
#include "CFillTriangle.h"
#include "CImageWarper.h"

void line2(float *imagedata, int w, int h, float x1, float y1, float x2, float y2, float value) {
  int xyIsSwapped = 0;
  float dx = x2 - x1;
  float dy = y2 - y1;
  if (fabs(dx) < fabs(dy)) {
    std::swap(x1, y1);
    std::swap(x2, y2);
    std::swap(dx, dy);
    xyIsSwapped = 1;
  }
  if (x2 < x1) {
    std::swap(x1, x2);
    std::swap(y1, y2);
  }
  float gradient = dy / dx;
  float y = y1;

  if (xyIsSwapped == 0) {

    for (int x = int(x1); x < x2; x++) {
      //         plot(x,int(y),1);
      if (y >= 0 && y < h && x >= 0 && x < w) imagedata[int(x) + int(y) * w] = value;
      y += gradient;
    }
  } else {

    for (int x = int(x1); x < x2; x++) {
      if (y >= 0 && y < w && x >= 0 && x < h) imagedata[int(y) + int(x) * w] = value;
      y += gradient;
    }
  }
}

void drawlines2(float *imagedata, int w, int h, int polyCorners, float *polyX, float *polyY, float value) {
  for (int j = 0; j < polyCorners - 1; j++) {
    line2(imagedata, w, h, polyX[j], polyY[j], polyX[j + 1], polyY[j + 1], value);
  }
  line2(imagedata, w, h, polyX[polyCorners - 1], polyY[polyCorners - 1], polyX[0], polyY[0], value);
}

void drawNGon(float *imagedata, int w, int h, int polyCorners, float *polyX, float *polyY, float value) {
  // float  *data, float  *values, int W,int H, int *xP,int *yP

  int *xp = new int[polyCorners];
  int *yp = new int[polyCorners];
  int centerX = 0;
  int centerY = 0;
  for (int i = 0; i < polyCorners; i++) {
    xp[i] = polyX[i];
    yp[i] = polyY[i];
    centerX += xp[i];
    centerY += yp[i];
  }
  centerX /= float(polyCorners);
  centerY /= float(polyCorners);
  int txp[3];
  int typ[3];
  txp[0] = centerX;
  typ[0] = centerY;
  txp[1] = xp[polyCorners - 1];
  typ[1] = yp[polyCorners - 1];
  txp[2] = xp[0];
  typ[2] = yp[0];
  fillTriangleFlat(imagedata, value, w, h, txp, typ);
  for (int i = 1; i < polyCorners; i++) {

    txp[1] = xp[i - 1];
    typ[1] = yp[i - 1];
    txp[2] = xp[i];
    typ[2] = yp[i];
    fillTriangleFlat(imagedata, value, w, h, txp, typ);
  }
  delete[] xp;
  delete[] yp;
}

void drawpoly2(float *imagedata, int w, int h, int polyCorners, float *polyX, float *polyY, float value) {
  //  public-domain code by Darel Rex Finley, 2007

  int nodes, pixelY, i, j, swap;
  int IMAGE_TOP = 0;
  int IMAGE_BOT = h;
  int IMAGE_LEFT = 0;
  int IMAGE_RIGHT = w;
  int *nodeX = new int[polyCorners * 2 + 1];

  //  Loop through the rows of the image.
  for (pixelY = IMAGE_TOP; pixelY < IMAGE_BOT; pixelY++) {

    //  Build a list of nodes.
    nodes = 0;
    j = polyCorners - 1;
    for (i = 0; i < polyCorners; i++) {
      if ((polyY[i] < (double)pixelY && polyY[j] >= (double)pixelY) || (polyY[j] < (double)pixelY && polyY[i] >= (double)pixelY)) {
        nodeX[nodes++] = (int)(polyX[i] + (pixelY - polyY[i]) / (polyY[j] - polyY[i]) * (polyX[j] - polyX[i]));
      }
      j = i;
    }

    //  Sort the nodes, via a simple “Bubble” sort.
    i = 0;
    while (i < nodes - 1) {
      if (nodeX[i] > nodeX[i + 1]) {
        swap = nodeX[i];
        nodeX[i] = nodeX[i + 1];
        nodeX[i + 1] = swap;
        if (i) i--;
      } else {
        i++;
      }
    }

    //  Fill the pixels between node pairs.
    for (i = 0; i < nodes; i += 2) {
      if (nodeX[i] >= IMAGE_RIGHT) break;
      if (nodeX[i + 1] > IMAGE_LEFT) {
        if (nodeX[i] < IMAGE_LEFT) nodeX[i] = IMAGE_LEFT;
        if (nodeX[i + 1] > IMAGE_RIGHT) nodeX[i + 1] = IMAGE_RIGHT;
        for (j = nodeX[i]; j < nodeX[i + 1]; j++) {
          imagedata[int(j) + int(pixelY) * w] = value;
        }
      }
    }
  }
  delete[] nodeX;
}

double *CConvertHexagon::getBBOXFromLatLonFields(CDF::Variable *lons, CDF::Variable *lats) {

  size_t numCells = lons->dimensionlinks[0]->getSize();
  // int numVerts = lons->dimensionlinks[1]->getSize();

  lons->readData(CDF_FLOAT, true);
  lats->readData(CDF_FLOAT, true);

  float *lonData = (float *)lons->data;
  float *latData = (float *)lats->data;
  float fillValueLon = NAN;
  float fillValueLat = NAN;

  try {
    lons->getAttribute("_FillValue")->getData(&fillValueLon, 1);
  } catch (int e) {
  };
  try {
    lats->getAttribute("_FillValue")->getData(&fillValueLat, 1);
  } catch (int e) {
  };

  double minx = NAN, maxx = NAN, miny = NAN, maxy = NAN;

  bool firstLATLONDone = false;
  for (size_t j = 0; j < numCells; j++) {
    float lon = lonData[j];
    float lat = latData[j];
    if (!(lon == fillValueLon || lat == fillValueLat)) {
      if (firstLATLONDone == false) {
        minx = lon;
        maxx = lon;
        miny = lat;
        maxy = lat;
        firstLATLONDone = true;
      } else {

        if (lon < minx) minx = lon;
        if (lat < miny) miny = lat;
        if (lon > maxx) maxx = lon;
        if (lat > maxy) maxy = lat;
      }
    }
  }
  double *dfBBOX = new double[4];
  dfBBOX[0] = minx;
  dfBBOX[1] = miny;
  dfBBOX[2] = maxx;
  dfBBOX[3] = maxy;

  if (dfBBOX[2] - dfBBOX[0] > 300) {
    if ((dfBBOX[0] + dfBBOX[2]) / 2 > 170 && (dfBBOX[0] + dfBBOX[2]) / 2 < 190) {
      dfBBOX[0] -= 180;
      dfBBOX[2] -= 180;
    }
  }

  return dfBBOX;
}

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertHexagon::convertHexagonHeader(CDFObject *cdfObject, CServerParams *srvParams) {
  // Check whether this is really a hexagon file
  try {
    // cdfObject->getDimension("col");
    // cdfObject->getDimension("row");
    cdfObject->getDimension("nvert_i");
    cdfObject->getDimension("cell_i");
    cdfObject->getVariable("lon_i");
    cdfObject->getVariable("lat_i");
    cdfObject->getVariable("bounds_lon_i");
    cdfObject->getVariable("bounds_lat_i");

    CDF::Dimension *var = cdfObject->getDimension("nvert_i");
    if (var->getSize() != 6) {
      CDBDebug("Only hexagons are currently supported");
      return 1;
    }

  } catch (int e) {
    // CDBDebug("NOT HEXAGON DATA");
    return 1;
  }

  CDBDebug("Using CConvertHexagon.h");

  // Determine boundingbox of variable

  CDF::Variable *lons;
  CDF::Variable *lats;
  try {
    lons = cdfObject->getVariable("lon_i");
    lats = cdfObject->getVariable("lat_i");
  } catch (int e) {
    CDBError("lat_i or lon_i variables not found");
    return 1;
  }
  double *dfBBOX = getBBOXFromLatLonFields(lons, lats);

  // Default size of adaguc 2dField is 2x2
  int width = 2;
  int height = 2;

  if (srvParams->geoParams.width > 1 && srvParams->geoParams.height > 1) {
    width = srvParams->geoParams.width;
    height = srvParams->geoParams.height;
  }

  double cellSizeX = (dfBBOX[2] - dfBBOX[0]) / double(width);
  double cellSizeY = (dfBBOX[3] - dfBBOX[1]) / double(height);
  double offsetX = dfBBOX[0];
  double offsetY = dfBBOX[1];
  delete[] dfBBOX;
  dfBBOX = NULL;

  // Add time_counter, if available
  CDF::Dimension *time_counter_dim = cdfObject->getDimensionNE("time_counter");
  if (time_counter_dim != NULL) {
    time_counter_dim->name = "counter";
    CDF::Variable *time_counter_var = new CDF::Variable();
    time_counter_var->setType(CDF_DOUBLE);
    time_counter_var->name.copy(time_counter_dim->name.c_str());
    time_counter_var->isDimension = true;
    time_counter_var->dimensionlinks.push_back(time_counter_dim);
    cdfObject->addVariable(time_counter_var);
    CDF::allocateData(CDF_DOUBLE, &time_counter_var->data, time_counter_dim->length);
    for (size_t j = 0; j < time_counter_dim->length; j++) {
      ((double *)time_counter_var->data)[j] = j;
    }
  }

  // Add geo variables, only if they are not there already
  CDF::Dimension *dimX = cdfObject->getDimensionNE("adaguccoordinatex");
  CDF::Dimension *dimY = cdfObject->getDimensionNE("adaguccoordinatey");
  CDF::Variable *varX = cdfObject->getVariableNE("adaguccoordinatex");
  CDF::Variable *varY = cdfObject->getVariableNE("adaguccoordinatey");
  if (dimX == NULL || dimY == NULL || varX == NULL || varY == NULL) {
    // If not available, create new dimensions and variables (X,Y,T)
    // For x
    dimX = new CDF::Dimension();
    dimX->name = "adaguccoordinatex";
    dimX->setSize(width);
    cdfObject->addDimension(dimX);
    varX = new CDF::Variable();
    varX->setType(CDF_DOUBLE);
    varX->name.copy("adaguccoordinatex");
    varX->isDimension = true;
    varX->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(varX);
    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);

    // For y
    dimY = new CDF::Dimension();
    dimY->name = "adaguccoordinatey";
    dimY->setSize(height);
    cdfObject->addDimension(dimY);
    varY = new CDF::Variable();
    varY->setType(CDF_DOUBLE);
    varY->name.copy("adaguccoordinatey");
    varY->isDimension = true;
    varY->dimensionlinks.push_back(dimY);
    cdfObject->addVariable(varY);
    CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

#ifdef CCONVERTHEXAGON_DEBUG
    CDBDebug("Data allocated for 'x' and 'y' variables");
#endif

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

  // Make a list of variables which will be available as 2D fields
  std::vector<CT::string> varsToConvert;
  for (size_t v = 0; v < cdfObject->variables.size(); v++) {
    CDF::Variable *var = cdfObject->variables[v];
    if (var->isDimension == false) {
      if (!var->name.equals("time2D") && !var->name.equals("time") && !var->name.equals("wgs84") && !var->name.equals("epsg") && !var->name.equals("lon_i") && !var->name.equals("lat_i") &&
          !var->name.equals("bounds_lon_i") && !var->name.equals("bounds_lat_i") && !var->name.equals("custom") && !var->name.equals("projection") && !var->name.equals("product") &&
          !var->name.equals("iso_dataset") && !var->name.equals("tile_properties") && (var->name.indexOf("bnds") == -1)) {
        if (var->dimensionlinks.size() >= 1) {
          CDBDebug("Checking var %s with dimo %s", var->name.c_str(), var->dimensionlinks[1]->name.c_str());
          if (var->dimensionlinks[1]->name.equals("cell_i")) {
            varsToConvert.push_back(CT::string(var->name.c_str()));
          }
        }
      }
      if (var->name.equals("projection") || var->name.equals("lat_bounds_i") || var->name.equals("bounds_lon_i")) {
        var->setAttributeText("ADAGUC_SKIP", "true");
      }
    }
  }

  // Create the new 2D field variables based on the swath variables
  for (size_t v = 0; v < varsToConvert.size(); v++) {
    CDF::Variable *hexagonVar = cdfObject->getVariable(varsToConvert[v].c_str());

    // Remove projection attribute if we use lat/lon for projecting
    hexagonVar->removeAttribute("grid_mapping");

#ifdef CCONVERTHEXAGON_DEBUG
    CDBDebug("Converting %d/%d %s", v, varsToConvert.size(), hexagonVar->name.c_str());
#endif

    CDF::Variable *new2DVar = new CDF::Variable();
    cdfObject->addVariable(new2DVar);

    for (size_t d = 0; d < hexagonVar->dimensionlinks.size() - 1; d++) {
      new2DVar->dimensionlinks.push_back(hexagonVar->dimensionlinks[d]);
    }

    new2DVar->dimensionlinks.push_back(dimY);
    new2DVar->dimensionlinks.push_back(dimX);

    new2DVar->setType(hexagonVar->getType());
    new2DVar->name = hexagonVar->name.c_str();
    hexagonVar->name.concat("_backup");

    // Copy variable attributes
    for (size_t j = 0; j < hexagonVar->attributes.size(); j++) {
      CDF::Attribute *a = hexagonVar->attributes[j];
      new2DVar->setAttribute(a->name.c_str(), a->getType(), a->data, a->length);
      new2DVar->setAttributeText("ADAGUC_VECTOR", "true");
    }

    // The swath variable is not directly plotable, so skip it
    hexagonVar->setAttributeText("ADAGUC_SKIP", "true");

    // Scale and offset are already applied
    new2DVar->removeAttribute("scale_factor");
    new2DVar->removeAttribute("add_offset");

    new2DVar->setType(CDF_FLOAT);
  }

  return 0;
}

/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertHexagon::convertHexagonData(CDataSource *dataSource, int mode) {

  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
  // Check whether this is really a hexagon file
  try {
    // cdfObject->getDimension("col");
    // cdfObject->getDimension("row");
    cdfObject->getDimension("nvert_i");
    cdfObject->getDimension("cell_i");
    cdfObject->getVariable("lon_i");
    cdfObject->getVariable("lat_i");
    cdfObject->getVariable("bounds_lon_i");
    cdfObject->getVariable("bounds_lat_i");

    CDF::Dimension *var = cdfObject->getDimension("nvert_i");
    if (var->getSize() != 6) {
      CDBDebug("Only hexagons are currently supported");
      return 1;
    }

  } catch (int e) {
    return 1;
  }

#ifdef CCONVERTHEXAGON_DEBUG
  CDBDebug("THIS IS Hexagon VECTOR DATA");
#endif

  size_t nrDataObjects = dataSource->getNumDataObjects();
  std::vector<CDataSource::DataObject *> dataObjects(nrDataObjects, nullptr);
  for (size_t d = 0; d < nrDataObjects; d++) {
    dataObjects[d] = dataSource->getDataObject(d);
  }
  CDF::Variable *new2DVar;
  new2DVar = dataObjects[0]->cdfVariable;

  CDF::Variable *hexagonVar;
  CT::string origSwathName = new2DVar->name.c_str();
  origSwathName.concat("_backup");
  hexagonVar = cdfObject->getVariableNE(origSwathName.c_str());
  if (hexagonVar == NULL) {
    // CDBError("Unable to find orignal swath variable with name %s",origSwathName.c_str());
    return 1;
  }

  size_t numDims = hexagonVar->dimensionlinks.size();
  size_t start[numDims];
  size_t count[numDims];
  ptrdiff_t stride[numDims];
  for (size_t dimInd = 0; dimInd < numDims; dimInd++) {
    start[dimInd] = 0;
    count[dimInd] = 1;
    stride[dimInd] = 1;

    CT::string dimName = hexagonVar->dimensionlinks[dimInd]->name.c_str();

    if (numDims - dimInd < 2) {
      start[dimInd] = 0;
      count[dimInd] = hexagonVar->dimensionlinks[dimInd]->getSize();
    } else {
      start[dimInd] = dataSource->getDimensionIndex(dimName.c_str());
    }
    CDBDebug("%s = %lu %lu", dimName.c_str(), start[dimInd], count[dimInd]);
  }

  hexagonVar->readData(CDF_FLOAT, start, count, stride, true);

  // Read original data first

  CDF::Attribute *fillValue = hexagonVar->getAttributeNE("_FillValue");
  if (fillValue != NULL) {

    dataObjects[0]->hasNodataValue = true;
    fillValue->getData(&dataObjects[0]->dfNodataValue, 1);
#ifdef CCONVERTHEXAGON_DEBUG
    CDBDebug("_FillValue = %f", dataObjects[0]->dfNodataValue);
#endif
    CDF::Attribute *fillValue2d = new2DVar->getAttributeNE("_FillValue");
    if (fillValue2d == NULL) {
      fillValue2d = new CDF::Attribute();
      fillValue2d->name = "_FillValue";
    }
    float f = dataObjects[0]->dfNodataValue;
    fillValue2d->setData(CDF_FLOAT, &f, 1);
  } else {
    dataObjects[0]->hasNodataValue = true;
    dataObjects[0]->dfNodataValue = NC_FILL_FLOAT;
    float f = dataObjects[0]->dfNodataValue;
    new2DVar->setAttribute("_FillValue", CDF_FLOAT, &f, 1);
  }

  // Detect minimum and maximum values
  float fill = (float)dataObjects[0]->dfNodataValue;
  float min = 0;
  float max = 0;
  bool firstValueDone = false;

#ifdef CCONVERTHEXAGON_DEBUG
  CDBDebug("Size == %d", hexagonVar->getSize());
#endif
  for (size_t j = 0; j < hexagonVar->getSize(); j++) {
    float v = ((float *)hexagonVar->data)[j];

    if (v != fill && v != INFINITY && v != NAN && v != -INFINITY && v == v) {
      if (!firstValueDone) {
        min = v;
        max = v;
        firstValueDone = true;
      }
      if (v < min) min = v;
      if (v > max) {
        max = v;
      }

      //      CDBDebug("Swathvar %f %f %f",v,min,max);
    }
  }

#ifdef CCONVERTHEXAGON_DEBUG
  CDBDebug("Calculated min/max : %f %f", min, max);
#endif

  // Set statistics
  if (dataSource->stretchMinMax) {
#ifdef CCONVERTHEXAGON_DEBUG
    CDBDebug("dataSource->stretchMinMax");
#endif
    if (dataSource->statistics == NULL) {
#ifdef CCONVERTHEXAGON_DEBUG
      CDBDebug("Setting statistics: min/max : %f %f", min, max);
#endif
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->setMaximum(max);
      dataSource->statistics->setMinimum(min);
    }
  }

  // Make the width and height of the new 2D adaguc field the same as the viewing window
  dataSource->dWidth = dataSource->srvParams->geoParams.width;
  dataSource->dHeight = dataSource->srvParams->geoParams.height;

  /*if(dataSource->dWidth == 1 && dataSource->dHeight == 1){
    dataSource->srvParams->geoParams.bbox.left=dataSource->srvParams->geoParams.bbox.left;
    dataSource->srvParams->geoParams.bbox.bottom=dataSource->srvParams->geoParams.bbox.bottom;
    dataSource->srvParams->geoParams.bbox.right=dataSource->srvParams->geoParams.bbox.right;
    dataSource->srvParams->geoParams.bbox.top=dataSource->srvParams->geoParams.bbox.top;
  }*/

  // Width needs to be at least 2 in this case.
  if (dataSource->dWidth == 1) dataSource->dWidth = 2;
  if (dataSource->dHeight == 1) dataSource->dHeight = 2;
  double cellSizeX = dataSource->srvParams->geoParams.bbox.span().x / dataSource->dWidth;
  double cellSizeY = dataSource->srvParams->geoParams.bbox.span().y / dataSource->dHeight;
  double offsetX = dataSource->srvParams->geoParams.bbox.left;
  double offsetY = dataSource->srvParams->geoParams.bbox.bottom;

  if (mode == CNETCDFREADER_MODE_OPEN_ALL) {

#ifdef CCONVERTHEXAGON_DEBUG
    CDBDebug("Drawing %s with WH = [%d,%d]", new2DVar->name.c_str(), dataSource->dWidth, dataSource->dHeight);
#endif

    CDF::Dimension *dimX;
    CDF::Dimension *dimY;
    CDF::Variable *varX;
    CDF::Variable *varY;

    // Create new dimensions and variables (X,Y,T)
    dimX = cdfObject->getDimension("adaguccoordinatex");
    dimX->setSize(dataSource->dWidth);

    dimY = cdfObject->getDimension("adaguccoordinatey");
    dimY->setSize(dataSource->dHeight);

    varX = cdfObject->getVariable("adaguccoordinatex");
    varY = cdfObject->getVariable("adaguccoordinatey");

    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);
    CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

#ifdef CCONVERTHEXAGON_DEBUG
    CDBDebug("Data allocated for 'x' and 'y' variables");
#endif

    // Fill in the X and Y dimensions with the array of coordinates
    for (size_t j = 0; j < dimX->length; j++) {
      double x = offsetX + double(j) * cellSizeX + cellSizeX / 2;
      ((double *)varX->data)[j] = x;
    }
    for (size_t j = 0; j < dimY->length; j++) {
      double y = offsetY + double(j) * cellSizeY + cellSizeY / 2;
      ((double *)varY->data)[j] = y;
    }

    size_t fieldSize = dataSource->dWidth * dataSource->dHeight;
    new2DVar->setSize(fieldSize);
    CDF::allocateData(new2DVar->getType(), &(new2DVar->data), fieldSize);

    // Draw data!
    if (dataObjects[0]->hasNodataValue) {
      for (size_t j = 0; j < fieldSize; j++) {
        ((float *)dataObjects[0]->cdfVariable->data)[j] = (float)dataObjects[0]->dfNodataValue;
      }
    } else {
      for (size_t j = 0; j < fieldSize; j++) {
        ((float *)dataObjects[0]->cdfVariable->data)[j] = NAN;
      }
    }

    float *sdata = ((float *)dataObjects[0]->cdfVariable->data);

    CImageWarper imageWarper;
    bool projectionRequired = false;
    if (dataSource->srvParams->geoParams.crs.length() > 0) {
      projectionRequired = true;
      new2DVar->setAttributeText("grid_mapping", "customgridprojection");
      // Apply once
      if (cdfObject->getVariableNE("customgridprojection") == NULL) {
        CDF::Variable *projectionVar = new CDF::Variable();
        projectionVar->name.copy("customgridprojection");
        cdfObject->addVariable(projectionVar);
        dataSource->nativeEPSG = dataSource->srvParams->geoParams.crs.c_str();
        imageWarper.decodeCRS(&dataSource->nativeProj4, &dataSource->nativeEPSG, &dataSource->srvParams->cfg->Projection);
        if (dataSource->nativeProj4.length() == 0) {
          dataSource->nativeProj4 = LATLONPROJECTION;
          dataSource->nativeEPSG = "EPSG:4326";
          projectionRequired = false;
        }
        projectionVar->setAttributeText("proj4_params", dataSource->nativeProj4.c_str());
      }
    }

#ifdef CCONVERTHEXAGON_DEBUG
    CDBDebug("Datasource CRS = %s nativeproj4 = %s", dataSource->nativeEPSG.c_str(), dataSource->nativeProj4.c_str());
    CDBDebug("Datasource bbox:%f %f %f %f", dataSource->srvParams->geoParams.bbox.left, dataSource->srvParams->geoParams.bbox.bottom, dataSource->srvParams->geoParams.bbox.right,
             dataSource->srvParams->geoParams.bbox.top);
    CDBDebug("Datasource width height %d %d", dataSource->dWidth, dataSource->dHeight);
#endif

    if (projectionRequired) {
      int status = imageWarper.initreproj(dataSource, dataSource->srvParams->geoParams, &dataSource->srvParams->cfg->Projection);
      if (status != 0) {
        CDBError("Unable to init projection");
        return 1;
      }
    }

    float *hexagonData = (float *)hexagonVar->data;

    bool drawBilinear = false;
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    if (styleConfiguration->styleCompositionName.indexOf("bilinear") >= 0) {

      // drawBilinear=true;
    }
    /*
     * Bilinear rendering is based on gouraud shading using the center of each quads by using lat and lon variables, while nearest neighbour rendering is based on lat_bnds and lat_bnds variables,
     * drawing the corners of the quads..
     */

    // drawBilinear=true;
    // Bilinear rendering
    // TODO
    if (drawBilinear) {

      CDF::Variable *lons;
      CDF::Variable *lats;

      try {
        lons = cdfObject->getVariable("lon");
        lats = cdfObject->getVariable("lat");
      } catch (int e) {
        CDBError("lat or lon variables not found");
        return 1;
      }

      //       int numCells = cdfObject->getDimension("row")->getSize();
      //       int numVerts = cdfObject->getDimension("col")->getSize();

      int numCells = lons->dimensionlinks[0]->getSize();
      int numVerts = lons->dimensionlinks[1]->getSize();

      lons->readData(CDF_FLOAT, true);
      lats->readData(CDF_FLOAT, true);

      float *lonData = (float *)lons->data;
      float *latData = (float *)lats->data;

      float fillValueLon = NAN;
      float fillValueLat = NAN;

      try {
        lons->getAttribute("_FillValue")->getData(&fillValueLon, 1);
      } catch (int e) {
      };
      try {
        lats->getAttribute("_FillValue")->getData(&fillValueLat, 1);
      } catch (int e) {
      };

      for (int y = 0; y < numCells - 1; y++) {
        for (int x = 0; x < numVerts - 1; x++) {
          size_t pSwath = x + y * numVerts;
          // CDBDebug("%d %d %d",x,y,pSwath);
          double lons[4], lats[4];
          float vals[4];
          lons[0] = (float)lonData[pSwath];
          lons[1] = (float)lonData[pSwath + 1];
          lons[2] = (float)lonData[pSwath + numVerts];
          lons[3] = (float)lonData[pSwath + numVerts + 1];

          lats[0] = (float)latData[pSwath];
          lats[1] = (float)latData[pSwath + 1];
          lats[2] = (float)latData[pSwath + numVerts];
          lats[3] = (float)latData[pSwath + numVerts + 1];

          vals[0] = hexagonData[pSwath];
          vals[1] = hexagonData[pSwath + 1];
          vals[2] = hexagonData[pSwath + numVerts];
          vals[3] = hexagonData[pSwath + numVerts + 1];

          bool tileHasNoData = false;
          bool tileIsOverDateBorder = false;
          for (int j = 0; j < 4; j++) {
            float lon = lons[j];
            float lat = lats[j];
            float val = vals[j];
            if (val == fill || val == INFINITY || val == NAN || val == -INFINITY || !(val == val)) {
              tileHasNoData = true;
              break;
            }
            if (lat == fillValueLat || lat == INFINITY || lat == -INFINITY || !(lat == lat)) {
              tileHasNoData = true;
              break;
            }
            if (lon == fillValueLon || lon == INFINITY || lon == -INFINITY || !(lon == lon)) {
              tileHasNoData = true;
              break;
            }
            if (lon > 180) tileIsOverDateBorder = true;
          }
          if (tileHasNoData == false) {
            int dlons[4], dlats[4];
            bool projectionIsOk = true;
            for (int j = 0; j < 4; j++) {
              if (tileIsOverDateBorder) {
                lons[j] -= 360;
                if (lons[j] < -280) lons[j] += 360;
              }

              if (projectionRequired) {
                if (imageWarper.reprojfromLatLon(lons[j], lats[j]) != 0) projectionIsOk = false;
              }

              dlons[j] = int((lons[j] - offsetX) / cellSizeX);
              dlats[j] = int((lats[j] - offsetY) / cellSizeY);
            }
            if (projectionIsOk) {
              fillQuadGouraud(sdata, vals, dataSource->dWidth, dataSource->dHeight, dlons, dlats);
            }
          }
        }
      }
    }

    // Nearest neighbour rendering
    if (drawBilinear == false) {
      CDF::Variable *swathLon;
      CDF::Variable *swathLat;

      try {
        swathLon = cdfObject->getVariable("bounds_lon_i");
        swathLat = cdfObject->getVariable("bounds_lat_i");
      } catch (int e) {
        CDBError("bounds_lat_i or bounds_lon_i variables not found");
        return 1;
      }

      int numCells = swathLon->dimensionlinks[0]->getSize();
      int numVerts = swathLon->dimensionlinks[1]->getSize();
#ifdef CCONVERTHEXAGON_DEBUG
      CDBDebug("numCells %d, numVerts %d", numCells, numVerts);
#endif
      int numTiles = numCells;

      // int numTiles =     cdfObject->getDimension("col")->getSize()*cdfObject->getDimension("row")->getSize();

#ifdef CCONVERTHEXAGON_DEBUG
      CDBDebug("There are %d tiles", numTiles);
#endif

      swathLon->readData(CDF_FLOAT, true);
      swathLat->readData(CDF_FLOAT, true);
      float *lonData = (float *)swathLon->data;
      float *latData = (float *)swathLat->data;

      float fillValueLon = NAN;
      float fillValueLat = NAN;

      try {
        swathLon->getAttribute("_FillValue")->getData(&fillValueLon, 1);
      } catch (int e) {
      };
      try {
        swathLat->getAttribute("_FillValue")->getData(&fillValueLat, 1);
      } catch (int e) {
      };

      for (int pSwath = 0; pSwath < numTiles; pSwath++) {

        float *lons = &lonData[pSwath * numVerts];
        float *lats = &latData[pSwath * numVerts];
        float val = hexagonData[pSwath];

        bool tileHasNoData = false;

        bool tileIsOverDateBorder = false;

        for (int j = 0; j < numVerts; j++) {
          float lon = lons[j];
          float lat = lats[j];

          if (val == fill || val == INFINITY || val == NAN || val == -INFINITY || !(val == val)) {
            tileHasNoData = true;
            break;
          }
          if (lat == fillValueLat || lat == INFINITY || lat == -INFINITY || !(lat == lat)) {
            tileHasNoData = true;
            break;
          }
          if (lon == fillValueLon || lon == INFINITY || lon == -INFINITY || !(lon == lon)) {
            tileHasNoData = true;
            break;
          }
          if (lon > 178) tileIsOverDateBorder = true; // Needs further inspection
        }
        if (tileIsOverDateBorder == true) {
          float center = 0;
          for (int j = 0; j < numVerts; j++) {
            center += lons[j];
          }
          center /= float(numVerts);
          for (int j = 0; j < numVerts; j++) {

            if (lons[0] > center) {
              if (lons[0] - lons[j] > 280) {
                lons[j] += 360;
              }
            } else {
              if (lons[j] - lons[0] > 280) {
                lons[j] -= 360;
              }
            }
          }
          for (int j = 0; j < numVerts; j++) {
            if (fabs(lons[j] - center) > 50) tileHasNoData = true;
          }
        }
        if (tileHasNoData == false) {
          // CDBDebug(" (%f,%f) (%f,%f) (%f,%f) (%f,%f)",lons[0],lats[0],lons[1],lats[1],lons[2],lats[2],lons[3],lats[3]);

          float flons[numVerts], flats[numVerts];
          for (int j = 0; j < numVerts; j++) {
            //             if(tileIsOverDateBorder){
            //               lons[j]-=360;
            //               while(lons[j]<-300)lons[j]+=360;
            //             }
            double dflon = lons[j];
            double dflat = lats[j];
            if (projectionRequired)
              if (imageWarper.reprojfromLatLon(dflon, dflat) != 0) {
                tileHasNoData = true;
                break;
              }
            flons[j] = float((dflon - offsetX) / cellSizeX);
            flats[j] = float((dflat - offsetY) / cellSizeY);
          }
          if (tileHasNoData == false) {

            // CDBDebug(" (%d,%d) (%d,%d) (%d,%d) (%d,%d)",dlons[0],dlats[0],dlons[1],dlats[1],dlons[2],dlats[2],dlons[3],dlats[3]);

            // fillQuadGouraud(sdata, vals, dataSource->dWidth,dataSource->dHeight, dlons,dlats);

            // drawpoly2(sdata,dataSource->dWidth,dataSource->dHeight,numVerts,flons,flats,val);
            drawNGon(sdata, dataSource->dWidth, dataSource->dHeight, numVerts, flons, flats, val);
            // drawlines2(sdata,dataSource->dWidth,dataSource->dHeight,numVerts,flons,flats,val);
          }
        }
      }
    }
    imageWarper.closereproj();
  }
#ifdef CCONVERTHEXAGON_DEBUG
  CDBDebug("/convertHexagonData");
#endif
  return 0;
}
