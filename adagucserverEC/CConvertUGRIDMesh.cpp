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

const char *CConvertUGRIDMesh::className = "CConvertUGRIDMesh";

#define CCONVERTUGRIDMESH_NODATA -32000
#define CCONVERTUGRIDMESH_MESH_VARIABLE "mesh2d"
#define NEWGRID_VAR_X "x"
#define NEWGRID_VAR_Y "y"
#define ORIGGRID_SUFFIX "orig_ugrid_var"
#define CCONVERTUGRIDMESH_NODE_COORDINATES_ATTRIBUTENAME "node_coordinates"
#define CCONVERTUGRIDMESH_FACE_NODE_CONNECTIVITY_ATTRIBUTENAME "face_node_connectivity"
#define CCONVERTUGRIDMESH_DEFAULT_MESH_NODE_NAME_X "mesh2d_node_x"
#define CCONVERTUGRIDMESH_DEFAULT_MESH_NODE_NAME_Y "mesh2d_node_y"
#define CCONVERTUGRIDMESH_DEFAULT_MESH_FACE_NODES_NAME "mesh2d_face_nodes"

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

void drawlines(float *imagedata, int w, int h, std::vector<f8point> &polyCoords, float value) {
  for (int j = 0; j < int(polyCoords.size()) - 1; j++) {
    if ((polyCoords[j].x >= 0 && polyCoords[j].y >= 0 && polyCoords[j].x < w && polyCoords[j].y < h) ||
        (polyCoords[j + 1].x >= 0 && polyCoords[j + 1].y >= 0 && polyCoords[j + 1].x < w && polyCoords[j + 1].y < h)) {
      if (polyCoords[j].x != CCONVERTUGRIDMESH_NODATA && polyCoords[j + 1].x != CCONVERTUGRIDMESH_NODATA) {
        line(imagedata, w, h, polyCoords[j].x, polyCoords[j].y, polyCoords[j + 1].x, polyCoords[j + 1].y, value);
      }
    }
  }
}

void drawpoly(float *imagedata, int w, int h, std::vector<f8point> &polyCoords, float value) {

  //  public-domain code by Darel Rex Finley, 2007
  size_t polyCorners = polyCoords.size() - 1;
  if (polyCorners < 2) return;
  int nodes, nodeX[polyCorners * 2 + 1], pixelY, i, j, swap;

  int minx = polyCoords[0].x;
  int miny = polyCoords[0].y;
  int maxx = polyCoords[0].x;
  int maxy = polyCoords[0].y;
  for (auto &polCoord : polyCoords) {
    if (minx > polCoord.x) minx = polCoord.x;
    if (miny > polCoord.y) miny = polCoord.y;
    if (maxx < polCoord.x) maxx = polCoord.x;
    if (maxy < polCoord.y) maxy = polCoord.y;
  }

  int IMAGE_TOP = std::max(miny - 1, 0);
  int IMAGE_BOT = std::min(maxy + 1, h);
  int IMAGE_LEFT = std::max(minx - 1, 0);
  int IMAGE_RIGHT = std::min(maxx + 1, w);

  //  Loop through the rows of the image.
  for (pixelY = IMAGE_TOP; pixelY < IMAGE_BOT; pixelY++) {

    //  Build a list of nodes.
    nodes = 0;
    j = polyCorners - 1;

    for (size_t i = 0; i < polyCorners; i++) {
      if ((polyCoords[i].y < pixelY && polyCoords[j].y >= pixelY) || (polyCoords[j].y < pixelY && polyCoords[i].y >= pixelY)) {
        nodeX[nodes++] = (int)(polyCoords[i].x + (pixelY - polyCoords[i].y) / (polyCoords[j].y - polyCoords[i].y) * (polyCoords[j].x - polyCoords[i].x));
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

void setup2dGrid(CDFObject *cdfObject, double dfBBOX[4], int width = 2, int height = 2) {
  // Add dims
  CDF::Dimension *dimX = cdfObject->getDimensionNE(NEWGRID_VAR_X) != nullptr ? cdfObject->getDimensionNE(NEWGRID_VAR_X) : new CDF::Dimension(cdfObject, NEWGRID_VAR_X, width);
  CDF::Dimension *dimY = cdfObject->getDimensionNE(NEWGRID_VAR_Y) != nullptr ? cdfObject->getDimensionNE(NEWGRID_VAR_Y) : new CDF::Dimension(cdfObject, NEWGRID_VAR_Y, height);

  // Add vars
  CDF::Variable *varX = cdfObject->getVariableNE(NEWGRID_VAR_X) != nullptr ? cdfObject->getVariableNE(NEWGRID_VAR_X) : new CDF::Variable(cdfObject, NEWGRID_VAR_X, CDF_DOUBLE, {dimX}, true);
  CDF::Variable *varY = cdfObject->getVariableNE(NEWGRID_VAR_Y) != nullptr ? cdfObject->getVariableNE(NEWGRID_VAR_Y) : new CDF::Variable(cdfObject, NEWGRID_VAR_Y, CDF_DOUBLE, {dimY}, true);

  // Allocate data
  dimX->setSize(width);
  dimY->setSize(height);
  CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);
  CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

  // Calculate cellsize and offset
  double cellSizeX = (dfBBOX[2] - dfBBOX[0]) / double(width);
  double cellSizeY = (dfBBOX[3] - dfBBOX[1]) / double(height);
  double offsetX = dfBBOX[0];
  double offsetY = dfBBOX[1];

  // Fill in the X and Y dimensions with the array of coordinates
  for (size_t j = 0; j < dimX->length; j++) {
    double x = offsetX + j * cellSizeX + cellSizeX / 2;
    ((double *)varX->data)[j] = x;
  }
  for (size_t j = 0; j < dimY->length; j++) {
    double y = offsetY + j * cellSizeY + cellSizeY / 2;
    ((double *)varY->data)[j] = y;
  }
}

void setupVarsToConvert(CDFObject *cdfObject, CT::StackList<CT::string> varsToConvert) {
  CDF::Dimension *dimX = cdfObject->getDimensionNE(NEWGRID_VAR_X);
  CDF::Dimension *dimY = cdfObject->getDimensionNE(NEWGRID_VAR_Y);
  // Create the new 2D field variables based on the mesh variables
  for (size_t v = 0; v < varsToConvert.size(); v++) {
    CDF::Variable *meshVar = cdfObject->getVariable(varsToConvert[v].c_str());

    CDF::Variable *newStandardGridVar = new CDF::Variable();
    cdfObject->addVariable(newStandardGridVar);

    newStandardGridVar->dimensionlinks.push_back(dimY);
    newStandardGridVar->dimensionlinks.push_back(dimX);

    newStandardGridVar->setType(meshVar->getType());
    newStandardGridVar->name = meshVar->name.c_str();

    // Rename the original variable by appending a suffix
    meshVar->name.concat(ORIGGRID_SUFFIX);

    // Copy variable attributes
    for (size_t j = 0; j < meshVar->attributes.size(); j++) {
      CDF::Attribute *a = meshVar->attributes[j];
      newStandardGridVar->setAttribute(a->name.c_str(), a->getType(), a->data, a->length);
      newStandardGridVar->setAttributeText("UGRID_MESH", "true");
    }

    // Scale and offset are already applied
    newStandardGridVar->removeAttribute("scale_factor");
    newStandardGridVar->removeAttribute("add_offset");

    newStandardGridVar->setType(CDF_FLOAT);
    if (newStandardGridVar->getAttributeNE("_FillValue") == NULL) {
      float f = NAN;
      newStandardGridVar->setAttribute("_FillValue", CDF_FLOAT, &f, 1);
    }
    newStandardGridVar->removeAttribute("ADAGUC_SKIP");
  }
}

bool isUGrid(CDFObject *cdfObject) { return (cdfObject->getVariableNE(CCONVERTUGRIDMESH_MESH_VARIABLE) != nullptr); }

std::pair<CT::string, CT::string> getNodeCoordinateVariableNames(CDFObject *cdfObject) {
  auto meshVar = cdfObject->getVariable(CCONVERTUGRIDMESH_MESH_VARIABLE);

  auto nodeCoordinates = meshVar->getAttributeNE(CCONVERTUGRIDMESH_NODE_COORDINATES_ATTRIBUTENAME);
  CT::string mesh2d_node_x = CCONVERTUGRIDMESH_DEFAULT_MESH_NODE_NAME_X;
  CT::string mesh2d_node_y = CCONVERTUGRIDMESH_DEFAULT_MESH_NODE_NAME_Y;

  if (nodeCoordinates != nullptr) {
    auto list = nodeCoordinates->getDataAsString().splitToStack(" ");
    if (list.size() == 2) {
      mesh2d_node_x = list[0];
      mesh2d_node_y = list[1];
    }
  }
  return std::make_pair(mesh2d_node_x, mesh2d_node_y);
}

CT::string getMeshFaceNodeVariableName(CDFObject *cdfObject) {
  auto meshVar = cdfObject->getVariable(CCONVERTUGRIDMESH_MESH_VARIABLE);
  auto faceNodeConnectivityAttribute = meshVar->getAttributeNE(CCONVERTUGRIDMESH_FACE_NODE_CONNECTIVITY_ATTRIBUTENAME);
  CT::string faceNodeVariableName = CCONVERTUGRIDMESH_DEFAULT_MESH_FACE_NODES_NAME;
  if (faceNodeConnectivityAttribute != nullptr) {
    faceNodeVariableName = faceNodeConnectivityAttribute->getDataAsString();
  }
  return faceNodeVariableName;
}

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertUGRIDMesh::convertUGRIDMeshHeader(CDFObject *cdfObject, CServerParams *srvParams) {
  if (!isUGrid(cdfObject)) {
    return 1;
  }
  CDBDebug("Using CConvertUGRIDMesh.h");

  auto mesh2d_node_names = getNodeCoordinateVariableNames(cdfObject);
  CDF::Variable *pointLon = cdfObject->getVariableNE(mesh2d_node_names.first);
  if (pointLon == nullptr) {
    CDBError("mesh_node_x missing");
    return 1;
  }
  CDF::Variable *pointLat = cdfObject->getVariableNE(mesh2d_node_names.second);
  if (pointLat == nullptr) {
    CDBError("mesh_node_y missing");
    return 1;
  }
  pointLon->readData(CDF_DOUBLE);
  pointLat->readData(CDF_DOUBLE);

  // Reproject coords
  CImageWarper warper;
  warper.init(LATLONPROJECTION,
              "+proj=sterea +lat_0=52.15616055555555 +lon_0=5.38763888888889 +k=0.9999079 +x_0=155000 +y_0=463000 +ellps=bessel "
              "+towgs84=565.4171,50.3319,465.5524,-0.398957388243134,0.343987817378283,-1.87740163998045,4.0725 +units=m +no_defs",
              &srvParams->cfg->Projection);
  double *lons = (double *)pointLon->data;
  double *lats = (double *)pointLat->data;
  for (size_t j = 0; j < pointLon->getSize(); j++) {
    warper.reprojpoint(lons[j], lats[j]);
  }
  MinMax lonMinMax = getMinMax(pointLon);
  MinMax latMinMax = getMinMax(pointLat);
  double dfBBOX[] = {lonMinMax.min, latMinMax.min, lonMinMax.max, latMinMax.max};

  // Make a list of variables which will be available as 2D fields
  CT::StackList<CT::string> varsToConvert;
  for (size_t v = 0; v < cdfObject->variables.size(); v++) {
    CDF::Variable *var = cdfObject->variables[v];
    if (var->isDimension == false) {
      auto location = var->getAttributeNE("location");
      if (location != nullptr && location->getDataAsString().equals("face")) {
        varsToConvert.add(CT::string(var->name.c_str()));
      }
      var->setAttributeText("ADAGUC_SKIP", "true");
    }
  }

  setup2dGrid(cdfObject, dfBBOX);

  setupVarsToConvert(cdfObject, varsToConvert);

  return 0;
}

/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertUGRIDMesh::convertUGRIDMeshData(CDataSource *dataSource, int mode) {

  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
  if (!isUGrid(cdfObject) || mode != CNETCDFREADER_MODE_OPEN_ALL) {
    return 1;
  }

  CDF::Variable *newStandardGridVar = dataSource->getDataObject(0)->cdfVariable;

  // Obtain the original mesh variable and read its data
  CT::string originalUGridVarName = newStandardGridVar->name + ORIGGRID_SUFFIX;

  auto originalUGridVar = cdfObject->getVariableNE(originalUGridVarName.c_str());
  if (originalUGridVar == NULL) {
    CDBError("Unable to find orignal mesh variable with name %s", originalUGridVarName.c_str());
    return 1;
  }
  originalUGridVar->readData(CDF_DOUBLE, false);

  // Make the width and height of the new 2D adaguc field the same as the viewing window
  dataSource->dWidth = std::max(dataSource->srvParams->Geo->dWidth, 2);
  dataSource->dHeight = std::max(dataSource->srvParams->Geo->dHeight, 2);

  CDF::Attribute *fillValue = newStandardGridVar->getAttributeNE("_FillValue");
  if (fillValue != NULL) {
    dataSource->getDataObject(0)->hasNodataValue = true;
    fillValue->getData(&dataSource->getDataObject(0)->dfNodataValue, 1);
  }

  double cellSizeX = (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0]) / double(dataSource->dWidth);
  double cellSizeY = (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1]) / double(dataSource->dHeight);
  double offsetX = dataSource->srvParams->Geo->dfBBOX[0];
  double offsetY = dataSource->srvParams->Geo->dfBBOX[1];

  setup2dGrid(cdfObject, dataSource->srvParams->Geo->dfBBOX, dataSource->dWidth, dataSource->dHeight);

  size_t fieldSize = dataSource->dWidth * dataSource->dHeight;
  newStandardGridVar->setSize(fieldSize);
  CDF::allocateData(newStandardGridVar->getType(), &(newStandardGridVar->data), fieldSize);
  CDF::fill(newStandardGridVar->data, newStandardGridVar->getType(), dataSource->getDataObject(0)->dfNodataValue, fieldSize);

  CImageWarper imageWarper;
  int status = imageWarper.initreproj(dataSource, dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);
  if (status != 0) {
    CDBError("Unable to init projection");
    return 1;
  }
  bool projectionRequired = imageWarper.isProjectionRequired();

  float *sdata = ((float *)dataSource->getDataObject(0)->cdfVariable->data);

  auto mesh2d_node_names = getNodeCoordinateVariableNames(cdfObject);
  auto mesh2dNodeVarX = cdfObject->getVariableNE(mesh2d_node_names.first);
  auto mesh2dNodeVarY = cdfObject->getVariableNE(mesh2d_node_names.second);

  auto faceNodesName = getMeshFaceNodeVariableName(cdfObject);

  CDF::Variable *faceNodesVar = cdfObject->getVariable(faceNodesName.c_str());
  faceNodesVar->readData(CDF_INT, false);
  int *faceNodesVarData = (int *)faceNodesVar->data;
  int faceNodesVarData_Fill = -1;

  size_t numberOfFaces = faceNodesVar->dimensionlinks[0]->getSize();
  size_t nodesPerFace = faceNodesVar->dimensionlinks[1]->getSize();

  CDBDebug("Num faces: %d", faceNodesVar->dimensionlinks[0]->getSize());
  CDBDebug("Max face size: %d", nodesPerFace);

  // Make a projectedX and projectedY list
  size_t numMeshPoints = mesh2dNodeVarX->getSize();
  double mesh2dNodeVarXProjected[numMeshPoints];
  double mesh2dNodeVarYProjected[numMeshPoints];
  double mesh2dNodeVarLon[numMeshPoints];
  double mesh2dNodeVarLat[numMeshPoints];
  for (size_t j = 0; j < numMeshPoints; j++) {
    double lon = ((double *)mesh2dNodeVarX->data)[j];
    double lat = ((double *)mesh2dNodeVarY->data)[j];
    double tprojectedX = lon;
    double tprojectedY = lat;
    int dlon = CCONVERTUGRIDMESH_NODATA, dlat = CCONVERTUGRIDMESH_NODATA;
    if (tprojectedX > -360 && tprojectedX < 360 && tprojectedY > -90 && tprojectedY < 90 && tprojectedX != NAN && tprojectedY != NAN && tprojectedX > -HUGE_VAL && tprojectedX < HUGE_VAL &&
        tprojectedY > -HUGE_VAL && tprojectedY < HUGE_VAL) {
      // float v = j;
      int status = 0;
      if (projectionRequired) status = imageWarper.reprojfromLatLon(tprojectedX, tprojectedY);

      if (!status) {
        dlon = int((tprojectedX - offsetX) / cellSizeX);
        dlat = int((tprojectedY - offsetY) / cellSizeY);
      }
    }
    mesh2dNodeVarXProjected[j] = dlon;
    mesh2dNodeVarYProjected[j] = dlat;
    mesh2dNodeVarLon[j] = lon;
    mesh2dNodeVarLat[j] = lat;
  }

  try {
    faceNodesVar->getAttribute("_FillValue")->getData(&faceNodesVarData_Fill, 1);
  } catch (int e) {
    CDBWarning("Warning: FillValue not defined");
  }

  // float polyX[nodesPerFace + 1];
  // float polyY[nodesPerFace + 1];

  struct polydef {
    std::vector<f8point> poly;
    float v;
  };

  // Assemble polys
  std::vector<polydef> polys;
  auto *points = &dataSource->getDataObject(0)->points;
  for (size_t f = 0; f < numberOfFaces; f++) {
    bool skip = false;
    std::vector<f8point> poly;
    for (size_t j = 0; j < nodesPerFace; j++) {
      int p1 = faceNodesVarData[j + f * nodesPerFace] - 1;
      if (p1 == faceNodesVarData_Fill) {
        skip = true;
        break;
      }
      poly.push_back({.x = mesh2dNodeVarXProjected[p1], .y = mesh2dNodeVarYProjected[p1]});
    }
    if (skip) {
      continue;
    }
    poly.push_back(poly[0]);
    float v = ((double *)originalUGridVar->data)[f];
    polys.push_back({.poly = poly, .v = v});
    int p1 = faceNodesVarData[0 + f * nodesPerFace] - 1;
    int x = mesh2dNodeVarXProjected[p1];
    int y = mesh2dNodeVarYProjected[p1];
    points->push_back(PointDVWithLatLon(x, y, mesh2dNodeVarLon[p1], mesh2dNodeVarLat[p1], v));
  }

  for (auto &poly : polys) {
    drawpoly(sdata, dataSource->dWidth, dataSource->dHeight, poly.poly, poly.v);
  }

  // TODO enable to show  lines of the polys
  // for (auto poly : polys) {
  //   drawlines(sdata, dataSource->dWidth, dataSource->dHeight, poly.poly, 0);
  // }

  return 0;
}
