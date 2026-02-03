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

#include "CConvertUGRIDMesh.h"
#include "CFillTriangle.h"
#include "CImageWarper.h"

#define CCONVERTUGRIDMESH_NODATA -32000

void line(float *imagedata, int w, int h, float x1, float y1, float x2, float y2, float value) {
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

void drawlines(float *imagedata, int w, int h, int polyCorners, float *polyX, float *polyY, float value) {
  for (int j = 0; j < polyCorners - 1; j++) {
    if ((polyX[j] >= 0 && polyY[j] >= 0 && polyX[j] < w && polyY[j] < h) || (polyX[j + 1] >= 0 && polyY[j + 1] >= 0 && polyX[j + 1] < w && polyY[j + 1] < h)) {
      if (polyX[j] != CCONVERTUGRIDMESH_NODATA && polyX[j + 1] != CCONVERTUGRIDMESH_NODATA) {
        line(imagedata, w, h, polyX[j], polyY[j], polyX[j + 1], polyY[j + 1], value);
      }
    }
  }
}

void drawpoly(float *imagedata, int w, int h, int polyCorners, float *polyX, float *polyY, float value) {
  //  public-domain code by Darel Rex Finley, 2007

  int nodes, nodeX[polyCorners * 2 + 1], pixelY, i, j, swap;
  int IMAGE_TOP = 0;
  int IMAGE_BOT = h;
  int IMAGE_LEFT = 0;
  int IMAGE_RIGHT = w;

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
}

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertUGRIDMesh::convertUGRIDMeshHeader(CDFObject *cdfObject) {
  // Check whether this is really an ugrid file
  try {
    cdfObject->getVariable("mesh");
  } catch (int e) {
    return 1;
  }

  CDBDebug("Using CConvertUGRIDMesh.h");

  // Standard bounding box of adaguc data is worldwide

  // Standard bounding box of adaguc data is worldwide
  CDF::Variable *pointLon;
  CDF::Variable *pointLat;

  try {
    pointLon = cdfObject->getVariable("mesh_node_lon");
    pointLat = cdfObject->getVariable("mesh_node_lat");
  } catch (int e) {
    CDBError("Mesh2_node_x or Mesh2_node_y variables not found");
    return 1;
  }

#ifdef MEASURETIME
  StopWatch_Stop("UGRID MESH DATA");
#endif
  pointLon->readData(CDF_FLOAT, true);
  pointLat->readData(CDF_FLOAT, true);
#ifdef MEASURETIME
  StopWatch_Stop("DATA READ");
#endif
  MinMax lonMinMax = getMinMax(pointLon);
  MinMax latMinMax = getMinMax(pointLat);
#ifdef MEASURETIME
  StopWatch_Stop("MIN/MAX Calculated");
#endif
  double dfBBOX[] = {lonMinMax.min, latMinMax.min, lonMinMax.max, latMinMax.max};

  // Default size of adaguc 2dField is 2x2
  int width = 2;
  int height = 2;

  double cellSizeX = (dfBBOX[2] - dfBBOX[0]) / double(width);
  double cellSizeY = (dfBBOX[3] - dfBBOX[1]) / double(height);
  double offsetX = dfBBOX[0];
  double offsetY = dfBBOX[1];

  // Add geo variables, only if they are not there already
  CDF::Dimension *dimX = cdfObject->getDimensionNE("x");
  CDF::Dimension *dimY = cdfObject->getDimensionNE("y");
  CDF::Variable *varX = cdfObject->getVariableNE("x");
  CDF::Variable *varY = cdfObject->getVariableNE("y");
  if (dimX == NULL || dimY == NULL || varX == NULL || varY == NULL) {
    // If not available, create new dimensions and variables (X,Y,T)
    // For x
    dimX = new CDF::Dimension();
    dimX->name = "x";
    dimX->setSize(width);
    cdfObject->addDimension(dimX);
    varX = new CDF::Variable();
    varX->setType(CDF_DOUBLE);
    varX->name.copy("x");
    varX->isDimension = true;
    varX->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(varX);
    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);

    // For y
    dimY = new CDF::Dimension();
    dimY->name = "y";
    dimY->setSize(height);
    cdfObject->addDimension(dimY);
    varY = new CDF::Variable();
    varY->setType(CDF_DOUBLE);
    varY->name.copy("y");
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

  // Make a list of variables which will be available as 2D fields
  std::vector<CT::string> varsToConvert;
  for (size_t v = 0; v < cdfObject->variables.size(); v++) {
    CDF::Variable *var = cdfObject->variables[v];
    if (var->isDimension == false) {
      if (var->name.equals("mesh")) {
        varsToConvert.push_back(CT::string(var->name.c_str()));
      }
      // CDBDebug("%s",var->name.c_str());
      var->setAttributeText("ADAGUC_SKIP", "true");
    }
  }

  // Create the new 2D field variables based on the mesh variables
  for (size_t v = 0; v < varsToConvert.size(); v++) {
    CDF::Variable *meshVar = cdfObject->getVariable(varsToConvert[v].c_str());

#ifdef CCONVERTUGRIDMESH_DEBUG
    CDBDebug("Converting %s", meshVar->name.c_str());
#endif

    CDF::Variable *new2DVar = new CDF::Variable();
    cdfObject->addVariable(new2DVar);

    new2DVar->dimensionlinks.push_back(dimY);
    new2DVar->dimensionlinks.push_back(dimX);

    new2DVar->setType(meshVar->getType());
    new2DVar->name = meshVar->name.c_str();
    meshVar->name.concat("_backup");

    // Copy variable attributes
    for (size_t j = 0; j < meshVar->attributes.size(); j++) {
      CDF::Attribute *a = meshVar->attributes[j];
      new2DVar->setAttribute(a->name.c_str(), a->getType(), a->data, a->length);
      new2DVar->setAttributeText("UGRID_MESH", "true");
    }

    // Scale and offset are already applied
    new2DVar->removeAttribute("scale_factor");
    new2DVar->removeAttribute("add_offset");

    new2DVar->setType(CDF_FLOAT);
    if (new2DVar->getAttributeNE("_FillValue") == NULL) {
      float f = NAN;
      new2DVar->setAttribute("_FillValue", CDF_FLOAT, &f, 1);
    }
    new2DVar->removeAttribute("ADAGUC_SKIP");
  }

  return 0;
}

/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertUGRIDMesh::convertUGRIDMeshData(CDataSource *dataSource, int mode) {
  //   #ifdef CCONVERTUGRIDMESH_DEBUG
  //   CDBDebug("convertUGRIDMeshData");
  //   #endif
  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
  // Check whether this is really an ugrid file
  try {
    cdfObject->getVariable("mesh");
  } catch (int e) {
    return 1;
  }
  CDBDebug("THIS IS UGRID MESH DATA");
  size_t nrDataObjects = dataSource->getNumDataObjects();
  CDataSource::DataObject *dataObjects[nrDataObjects];
  for (size_t d = 0; d < nrDataObjects; d++) {
    dataObjects[d] = dataSource->getDataObject(d);
  }
  CDF::Variable *new2DVar;
  new2DVar = dataObjects[0]->cdfVariable;

  CDF::Variable *meshVar;
  CT::string origMeshName = new2DVar->name.c_str();
  origMeshName.concat("_backup");
  meshVar = cdfObject->getVariableNE(origMeshName.c_str());
  if (meshVar == NULL) {
    CDBError("Unable to find orignal mesh variable with name %s", origMeshName.c_str());
    return 1;
  }
  CDF::Variable *meshLon;
  CDF::Variable *meshLat;

  try {
    meshLon = cdfObject->getVariable("mesh_node_lon");
    meshLat = cdfObject->getVariable("mesh_node_lat");
  } catch (int e) {
    CDBError("lat or lon variables not found");
    return 1;
  }

  // Read original data first
  //   meshVar->readData(CDF_FLOAT,true);
  meshLon->readData(CDF_FLOAT, true);
  meshLat->readData(CDF_FLOAT, true);

  // Make the width and height of the new 2D adaguc field the same as the viewing window
  dataSource->dWidth = dataSource->srvParams->geoParams.width;
  dataSource->dHeight = dataSource->srvParams->geoParams.height;

  if (dataSource->dWidth == 1 && dataSource->dHeight == 1) {
    dataSource->srvParams->geoParams.bbox = dataSource->srvParams->geoParams.bbox;
  }

  // Width needs to be at least 2 in this case.
  if (dataSource->dWidth == 1) dataSource->dWidth = 2;
  if (dataSource->dHeight == 1) dataSource->dHeight = 2;
  double cellSizeX = dataSource->srvParams->geoParams.bbox.span().x / dataSource->dWidth;
  double cellSizeY = dataSource->srvParams->geoParams.bbox.span().y / dataSource->dHeight;
  double offsetX = dataSource->srvParams->geoParams.bbox.left;
  double offsetY = dataSource->srvParams->geoParams.bbox.bottom;

  CDF::Attribute *fillValue = new2DVar->getAttributeNE("_FillValue");
  if (fillValue != NULL) {
    dataObjects[0]->hasNodataValue = true;
    fillValue->getData(&dataObjects[0]->dfNodataValue, 1);
#ifdef CCONVERTADAGUCPOINT_DEBUG
    CDBDebug("_FillValue = %f", dataObjects[0]->dfNodataValue);
#endif
  }

  if (mode == CNETCDFREADER_MODE_OPEN_ALL) {
#ifdef CCONVERTUGRIDMESH_DEBUG
    CDBDebug("convertUGRIDMeshData OPEN ALL");
#endif

#ifdef CCONVERTUGRIDMESH_DEBUG
    CDBDebug("Drawing %s", new2DVar->name.c_str());
#endif

    CDF::Dimension *dimX;
    CDF::Dimension *dimY;
    CDF::Variable *varX;
    CDF::Variable *varY;

    // Create new dimensions and variables (X,Y,T)
    dimX = cdfObject->getDimension("x");
    dimX->setSize(dataSource->dWidth);

    dimY = cdfObject->getDimension("y");
    dimY->setSize(dataSource->dHeight);

    varX = cdfObject->getVariable("x");
    varY = cdfObject->getVariable("y");

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

    size_t fieldSize = dataSource->dWidth * dataSource->dHeight;
    new2DVar->setSize(fieldSize);
    CDF::allocateData(new2DVar->getType(), &(new2DVar->data), fieldSize);

    // Draw data!

    for (size_t j = 0; j < fieldSize; j++) {
      ((float *)new2DVar->data)[j] = dataObjects[0]->dfNodataValue;
    }

    float *sdata = ((float *)dataObjects[0]->cdfVariable->data);

    float *lonData = (float *)meshLon->data;
    float *latData = (float *)meshLat->data;

    size_t numMeshPoints = meshLon->getSize();

    CImageWarper imageWarper;
    //     bool projectionRequired=false;
    //     if(dataSource->srvParams->geoParams.CRS.length()>0){
    //       projectionRequired=true;
    //       new2DVar->setAttributeText("grid_mapping","customgridprojection");
    //       if(cdfObject->getVariableNE("customgridprojection")==NULL){
    //         CDF::Variable *projectionVar = new CDF::Variable();
    //         projectionVar->name.copy("customgridprojection");
    //         cdfObject->addVariable(projectionVar);
    //         dataSource->nativeEPSG = dataSource->srvParams->geoParams.CRS.c_str();
    //         imageWarper.decodeCRS(&dataSource->nativeProj4,&dataSource->nativeEPSG,&dataSource->srvParams->cfg->Projection);
    //         if(dataSource->nativeProj4.length()==0){
    //           dataSource->nativeProj4=LATLONPROJECTION;
    //           dataSource->nativeEPSG="EPSG:4326";
    //           projectionRequired=false;
    //         }
    //         projectionVar->setAttributeText("proj4_params",dataSource->nativeProj4.c_str());
    //       }
    //     }
    //

#ifdef CCONVERTUGRIDMESH_DEBUG
    CDBDebug("Datasource CRS = %s nativeproj4 = %s", dataSource->nativeEPSG.c_str(), dataSource->nativeProj4.c_str());
    CDBDebug("Datasource bbox:%f %f %f %f", dataSource->srvParams->geoParams.bbox.left, dataSource->srvParams->geoParams.bbox.bottom, dataSource->srvParams->geoParams.bbox.right,
             dataSource->srvParams->geoParams.bbox.top);
    CDBDebug("Datasource width height %d %d", dataSource->dWidth, dataSource->dHeight);
#endif

    // if(projectionRequired){
    int status = imageWarper.initreproj(dataSource, dataSource->srvParams->geoParams, &dataSource->srvParams->cfg->Projection);
    if (status != 0) {
      CDBError("Unable to init projection");
      return 1;
    }
    // }
    bool projectionRequired = imageWarper.isProjectionRequired();
    //     int polyCorners = 5;
    float projectedX[numMeshPoints]; //={10,100,40,110,20,10};
    float projectedY[numMeshPoints]; //={10,20,40,100,110,10};

#ifdef MEASURETIME
    StopWatch_Stop("Iterating lat/lon data");
#endif

    for (size_t j = 0; j < numMeshPoints; j++) {

      double lon = double(lonData[j]), lat = double(latData[j]);
      double tprojectedX = lon;
      double tprojectedY = lat;
      float v = j;
      int status = 0;
      if (projectionRequired) status = imageWarper.reprojfromLatLon(tprojectedX, tprojectedY);
      int dlon, dlat;
      if (!status) {
        dlon = int((tprojectedX - offsetX) / cellSizeX);
        dlat = int((tprojectedY - offsetY) / cellSizeY);
      } else {
        dlat = CCONVERTUGRIDMESH_NODATA;
        dlon = CCONVERTUGRIDMESH_NODATA;
      }
      dataObjects[0]->points.push_back(PointDVWithLatLon(dlon, dlat, lon, lat, v));
      projectedX[j] = dlon;
      projectedY[j] = dlat;
    }

#ifdef MEASURETIME
    StopWatch_Stop("Start reading face nodes");
#endif

    CDF::Variable *Mesh2_face_nodes = cdfObject->getVariable("mesh_face_nodes");
    Mesh2_face_nodes->readData(CDF_INT, false);
    int *Mesh2_face_nodesData = (int *)Mesh2_face_nodes->data;
    int Mesh2_face_nodesData_Fill = -1;

    try {
      Mesh2_face_nodes->getAttribute("_FillValue")->getData(&Mesh2_face_nodesData_Fill, 1);
      ;

    } catch (int e) {
      CDBWarning("Warning: FillValue not defined");
    }

    size_t nFaces = Mesh2_face_nodes->dimensionlinks[0]->getSize();
    size_t MaxNumNodesPerFace = Mesh2_face_nodes->dimensionlinks[1]->getSize();

    CDBDebug("Num faces: %d", Mesh2_face_nodes->dimensionlinks[0]->getSize());
    CDBDebug("Max face size: %d", MaxNumNodesPerFace);

    float polyX[MaxNumNodesPerFace + 1];
    float polyY[MaxNumNodesPerFace + 1];

    int numPoints = 0;
    //     CDBDebug("drawpolys");
    //     for(size_t f=0;f<nFaces;f++){
    //       for(size_t j=0;j<MaxNumNodesPerFace;j++){
    //         int p1 = Mesh2_face_nodesData[j+f*MaxNumNodesPerFace];
    //         if(p1!=Mesh2_face_nodesData_Fill){
    //           polyX[numPoints] = projectedX[p1];
    //           polyY[numPoints++] = projectedY[p1];
    //         }
    //       }
    //       polyX[numPoints] = polyX[0];
    //       polyY[numPoints++] = polyY[0];
    //       drawpoly(sdata,dataSource->dWidth,dataSource->dHeight,numPoints,polyX,polyY,f);
    //       //drawlines(sdata,dataSource->dWidth,dataSource->dHeight,numPoints,polyX,polyY);
    //       numPoints = 0;
    //     }

#ifdef MEASURETIME
    StopWatch_Stop("drawlines");
#endif
    for (size_t f = 0; f < nFaces; f++) {
      for (size_t j = 0; j < MaxNumNodesPerFace; j++) {
        int p1 = Mesh2_face_nodesData[j + f * MaxNumNodesPerFace];

        if (p1 != Mesh2_face_nodesData_Fill) {

          polyX[numPoints] = projectedX[p1];
          polyY[numPoints++] = projectedY[p1];
        }
      }
      polyX[numPoints] = polyX[0];
      polyY[numPoints++] = polyY[0];
      // drawpoly(sdata,dataSource->dWidth,dataSource->dHeight,numPoints,polyX,polyY,f);
      drawlines(sdata, dataSource->dWidth, dataSource->dHeight, numPoints, polyX, polyY, 0);
      numPoints = 0;
    }
#ifdef MEASURETIME
    StopWatch_Stop("drawlines done");
#endif
    imageWarper.closereproj();
  }
#ifdef CCONVERTUGRIDMESH_DEBUG
  CDBDebug("/convertUGRIDMeshData");
#endif
  return 0;
}
