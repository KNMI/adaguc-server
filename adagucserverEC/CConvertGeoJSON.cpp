/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  Transforms GeoJSON into a Grid.
 * Author:   Ernst de Vreede (KNMI)
 * Date:     2016-08
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

// GeoJSON in non standard geographical projection:
// http://cariska.mona.uwi.edu/geoserver/wfs?typename=geonode%3ACatchment&outputFormat=json&version=1.0.0&request=GetFeature&service=WFS
// GeoJSON in latlon geographical projection:
// http://cariska.mona.uwi.edu/geoserver/wfs?typename=geonode%3ACatchment&outputFormat=json&version=1.0.0&request=GetFeature&service=WFS&srsName=EPSG:4326

#include "CConvertGeoJSON.h"
#include "CImageWarper.h"
#include "CFillTriangle.h"

#define CCONVERTGEOJSONCOORDS_NODATA -32000
#define CCONVERTGEOJSON_FILL 65535u

struct Hole {
  int holeSize;
  float *holeX;
  float *holeY;
  float *projectedHoleXY;
};

int cntCnt = 0;
void buildNodeList(int pixelY, int &nodes, int nodeX[], int polyCorners, float *polyXY) {
  int i, j;
  int i2, j2;
  cntCnt++;
  //  Build a list of nodes.
  nodes = 0;
  j = polyCorners - 1;
  for (i = 0; i < polyCorners; i++) {
    i2 = i * 2;
    j2 = j * 2;
    if ((polyXY[i2 + 1] < (double)pixelY && polyXY[j2 + 1] >= (double)pixelY) || (polyXY[j2 + 1] < (double)pixelY && polyXY[i2 + 1] >= (double)pixelY)) {
      nodeX[nodes++] = (int)(polyXY[i2] + (pixelY - polyXY[i2 + 1]) / (polyXY[j2 + 1] - polyXY[i2 + 1]) * (polyXY[j2] - polyXY[i2]));
    }
    j = i;
  }
}

void bubbleSort(int nodes, int nodeX[]) {
  //  Sort the nodes, via a simple “Bubble” sort.
  int i, swap;
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
}

void drawpolyWithHoles_index(int xMin, int yMin, int xMax, int yMax, unsigned short int *imagedata, int w, int h, int polyCorners, float *polyXY, unsigned short int value, int holes,
                             Hole *holeArray) {
  if (xMax < 0 || yMax < 0 || xMin >= w || yMin >= h) return;

  //  public-domain code by Darel Rex Finley, 2007

  size_t nodeXLength = polyCorners * 2 + 1;
  int nodes, pixelY, i;

  if (xMin < 0) xMin = 0;
  if (yMin < 5) yMin = 0;
  if (xMax >= w) xMax = w;
  if (yMax >= h) yMax = h;

  int IMAGE_TOP = yMin;
  int IMAGE_BOT = yMax + 1;
  if (IMAGE_BOT > h) IMAGE_BOT = h;
  int IMAGE_LEFT = xMin;
  int IMAGE_RIGHT = xMax;
  int scanLineWidth = IMAGE_RIGHT - IMAGE_LEFT;

  int *nodeX = new int[nodeXLength];
  // Allocate  scanline
  unsigned short *scanline = new unsigned short[scanLineWidth];
  // Loop through the rows of the image.
  for (pixelY = IMAGE_TOP; pixelY < IMAGE_BOT; pixelY++) {

    for (i = 0; i < scanLineWidth; i++) scanline[i] = CCONVERTGEOJSON_FILL;
    buildNodeList(pixelY, nodes, nodeX, polyCorners, polyXY);
    bubbleSort(nodes, nodeX);
    for (i = 0; i < nodes; i += 2) {
      int x1 = nodeX[i] - IMAGE_LEFT;
      int x2 = nodeX[i + 1] - IMAGE_LEFT;
      if (x1 < 0) x1 = 0;
      if (x2 > scanLineWidth) x2 = scanLineWidth;
      for (int j = x1; j < x2; j++) {
        scanline[j] = value;
      }
    }

    for (int h = 0; h < holes; h++) {
      buildNodeList(pixelY, nodes, nodeX, holeArray[h].holeSize, holeArray[h].projectedHoleXY);
      bubbleSort(nodes, nodeX);
      for (i = 0; i < nodes; i += 2) {
        int x1 = nodeX[i] - IMAGE_LEFT;
        int x2 = nodeX[i + 1] - IMAGE_LEFT;
        if (x1 < 0) x1 = 0;
        if (x2 > scanLineWidth) x2 = scanLineWidth;
        for (int j = x1; j < x2; j++) {
          scanline[j] = CCONVERTGEOJSON_FILL;
        }
      }
    }
    unsigned int startScanLineY = pixelY * w;
    for (i = IMAGE_LEFT; i < IMAGE_RIGHT; i++) {
      if (scanline[i - IMAGE_LEFT] != CCONVERTGEOJSON_FILL) {
        imagedata[i + startScanLineY] = scanline[i - IMAGE_LEFT];
      }
    }
  }
  delete[] nodeX;
  delete[] scanline;
};

std::map<std::string, std::vector<Feature *>> CConvertGeoJSON::featureStore;

void CConvertGeoJSON::clearFeatureStore() {
  for (std::map<std::string, std::vector<Feature *>>::iterator itf = featureStore.begin(); itf != featureStore.end(); ++itf) {
    std::string fileName = itf->first.c_str();
    for (std::vector<Feature *>::iterator it = featureStore[fileName].begin(); it != featureStore[fileName].end(); ++it) {
      delete *it;
      *it = NULL;
    }
  }
  featureStore.clear();
}

/**
 * Delete one set of features from the featureStore
 */
void CConvertGeoJSON::clearFeatureStore(CT::string name) {
  for (std::map<std::string, std::vector<Feature *>>::iterator itf = featureStore.begin(); itf != featureStore.end(); ++itf) {
    std::string fileName = itf->first.c_str();
    if (fileName == name.c_str()) {
      for (std::vector<Feature *>::iterator it = featureStore[fileName].begin(); it != featureStore[fileName].end(); ++it) {
        delete *it;
        *it = NULL;
      }
    }
  }
}

std::vector<Feature *> getPointFeatures(std::vector<Feature *> features) {
  std::vector<Feature *> pointFeatures;
  for (Feature *f : features) {
    std::vector<GeoPoint> *pts = f->getPoints();
    if (pts->size() > 0) {
      pointFeatures.push_back(f);
    }
  }
  return pointFeatures;
}

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertGeoJSON::convertGeoJSONHeader(CDFObject *cdfObject) {
  // Check whether this is really a geojson file
  CDF::Variable *jsonVar;
  try {
    jsonVar = cdfObject->getVariable("jsoncontent");
  } catch (int e) {
    return 1;
  }

#ifdef CCONVERTGEOJSON_DEBUG
  CDBDebug("convertGeoJSONHeader");
  CDBDebug("Using CConvertGeoJSON.cpp");
#endif

  jsonVar->readData(CDF_CHAR);
  CT::string inputjsondata = (char *)jsonVar->data;
  json_value *json = json_parse((json_char *)inputjsondata.c_str(), inputjsondata.length());
#ifdef CCONVERTGEOJSON_DEBUG
  CDBDebug("JSON result: %x", json);
#endif
#ifdef MEASURETIME
  StopWatch_Stop("GeoJSON DATA");
#endif

  if (json == 0) {
    CDBDebug("Error parsing jsonfile");
    return 1;
  }

  std::vector<Feature *> features;
  BBOX dfBBOX;
  getBBOX(cdfObject, dfBBOX, *json, features);

  getDimensions(cdfObject, *json, false);

  addCDFInfo(cdfObject, NULL, dfBBOX, features, false);

  addPropertyVariables(cdfObject, features);

  CT::string dumpString = CDF::dump(cdfObject);

  std::string geojsonkey = jsonVar->getAttributeNE("ADAGUC_BASENAME")->toString().c_str();
  featureStore[geojsonkey] = features;

  json_value_free(json);
#ifdef MEASURETIME
  StopWatch_Stop("DATA READ");
#endif

#ifdef MEASURETIME
  StopWatch_Stop("BBOX Calculated");
#endif

#ifdef CCONVERTGEOJSON_DEBUG
  CDBDebug("CConvertGeoJSON::convertGeoJSONHeader() done");
#endif
  return 0;
}

void CConvertGeoJSON::addCDFInfo(CDFObject *cdfObject, CServerParams *, BBOX &dfBBOX, std::vector<Feature *> &featureMap, bool) {
  // Create variables for all properties fields

  // Default size of adaguc 2dField is 2x2
  int width = 2;
  int height = 2;

  double cellSizeX = (dfBBOX.urX - dfBBOX.llX) / double(width);
  double cellSizeY = (dfBBOX.urY - dfBBOX.llY) / double(height);
  double offsetX = dfBBOX.llX;
  double offsetY = dfBBOX.llY;

  // Add geo variables, only if they are not there already
  CDF::Dimension *dimX = cdfObject->getDimensionNE("x");
  CDF::Dimension *dimY = cdfObject->getDimensionNE("y");
  CDF::Variable *varX = cdfObject->getVariableNE("x");
  CDF::Variable *varY = cdfObject->getVariableNE("y");

  CDF::Dimension *timeDim = cdfObject->getDimensionNE("time");
  // CDBDebug("timeDim: %d", timeDim);
  CDF::Dimension *elevationDim = cdfObject->getDimensionNE("elevation");
  // CDBDebug("elevationDim: %d", elevationDim);

  if (dimX == NULL || dimY == NULL || varX == NULL || varY == NULL) {

    // If not available, create new dimensions and variables (X,Y,T)
#ifdef CCONVERTGEOJSON_DEBUG
    CDBDebug("CellsizeX: %f", cellSizeX);
#endif
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
    varX->setType(CDF_DOUBLE);
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

  CDF::Variable *polygonIndexVar = new CDF::Variable();
  cdfObject->addVariable(polygonIndexVar);

  if (elevationDim != NULL) {
    polygonIndexVar->dimensionlinks.push_back(elevationDim);
  }
  if (timeDim != NULL) {
    polygonIndexVar->dimensionlinks.push_back(timeDim);
  }
  polygonIndexVar->dimensionlinks.push_back(dimY);
  polygonIndexVar->dimensionlinks.push_back(dimX);
  polygonIndexVar->setType(CDF_USHORT);
  polygonIndexVar->name = "features";
  polygonIndexVar->setAttributeText("long_name", "Feature index");
  polygonIndexVar->setAttributeText("adaguc_data_type", "CConvertGeoJSON");

  unsigned short f = CCONVERTGEOJSON_FILL;
  polygonIndexVar->setAttribute("_FillValue", CDF_USHORT, &f, 1);

  int nrFeatures = featureMap.size();

  CDF::Dimension *dimFeatures;
  bool found = false;
  try {
    dimFeatures = cdfObject->getDimension("features");
    found = true;
  } catch (int e) {
  }
  if (!found) {
    // CDBDebug("Creating dim %s %d", "features", nrFeatures);
    dimFeatures = new CDF::Dimension();
    dimFeatures->name = "features";
    dimFeatures->setSize(nrFeatures);
    cdfObject->addDimension(dimFeatures);
  }
  CDF::Variable *featureIdVar = new CDF::Variable();

  try {
    cdfObject->getVariable("featureids");
    found = true;
  } catch (int e) {
  }
  if (!found) {
    cdfObject->addVariable(featureIdVar);
    featureIdVar->dimensionlinks.push_back(dimFeatures);
    if (elevationDim != NULL) {
      featureIdVar->dimensionlinks.push_back(elevationDim);
    }
    if (timeDim != NULL) {
      featureIdVar->dimensionlinks.push_back(timeDim);
    }
    featureIdVar->setType(CDF_STRING);
    featureIdVar->name = "featureids";
    CDF::allocateData(CDF_STRING, &featureIdVar->data, nrFeatures);
    featureIdVar->setSize(nrFeatures);
  }

  int featureCnt = 0;
  int numPoints = 0;
  int numPolys = 0;
  for (auto &sample : featureMap) {
    ((const char **)featureIdVar->data)[featureCnt++] = strdup(sample->getId().c_str());
    numPoints += sample->getPoints()->size();
    numPolys += sample->getPolygons()->size();
  }
  if (numPoints > numPolys) {
    polygonIndexVar->setAttributeText("adaguc_data_type", "CConvertGeoJSONPOINT");
  }
  if (numPoints < numPolys) {
    polygonIndexVar->setAttributeText("adaguc_data_type", "CConvertGeoJSONPOLYGON");
  }
}

void CConvertGeoJSON::getDimensions(CDFObject *cdfObject, json_value &json, bool) {
  if (json.type == json_object) {
    CT::string type;
    if (json["type"].type != json_null) {
#ifdef CCONVERTGEOJSON_DEBUG
      CDBDebug("type found");
#endif
      type = json["type"].u.string.ptr;
#ifdef CCONVERTGEOJSON_DEBUG
      CDBDebug("type: %s\n", type.c_str());
#endif
      CT::string timeVal;
      double dTimeVal = -9999;
      int iTimeVal = -9999;
      CT::string timeUnits;

      if (type.equals("FeatureCollection")) {
        json_value dimensions = json["dimensions"];
        if (dimensions.type == json_object) {
          for (unsigned int dimCnt = 0; dimCnt < dimensions.u.object.length; dimCnt++) {
            json_object_entry dimObject = dimensions.u.object.values[dimCnt];
            CT::string dimName(dimObject.name);
            json_value dim = *dimObject.value;
            // CDBDebug("[%d] dim[%s] %d %d", dimCnt, dimName.c_str(), dim.type, dim.type==json_string);
            if (dimName.equals("time")) {
              // CDBDebug("time found !!!!");

              if (dim.type == json_object) {
                for (unsigned int fldCnt = 0; fldCnt < dim.u.object.length; fldCnt++) {
                  json_object_entry fldObject = dim.u.object.values[fldCnt];
                  CT::string fldName(fldObject.name);
                  json_value fldValue = *fldObject.value;
                  if (fldValue.type == json_string) {
                    CT::string value(fldValue.u.string.ptr);
                    // CDBDebug("[ ] dim[%s]: %s=%s", dimName.c_str(), fldName.c_str(), value.c_str());
                    //                      CDBDebug("[%d] prop[%s]=%s", cnt, propName.c_str(), prop.u.string.ptr);
                    //                      CDBDebug("[%d] prop[%s]S =%s", cnt, propName.c_str(),prop.u.string.ptr);
                    if (fldName.equals("units")) {
                      timeUnits = value.c_str();
                    } else if (fldName.equals("value")) {
                      timeVal = value.c_str();
                    }
                  }
                  if (fldValue.type == json_double) {
                    // CDBDebug("[ ] dim[%s]: dbl", dimName.c_str(), fldName.c_str());
                    if (fldName.equals("value")) {
                      dTimeVal = fldValue.u.dbl;
                    }
                  }
                  if (fldValue.type == json_integer) {
                    if (fldName.equals("value")) {
                      iTimeVal = fldValue.u.integer;
                    }
                  }
                }
              }

              // CDBDebug("time: %s %s %f %d", timeVal.c_str(), timeUnits.c_str(), dTimeVal, iTimeVal);
              CDF::Variable timeVarHelper;
              timeVarHelper.setAttributeText("units", "seconds since 1970-1-1");
              CTime *timeHelper = CTime::GetCTimeInstance(&timeVarHelper);
              if (timeHelper == nullptr) {
                CDBError(CTIME_GETINSTANCE_ERROR_MESSAGE);
                throw __LINE__;
              }
              double timeOffset;
              if (timeVal.length() > 0) {
                timeOffset = timeHelper->dateToOffset(timeHelper->freeDateStringToDate(timeVal.c_str()));
              } else if (dTimeVal > 0) {
                timeOffset = dTimeVal;
              } else if (iTimeVal > 0) {
                timeOffset = iTimeVal;
              } else {
                timeOffset = 0.0;
                CDBDebug("CConvertGeoJSON::getDimensions: WARNING: timeOffset set to 0.0");
              }
              // CDBDebug("timeOffset=%f", timeOffset);
              CDF::Dimension *timeDim = new CDF::Dimension();
              timeDim->name = "time";
              timeDim->setSize(1);
              cdfObject->addDimension(timeDim);
              CDF::Variable *timeVar = new CDF::Variable();
              timeVar->setType(CDF_DOUBLE);
              timeVar->name.copy("time");
              timeVar->isDimension = true;
              timeVar->setAttributeText("units", "seconds since 1970-1-1");
              timeVar->setAttributeText("standard_name", "time");
              timeVar->dimensionlinks.push_back(timeDim);
              cdfObject->addVariable(timeVar);
              CDF::allocateData(CDF_DOUBLE, &timeVar->data, timeDim->length);
              timeVar->setType(CDF_DOUBLE);
              ((double *)timeVar->data)[0] = timeOffset;
            } else {
              // CDBDebug("other dim: %s", dimName.c_str());
              CT::string dimUnits;
              CT::string dimVal;
              double dDimVal = 0.0;

              if (dim.type == json_object) {
                for (unsigned int fldCnt = 0; fldCnt < dim.u.object.length; fldCnt++) {
                  json_object_entry fldObject = dim.u.object.values[fldCnt];
                  CT::string fldName(fldObject.name);
                  json_value fldValue = *fldObject.value;
                  if (fldValue.type == json_string) {
                    CT::string value(fldValue.u.string.ptr);
                    // CDBDebug("[ ] dim[%s]: %s=%s", dimName.c_str(), fldName.c_str(), value.c_str());
                    //                      CDBDebug("[%d] prop[%s]=%s", cnt, propName.c_str(), prop.u.string.ptr);
                    //                      CDBDebug("[%d] prop[%s]S =%s", cnt, propName.c_str(),prop.u.string.ptr);
                    if (fldName.equals("units")) {
                      dimUnits = value.c_str();
                    } else if (fldName.equals("value")) {
                      dimVal = value.c_str();
                    }
                  }
                  if (fldValue.type == json_double) {
                    // CDBDebug("[ ] dim[%s]: dbl", dimName.c_str(), fldName.c_str());
                    if (fldName.equals("value")) {
                      dDimVal = fldValue.u.dbl;
                    }
                  }
                }
              }

              CDF::Dimension *dim = new CDF::Dimension();
              dim->name.copy(dimName);
              dim->setSize(1);
              cdfObject->addDimension(dim);
              CDF::Variable *dimVar = new CDF::Variable();
              dimVar->setType(CDF_DOUBLE);
              dimVar->name.copy(dimName);
              dimVar->isDimension = true;
              dimVar->setAttributeText("units", dimUnits.c_str());
              dimVar->setAttributeText("standard_name", dimName.c_str());
              dimVar->dimensionlinks.push_back(dim);
              // CDBDebug("Pushed_back %s dim", dim->name.c_str());
              cdfObject->addVariable(dimVar);
              CDF::allocateData(CDF_DOUBLE, &dimVar->data, dim->length);
              dimVar->setType(CDF_DOUBLE);
              ((double *)dimVar->data)[0] = dDimVal;

#ifdef USETHIS
              CDF::Variable timeVarHelper;
              timeVarHelper.setAttributeText("units", "seconds since 1970-1-1");
              CTime *timeHelper = CTime::GetCTimeInstance(&timeVarHelper);

              double timeOffset;
              if (timeVal.length() > 0) {
                timeOffset = timeHelper->dateToOffset(timeHelper->freeDateStringToDate(timeVal.c_str()));
              } else if (dTimeVal > 0) {
                timeOffset = dTimeVal;
              } else if (iTimeVal > 0) {
                timeOffset = iTimeVal;
              }
              CDBDebug("timeOffset=%f", timeOffset);
              CDF::Dimension *timeDim = new CDF::Dimension();
              timeDim->name = "time";
              timeDim->setSize(1);
              cdfObject->addDimension(timeDim);
              CDF::Variable *timeVar = new CDF::Variable();
              timeVar->setType(CDF_DOUBLE);
              timeVar->name.copy("time");
              timeVar->isDimension = true;
              timeVar->setAttributeText("units", "seconds since 1970-1-1");
              timeVar->setAttributeText("standard_name", "time");
              timeVar->dimensionlinks.push_back(timeDim);
              cdfObject->addVariable(timeVar);
              CDF::allocateData(CDF_DOUBLE, &timeVar->data, timeDim->length);
              timeVar->setType(CDF_DOUBLE);
              ((double *)timeVar->data)[0] = timeOffset;
#endif
            }
          }
        }
      }
    }
  }
}

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

void CConvertGeoJSON::getBBOX(CDFObject *, BBOX &bbox, json_value &json, std::vector<Feature *> &featureMap) {
  double minLat = 90., maxLat = -90., minLon = 180, maxLon = -180;
  bool BBOXFound = false;
  if (json.type == json_object) {
    CT::string type;
    if (json["type"].type != json_null) {
#ifdef CCONVERTGEOJSON_DEBUG
      CDBDebug("type found");
#endif
      type = json["type"].u.string.ptr;
#ifdef CCONVERTGEOJSON_DEBUG
      CDBDebug("type: %s\n", type.c_str());
#endif
      if (type.equals("FeatureCollection")) {
        json_value bbox_v = json["bbox"];
        if (bbox_v.type == json_array) {
          bbox.llX = (double)(*bbox_v.u.array.values[0]);
          bbox.llY = (double)(*bbox_v.u.array.values[1]);
          bbox.urX = (double)(*bbox_v.u.array.values[2]);
          bbox.urY = (double)(*bbox_v.u.array.values[3]);
          BBOXFound = true;
        }
      }
      json_value features = json["features"];
      if (features.type == json_array) {
#ifdef CCONVERTGEOJSON_DEBUG
        CDBDebug("features found");
#endif
        for (unsigned int cnt = 0; cnt < features.u.array.length; cnt++) {
          json_value feature = *features.u.array.values[cnt];
          CT::string featureId;
          json_value id = feature["id"];
          if (id.type == json_string) {
            //                  CDBDebug("Id=%s", id.u.string.ptr);
            featureId = id.u.string.ptr;
          } else if (id.type == json_integer) {
            featureId.print("%1d", id.u.integer);
          }
          //                CDBDebug("found featureId as attribute %s", featureId.c_str());
          Feature *feat = new Feature();

          json_value props = feature["properties"];
          //                CDBDebug("props.type=%d", props.type);
          if (props.type == json_object) {
            for (unsigned int propCnt = 0; propCnt < props.u.object.length; propCnt++) {
              json_object_entry propObject = props.u.object.values[propCnt];
              CT::string propName(propObject.name);
              json_value prop = *propObject.value;
              if (prop.type == json_string) {
                //                      CDBDebug("[%d] prop[%s]=%s", cnt, propName.c_str(), prop.u.string.ptr);
                //                      CDBDebug("[%d] prop[%s]S =%s", cnt, propName.c_str(),prop.u.string.ptr);
                feat->addProp(propName, prop.u.string.ptr);
              } else if (prop.type == json_integer) {
                // CDBDebug("[%d] prop[%s]I =%ld", cnt, propName.c_str(), prop.u.integer);
                feat->addPropInt64(propName, (int64_t)prop.u.integer);
              } else if (prop.type == json_double) {
                // CDBDebug("[%d] prop[%s]D =%f", cnt, propName.c_str(), prop.u.dbl);
                feat->addProp(propName, prop.u.dbl);
              }
            }
          }
          if (featureId.length() == 0) {
            std::map<std::string, FeatureProperty *>::iterator it;
            CT::string id_s;
            std::map<std::string, FeatureProperty *> *featurePropertyMap = feat->getFp();
            it = featurePropertyMap->find("id");
            if (it != featurePropertyMap->end()) {
              id_s = it->second->toString().c_str();
              //                     CDBDebug("Found %s %s", it->first.c_str(), id_s.c_str());
              if (!id_s.equals("NONE")) {
                featureId = id_s;
              }
            } else {
              it = featurePropertyMap->find("NUTS_ID");
              if (it != featurePropertyMap->end()) {
                id_s = it->second->toString().c_str();
                //                       CDBDebug("Found %s %s", it->first.c_str(), id_s.c_str());
                if (!id_s.equals("NONE")) {
                  featureId = id_s;
                }
              } else {
                it = featurePropertyMap->find("name");
                if (it != featurePropertyMap->end()) {
                  id_s = it->second->toString().c_str();
                  //                         CDBDebug("Found %s %s", it->first.c_str(), id_s.c_str());
                  if (!id_s.equals("NONE")) {
                    featureId = id_s;
                  }
                } else {
                  //                        CDBDebug("Fallback to id %d", cnt);
                  featureId.print("%04d", cnt);
                }
              }
              //                    CDBDebug("found featureId in properties %s", featureId.c_str());
            }
          }
          feat->setId(featureId);

          json_value geom = feature["geometry"];
          if (geom.type == json_object) {
            json_value geomType = geom["type"];
            if (geomType.type == json_string) {
              //                    CDBDebug("geomType: %s", geomType.u.string.ptr);
              if (strcmp(geomType.u.string.ptr, "Polygon") == 0) {
                json_value coords = geom["coordinates"];
                if (coords.type == json_array) {
                  //                        CDBDebug("  array of %d coords",coords.u.array.length);
                  for (unsigned int j = 0; j < coords.u.array.length; j++) {
                    if (j == 0) {
                      feat->newPolygon();
                    } else {
                      feat->newHole();
                    }
                    json_value polygon = *coords.u.array.values[j];
                    //                          CDBDebug("polygon: %d", polygon.u.array.length);
                    for (unsigned int i = 0; i < polygon.u.array.length; i++) {
                      json_value pt = *polygon.u.array.values[i];
                      json_value lo = *pt.u.array.values[0];
                      json_value la = *pt.u.array.values[1];
                      double lon = (double)lo;
                      double lat = (double)la;
                      if (j == 0) {
                        feat->addPolygonPoint((float)lon, (float)lat);
                      } else {
                        feat->addHolePoint((float)lon, (float)lat);
                      }
                      if (lat < minLat) minLat = lat;
                      if (lat > maxLat) maxLat = lat;
                      if (lon < minLon) minLon = lon;
                      if (lon > maxLon) maxLon = lon;
                      //                            CDBDebug("    %f,%f", (double)lo, (double)la);
                    }
                  }
                } else {
                  //                        CDBDebug("  coords type of %d", coords.type);
                }
              } else if (strcmp(geomType.u.string.ptr, "LineString") == 0) {
                json_value coords = geom["coordinates"];
                if (coords.type == json_array) {
                  feat->newPolyline();
                  for (unsigned int i = 0; i < coords.u.array.length; i++) {
                    json_value pt = *coords.u.array.values[i];
                    json_value lo = *pt.u.array.values[0];
                    json_value la = *pt.u.array.values[1];
                    double lon = (double)lo;
                    double lat = (double)la;
                    feat->addPolylinePoint((float)lon, (float)lat);
                    if (lat < minLat) minLat = lat;
                    if (lat > maxLat) maxLat = lat;
                    if (lon < minLon) minLon = lon;
                    if (lon > maxLon) maxLon = lon;
                    //                          CDBDebug("    %f,%f", (double)lo, (double)la);
                  }

                } else {
                  CDBDebug("  coords type of %d", coords.type);
                }
              } else if (strcmp(geomType.u.string.ptr, "MultiLineString") == 0) {
                json_value multicoords = geom["coordinates"];
                if (multicoords.type == json_array) {
                  //                            CDBDebug("  array of %d coords",coords.u.array.length);
                  for (unsigned int j = 0; j < multicoords.u.array.length; j++) {
                    json_value coords = *multicoords.u.array.values[j];
                    if (coords.type == json_array) {
                      feat->newPolyline();
                      for (unsigned int i = 0; i < coords.u.array.length; i++) {
                        json_value pt = *coords.u.array.values[i];
                        json_value lo = *pt.u.array.values[0];
                        json_value la = *pt.u.array.values[1];
                        double lon = (double)lo;
                        double lat = (double)la;
                        feat->addPolylinePoint((float)lon, (float)lat);
                        if (lat < minLat) minLat = lat;
                        if (lat > maxLat) maxLat = lat;
                        if (lon < minLon) minLon = lon;
                        if (lon > maxLon) maxLon = lon;
                        //                          CDBDebug("    %f,%f", (double)lo, (double)la);
                      }
                    }
                  }
                } else {
                  CDBDebug("  multicoords type of %d", multicoords.type);
                }
              } else if (strcmp(geomType.u.string.ptr, "MultiPolygon") == 0) {
                json_value multicoords = geom["coordinates"];
                if (multicoords.type == json_array) {
                  //                        CDBDebug("  array of %d coords",multicoords.u.array.length);
                  for (unsigned int i = 0; i < multicoords.u.array.length; i++) {
                    json_value coords = *multicoords.u.array.values[i];
                    if (coords.type == json_array) {
                      //                            CDBDebug("  array of %d coords",coords.u.array.length);
                      for (unsigned int j = 0; j < coords.u.array.length; j++) {
                        if (j == 0) {
                          feat->newPolygon();
                        } else {
                          feat->newHole();
                        }
                        json_value polygon = *coords.u.array.values[j];
                        //                              CDBDebug("polygon: %d", polygon.u.array.length);
                        for (unsigned int k = 0; k < polygon.u.array.length; k++) {
                          json_value pt = *polygon.u.array.values[k];
                          json_value lo = *pt.u.array.values[0];
                          json_value la = *pt.u.array.values[1];
                          double lon = (double)lo;
                          double lat = (double)la;
                          if (j == 0) {
                            feat->addPolygonPoint((float)lon, (float)lat);
                          } else {
                            feat->addHolePoint((float)lon, (float)lat);
                          }
                          if (lat < minLat) minLat = lat;
                          if (lat > maxLat) maxLat = lat;
                          if (lon < minLon) minLon = lon;
                          if (lon > maxLon) maxLon = lon;
                          //                              CDBDebug("    %f,%f", (double)lo, (double)la);
                        }
                      }
                    } else {
                      //                            CDBDebug("  coords type of %d", coords.type);
                    }
                  }
                }
              } else if (strcmp(geomType.u.string.ptr, "Point") == 0) { // TODO is this useful?
                json_value coords = geom["coordinates"];
                if (coords.type == json_array) {
                  json_value lo = *coords.u.array.values[0];
                  json_value la = *coords.u.array.values[1];
                  double lon = (double)lo;
                  double lat = (double)la;
                  feat->addPoint(lon, lat);
                }
              }
            }
          }
          // CDBDebug("FEAT %s", feat->toString().c_str());
          featureMap.push_back(feat);
        }
      }
    }
  }

  //         CDBDebug("<><><><><><><>Cleaning up for all properties fields<><><><><><>");
  //         int itctr=0;
  //         for (std::vector<Feature*>::iterator it = featureMap.begin(); it != featureMap.end(); ++it) {
  // //          CDBDebug("FT[%d] has %d items", itctr, (*it)->getFp().size());
  //           for (std::map<std::string, FeatureProperty*>::iterator ftit=(*it)->getFp().begin(); ftit!=(*it)->getFp().end(); ++ftit) {
  // //            CDBDebug("FT: %d %s %s", itctr, ftit->first.c_str(), ftit->second->toString().c_str());
  //             delete ftit->second;
  //           }
  //           (*it)->getFp().clear();
  //           delete *it;
  //           itctr++;
  //         }
  //         featureMap.clear();
  // If no BBOX was found in file, generate it from found geo coordinates
  if (!BBOXFound) {
    bbox.llX = minLon;
    bbox.llY = minLat;
    bbox.urX = maxLon;
    bbox.urY = maxLat;
  }
#ifdef CCONVERTGEOJSON_DEBUG
  CDBDebug("BBOX: %f,%f,%f,%f", bbox.llX, bbox.llY, bbox.urX, bbox.urY);
#endif
  return;
}

std::vector<CDF::Dimension *> getVarDimensions(CDFObject *cdfObject) {
  std::vector<CDF::Dimension *> dims;

  for (CDF::Dimension *dim : cdfObject->dimensions) {
    // CDataReader::DimensionType dtyp = CDataReader::getDimensionType(cdfObject, dim->getName());
    CDataReader::DimensionType dtyp = CDataReader::dtype_normal;
    switch (dtyp) {
    case CDataReader::dtype_reference_time:
    case CDataReader::dtype_time:
      dims.push_back(dim);
      break;
    default:
      break;
    }
  }
  CDF::Dimension *dimy = cdfObject->getDimension("y");
  dims.push_back(dimy);
  CDF::Dimension *dimx = cdfObject->getDimension("x");
  dims.push_back(dimx);
  return dims;
}

size_t getDimensionSize(CDFObject *cdfObject) {
  size_t size = 1;

  CDF::Dimension *dimx = cdfObject->getDimension("x");
  size = size * dimx->getSize();
  CDF::Dimension *dimy = cdfObject->getDimension("y");
  size = size * dimy->getSize();

  for (CDF::Dimension *dim : cdfObject->dimensions) {
    CDataReader::DimensionType dtyp = CDataReader::getDimensionType(cdfObject, dim->getName());
    switch (dtyp) {
    case CDataReader::dtype_reference_time:
    case CDataReader::dtype_time:
      size = size * dim->length;
      break;
    default:
      break;
    }
  }
  return size;
}

int CConvertGeoJSON::addPropertyVariables(CDFObject *cdfObject, std::vector<Feature *> features) {
#ifdef CCONVERTGEOJSON_DEBUG
  CDBDebug("Adding propertyVariables");
#endif
  std::vector<Feature *> pointFeatures = getPointFeatures(features);
  std::map<CT::string, CDF::Variable *> newVars;

  std::vector<CDF::Dimension *> varDims = getVarDimensions(cdfObject);

  for (Feature *feature : pointFeatures) {
    // std::vector<GeoPoint> *pts = feature->getPoints();
    std::map<std::string, FeatureProperty *> *featurePropertyMap = feature->getFp();
    for (auto iter = featurePropertyMap->begin(); iter != featurePropertyMap->end(); ++iter) {
      CT::string name = iter->first.c_str();
      if (newVars.find(name.c_str()) == newVars.end() && cdfObject->getVariableNE(name.c_str()) == NULL) {
#ifdef CCONVERTGEOJSON_DEBUG
        CDBDebug("Creating var %s", name.c_str());
#endif
        CDF::Variable *newVar = new CDF::Variable();
        newVar->name.copy(name.c_str());
        switch (iter->second->getType()) {
        case typeInt:
          newVar->setType(CDF_FLOAT);
          newVar->currentType = newVar->nativeType = CDF_FLOAT;
          break;
        case typeDouble:
          newVar->setType(CDF_FLOAT);
          newVar->currentType = newVar->nativeType = CDF_FLOAT;
          break;
        case typeStr:
          newVar->setType(CDF_FLOAT);
          newVar->currentType = newVar->nativeType = CDF_FLOAT;
          break;
        case typeNone:
          break;
        }
        newVar->isDimension = false;
        for (auto it = std::begin(varDims); it != std::end(varDims); ++it) {
          newVar->dimensionlinks.push_back(*it);
        }
        newVar->setAttributeText("standard_name", name);
        newVar->setAttributeText("grid_mapping", "projection");
        cdfObject->addVariable(newVar);
#ifdef CCONVERTGEOJSON_DEBUG
        CDBDebug("adding variable %s", name.c_str());
#endif
        newVars[name] = newVar;
      }
    }
  }
#ifdef CCONVERTGEOJSON_DEBUG
  CDBDebug("/Adding propertyVariables");
#endif
  return 0;
}

int CConvertGeoJSON::convertGeoJSONData(CDataSource *dataSource, int mode) {
  // Get jsondata, parse into a Feature map with id as key
  CDF::Variable *jsonVar;
  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
  try {
    jsonVar = cdfObject->getVariable("jsoncontent");
  } catch (int e) {
    return 1;
  }

  // CDBDebug("convertGEOJSONData %s", (mode == CNETCDFREADER_MODE_OPEN_ALL) ? "ALL" : "NOT ALL");
  int result = 0;

  // Check whether this is really an geojson file
  try {
    cdfObject->getVariable("features"); // TODO generate these again
  } catch (int e) {
    return 1;
  }

  std::string geojsonkey = jsonVar->getAttributeNE("ADAGUC_BASENAME")->toString().c_str();
  std::vector<Feature *> features = featureStore[geojsonkey];

  if (features.size() == 0) {
#ifdef CCONVERTGEOJSON_DEBUG
    CDBDebug("Rereading JSON");
#endif
    CT::string inputjsondata = (char *)jsonVar->data;
    json_value *json = json_parse((json_char *)inputjsondata.c_str(), inputjsondata.length());
#ifdef CCONVERTGEOJSON_DEBUG
    CDBDebug("JSON result: %x", json);
#endif

    BBOX dfBBOX;
    getBBOX(cdfObject, dfBBOX, *json, features);
#ifdef CCONVERTGEOJSON_DEBUG
    CDBDebug("addCDFInfo again");
#endif
    addCDFInfo(cdfObject, dataSource->srvParams, dfBBOX, features, true);
  }

  // Store featureSet name (geojsonkey) in datasource
  dataSource->featureSet = geojsonkey.c_str();

  // Make the width and height of the new 2D adaguc field the same as the viewing window
  dataSource->dWidth = dataSource->srvParams->geoParams.width;
  dataSource->dHeight = dataSource->srvParams->geoParams.height;

  // Set statistics
  if (dataSource->stretchMinMax) {
#ifdef CCONVERTGEOJSON_DEBUG
    CDBDebug("dataSource->stretchMinMax");
#endif
    if (dataSource->statistics == NULL) {
#ifdef CCONVERTGEOJSON_DEBUG
      CDBDebug("Setting statistics: min/max : %d %d", 0, features.size() - 1);
#endif
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->setMaximum(features.size() - 1);
      dataSource->statistics->setMinimum(0);
    }
  }

  size_t nrDataObjects = dataSource->getNumDataObjects();

  if (dataSource->srvParams->requestType == REQUEST_WMS_GETLEGENDGRAPHIC || (dataSource->dWidth == 1 && dataSource->dHeight == 1)) {
    if (dataSource->stretchMinMax == false || (nrDataObjects > 0 && dataSource->getDataObject(0)->variableName.equals("features") == true)) {
      // CDBDebug("Returning because of REQUEST_WMS_GETLEGENDGRAPHIC and  dataSource->stretchMinMax is set to false or variable name is features");
      dataSource->srvParams->geoParams.bbox.toArray(dataSource->dfBBOX);
      return 0;
    }
  }
#ifdef CCONVERTGEOJSON_DEBUG
  if (nrDataObjects > 0) {
    CDBDebug("Working on %s", dataSource->getDataObject(0)->variableName.c_str());
  }
#endif

  if (mode == CNETCDFREADER_MODE_OPEN_ALL) {
#ifdef CCONVERTGEOJSON_DEBUG
    CDBDebug("convertGeoJSONData OPEN ALL");
#endif
    // CDBDebug("convertGeoJSONData OPEN ALL (*)");

    CDataSource::DataObject *dataObject;
    for (size_t d = 0; d < nrDataObjects; d++) {
      dataObject = dataSource->getDataObject(d);

      CDF::Variable *polygonIndexVar;
      polygonIndexVar = dataObject->cdfVariable;
      // Width needs to be at least 2 in this case.
      if (dataSource->dWidth == 1) dataSource->dWidth = 2;
      if (dataSource->dHeight == 1) dataSource->dHeight = 2;
      double cellSizeX = dataSource->srvParams->geoParams.bbox.span().x / dataSource->dWidth;
      double cellSizeY = dataSource->srvParams->geoParams.bbox.span().y / dataSource->dHeight;
      double offsetX = dataSource->srvParams->geoParams.bbox.left + cellSizeX / 2;
      double offsetY = dataSource->srvParams->geoParams.bbox.bottom + cellSizeY / 2;

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
        double x = offsetX + double(j) * cellSizeX;
        ((double *)varX->data)[j] = x;
      }
      for (size_t j = 0; j < dimY->length; j++) {
        double y = offsetY + double(j) * cellSizeY;
        ((double *)varY->data)[j] = y;
      }
      bool projectionRequired = false;
      CImageWarper imageWarper;
      if (dataSource->srvParams->geoParams.crs.length() > 0) {
        projectionRequired = true;
        //            for(size_t d=0;d<nrDataObjects;d++){
        polygonIndexVar->setAttributeText("grid_mapping", "customgridprojection");
        //            }
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

#ifdef CCONVERTGEOJSON_DEBUG
      CDBDebug("Drawing %s", polygonIndexVar->name.c_str());
#endif

      // Allocate data for the 2D field
      size_t fieldSize = dataSource->dWidth * dataSource->dHeight;
      polygonIndexVar->setSize(fieldSize);
      CDF::allocateData(polygonIndexVar->getType(), &(polygonIndexVar->data), fieldSize);

      // Determine the fillvalue
      dataObject->dfNodataValue = CCONVERTGEOJSON_FILL;

      CDF::Attribute *fillValue = polygonIndexVar->getAttributeNE("_FillValue");
      if (fillValue == NULL) {
        polygonIndexVar->setAttribute("_FillValue", polygonIndexVar->getType(), dataObject->dfNodataValue);
        fillValue = polygonIndexVar->getAttributeNE("_FillValue");
        // CDBDebug("Setting fill value to %f", dataObject->dfNodataValue);
      }

      dataObject->hasNodataValue = true;
      fillValue->getData(&dataObject->dfNodataValue, 1);

      // Fill the data with the nodatavalue
      CDF::fill(polygonIndexVar->data, polygonIndexVar->getType(), dataObject->dfNodataValue, fieldSize);

#ifdef MEASURETIME
      StopWatch_Stop("GeoJSON DATA");
#endif

#ifdef CCONVERTGEOJSON_DEBUG
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

#ifdef MEASURETIME
      StopWatch_Stop("Iterating lat/lon data");
#endif

#ifdef MEASURETIME
      StopWatch_Stop("Feature drawing starts");
#endif
      // CDBDebug("nrFeatures: %d", features.size());

      unsigned short int featureIndex = 0;
      float min = NAN;
      float max = NAN;

      for (auto feature = features.begin(); feature != features.end(); ++feature) { // Loop over all features
        std::map<std::string, FeatureProperty *> *featurePropertyMap = (*feature)->getFp();
        drawPolygons(*feature, featureIndex, dataSource, projectionRequired, &imageWarper, cellSizeX, cellSizeY, offsetX, offsetY);
        drawPoints(*feature, featureIndex, dataSource, projectionRequired, &imageWarper, cellSizeX, cellSizeY, offsetX, offsetY, min, max);

        for (std::map<std::string, FeatureProperty *>::iterator ftit = featurePropertyMap->begin(); ftit != featurePropertyMap->end(); ++ftit) {
          if (dataObject->features.count(featureIndex) == 0) {
            dataObject->features[featureIndex] = CFeature(featureIndex);
          }
          dataObject->features[featureIndex].addProperty(ftit->first.c_str(), ftit->second->toString().c_str());
        }
        featureIndex++;
      }
      if (dataSource && dataSource->statistics != NULL) {
        if (dataObject->variableName.equals("features") == false) {
          if (min != NAN) dataSource->statistics->setMinimum(min);
          if (max != NAN) dataSource->statistics->setMaximum(max);
        }
      }

#ifdef MEASURETIME
      StopWatch_Stop("Feature drawing done");
#endif
#ifdef CCONVERTGEOJSON_DEBUG
      CDBDebug("/convertGEOJSONData");
#endif
    }
  }
  return result;
}

void CConvertGeoJSON::drawDot(int px, int py, unsigned short v, int W, int H, unsigned short *grid) {
  for (int x = -4; x < 6; x++) {
    for (int y = -4; y < 6; y++) {
      int pointX = px + x;
      int pointY = py + y;
      if (pointX >= 0 && pointY >= 0 && pointX < W && pointY < H) grid[pointX + pointY * W] = v;
    }
  }
}

void CConvertGeoJSON::drawPolygons(Feature *feature, unsigned short int featureIndex, CDataSource *dataSource, bool projectionRequired, CImageWarper *imageWarper, double cellSizeX, double cellSizeY,
                                   double offsetX, double offsetY) {
  std::vector<Polygon> *polygons = feature->getPolygons();
  if (polygons->size() == 0) return;
  CDF::Variable *polygonIndexVar = dataSource->getDataObject(0)->cdfVariable;

  for (std::vector<Polygon>::iterator itpoly = polygons->begin(); itpoly != polygons->end(); ++itpoly) {
    float *polyX = itpoly->getLons();
    float *polyY = itpoly->getLats();
    int numPolygonPoints = itpoly->getSize();
    float *projectedXY = new float[numPolygonPoints * 2];
    float minX = FLT_MAX, minY = FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX;

    int pxMin, pxMax, pyMin, pyMax, first = 0;

    for (int j = 0; j < numPolygonPoints; j++) {
      double tprojectedX = polyX[j];
      double tprojectedY = polyY[j];
      int status = 0;
      if (projectionRequired) status = imageWarper->reprojfromLatLon(tprojectedX, tprojectedY);
      int dlon = 0, dlat = 0;
      if (!status) {
        if (cellSizeX != 0 && cellSizeY != 0) {
          dlon = int((tprojectedX - offsetX) / cellSizeX) + 1;
          dlat = int((tprojectedY - offsetY) / cellSizeY);
        }

        if (first == 0) {
          first = 1;
          pxMin = dlon;
          pxMax = dlon;
          pyMin = dlat;
          pyMax = dlat;
        } else {
          if (dlon < pxMin) pxMin = dlon;
          if (dlon > pxMax) pxMax = dlon;
          if (dlat < pyMin) pyMin = dlat;
          if (dlat > pyMax) pyMax = dlat;
        }

        minX = MIN(minX, tprojectedX);
        minY = MIN(minY, tprojectedY);
        maxX = MAX(maxX, tprojectedX);
        maxY = MAX(maxY, tprojectedY);
      } else {
        dlat = CCONVERTGEOJSONCOORDS_NODATA;
        dlon = CCONVERTGEOJSONCOORDS_NODATA;
      }
      projectedXY[j * 2] = dlon;
      projectedXY[j * 2 + 1] = dlat;
    }

    std::vector<PointArray> holes = itpoly->getHoles();
    int nrHoles = holes.size();

    Hole *holeArray = new Hole[nrHoles];

    int h = 0;
    for (std::vector<PointArray>::iterator itholes = holes.begin(); itholes != holes.end(); ++itholes) {
      holeArray[h].holeX = itholes->getLons();
      holeArray[h].holeY = itholes->getLats();
      holeArray[h].holeSize = itholes->getSize();
      holeArray[h].projectedHoleXY = new float[holeArray[h].holeSize * 2];
      for (int j = 0; j < holeArray[h].holeSize; j++) {
        double tprojectedX = holeArray[h].holeX[j];
        double tprojectedY = holeArray[h].holeY[j];
        int holeStatus = 0;
        if (projectionRequired) holeStatus = imageWarper->reprojfromLatLon(tprojectedX, tprojectedY);
        int dlon, dlat;
        if (!holeStatus) {
          dlon = int((tprojectedX - offsetX) / cellSizeX) + 1;
          dlat = int((tprojectedY - offsetY) / cellSizeY);
        } else {
          dlat = CCONVERTGEOJSONCOORDS_NODATA;
          dlon = CCONVERTGEOJSONCOORDS_NODATA;
        }
        holeArray[h].projectedHoleXY[j * 2] = dlon;
        holeArray[h].projectedHoleXY[j * 2 + 1] = dlat;
        // std::vector<GeoPoint> points = feature->getPoints();

#ifdef MEASURETIME
        StopWatch_Stop("Feature drawn %d", featureIndex);
#endif
      }
      h++;
    }

    int dpCount = numPolygonPoints;
    if (polygonIndexVar->getType() == CDF_USHORT) {
      unsigned short *sdata = (unsigned short *)polygonIndexVar->data;
      drawpolyWithHoles_index(pxMin, pyMin, pxMax, pyMax, sdata, dataSource->dWidth, dataSource->dHeight, dpCount, projectedXY, featureIndex, nrHoles, holeArray);
    }

    for (int h = 0; h < nrHoles; h++) {
      delete[] holeArray[h].projectedHoleXY;
    }
    delete[] holeArray;
    delete[] projectedXY;
  }
}

void CConvertGeoJSON::drawPoints(Feature *feature, unsigned short int, CDataSource *dataSource, bool projectionRequired, CImageWarper *imageWarper, double cellSizeX, double cellSizeY, double offsetX,
                                 double offsetY, float &min, float &max) {
  std::vector<GeoPoint> *points = feature->getPoints();
  if (points->size() == 0) return;
  size_t nrDataObjects = dataSource->getNumDataObjects();
  for (size_t d = 0; d < nrDataObjects; d++) {
    CDataSource::DataObject *dataObject = dataSource->getDataObject(d);
    CDF::Variable *pointGridVariable = dataObject->cdfVariable;

    std::map<std::string, FeatureProperty *> *fp = feature->getFp();

    CT::string pointValue, pointName, pointDescription;
    bool isString = false;
    std::map<std::string, FeatureProperty *>::iterator ftit = fp->find(pointGridVariable->name.c_str());
    if (ftit != fp->end()) {
      isString = (ftit->second->getType() == typeStr);
      pointValue = ftit->second->toString().c_str();
      pointName = pointGridVariable->name.c_str();
      pointDescription = pointName;
      CDF::Attribute *longName = pointGridVariable->getAttributeNE("long_name");
      if (longName) {
        pointDescription = (const char *)longName->data;
      }
    }

    for (std::vector<GeoPoint>::iterator itpoint = points->begin(); itpoint != points->end(); ++itpoint) {
      // Draw point
      double pointLongitude = itpoint->getLon();
      double pointLatitude = itpoint->getLat();
      double tprojectedX = pointLongitude;
      double tprojectedY = pointLatitude;
      int status = 0;
      if (projectionRequired) status = imageWarper->reprojfromLatLon(tprojectedX, tprojectedY);

      int dlon = 0, dlat = 0;
      if (!status && cellSizeX != 0 && cellSizeY != 0) {
        dlon = int((tprojectedX - offsetX) / cellSizeX) + 1;
        dlat = int((tprojectedY - offsetY) / cellSizeY);
      }
      float f = isString ? NAN : pointValue.toFloat();
      dataObject->points.push_back(PointDVWithLatLon(dlon, dlat, pointLongitude, pointLatitude, f));

      if (pointGridVariable->getType() == CDF_FLOAT) {
        if (f < min || min != min) min = f;
        if (f > max || max != max) max = f;
      }
      PointDVWithLatLon *lastPoint = &(dataObject->points.back());
      // Get the last pushed point from the array and push the character text data in the paramlist
      lastPoint->paramList.push_back({.key = pointName.c_str(), .description = pointDescription.c_str(), .value = pointValue.c_str()});
    }
  }
}