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

#include "Types/ProjectionStore.h"
#include "CImageWarper.h"
#include "ProjCache.h"
#include <iostream>
#include <vector>
#include <cmath>
const char *CImageWarper::className = "CImageWarper";

void floatToString(char *string, size_t maxlen, int numdigits, float number) {
  // snprintf(string,maxlen,"%0.2f",number);
  // return;
  if (numdigits > -3 && numdigits < 4) {
    if (numdigits <= -3) snprintf(string, maxlen, "%0.5f", float(floor(number * 100000.0 + 0.5) / 100000));
    if (numdigits == -2) snprintf(string, maxlen, "%0.4f", float(floor(number * 10000.0 + 0.5) / 10000));
    if (numdigits == -1) snprintf(string, maxlen, "%0.3f", float(floor(number * 1000.0 + 0.5) / 1000));
    if (numdigits == 0) snprintf(string, maxlen, "%0.3f", float(floor(number * 1000.0 + 0.5) / 1000));
    if (numdigits == 1) snprintf(string, maxlen, "%0.2f", float(floor(number * 100.0 + 0.5) / 100));
    if (numdigits == 2) snprintf(string, maxlen, "%0.1f", float(floor(number * 10.0 + 0.5) / 10));
    if (numdigits >= 3) snprintf(string, maxlen, "%0.1f", float(floor(number + 0.5)));
  } else
    snprintf(string, maxlen, "%0.3e", number);
}

void floatToString(char *string, size_t maxlen, float number) {
  int numdigits = 0;

  if (number == 0.0f)
    numdigits = 0;
  else {
    float tempp = number;
    if (tempp < 0.00000001 && tempp > -0.00000001) tempp += 0.00000001;
    numdigits = int(log10(fabs(tempp))) + 1;
  }
  floatToString(string, maxlen, numdigits, number);
}

void floatToString(char *string, size_t maxlen, float min, float max, float number) {
  float range = fabs(max - min);
  int digits = (int)log10(range) + 1;
  floatToString(string, maxlen, digits, number);
}

int CImageWarper::closereproj() {
  if (initialized) {
    // Nothing to do since we switched to caching the projections for performance
  }
  initialized = false;
  return 0;
}

int CImageWarper::reprojpoint(double &dfx, double &dfy) {
  // TODO: Should t all point to HUGE_VAL instead of 0.0?
  if (proj_trans_generic(projSourceToDest, PJ_INV, &dfx, sizeof(double), 1, &dfy, sizeof(double), 1, nullptr, 0, 0, nullptr, 0, 0) != 1) {
    // throw("reprojpoint error");
    return 1;
    // CDBError("ReprojException");
  }
  if (isnan(dfx) || isnan(dfy)) {
    dfx = 0;
    dfy = 0;
    return 1;
  }
  if (dfx == HUGE_VAL || dfy == HUGE_VAL) {
    dfx = 0;
    dfy = 0;
    return 1;
  }
  return 0;
}
int CImageWarper::reprojpoint(f8point &p) { return reprojpoint(p.x, p.y); }
int CImageWarper::reprojpoint_inv(f8point &p) { return reprojpoint_inv(p.x, p.y); }

int CImageWarper::reprojToLatLon(double &dfx, double &dfy) {
  if (proj_trans_generic(projLatlonToDest, PJ_INV, &dfx, sizeof(double), 1, &dfy, sizeof(double), 1, nullptr, 0, 0, nullptr, 0, 0) != 1) {
    // throw("reprojfromLatLon error");
    dfx = 0;
    dfy = 0;
    return 1;
  }
  return 0;
}

int CImageWarper::reprojfromLatLon(double &dfx, double &dfy) {
  if (dfx < -180 || dfx > 180 || dfy < -90 || dfy > 90) {
    dfx = 0;
    dfy = 0;
    return 1;
  }

  if (proj_trans_generic(projLatlonToDest, PJ_FWD, &dfx, sizeof(double), 1, &dfy, sizeof(double), 1, nullptr, 0, 0, nullptr, 0, 0) != 1) {
    // CDBError("Projection error");
    dfx = 0;
    dfy = 0;
    return 1;
  }
  if (isnan(dfx) || isnan(dfy)) {
    dfx = 0;
    dfy = 0;
    return 1;
  }
  if (dfx == HUGE_VAL || dfy == HUGE_VAL) {
    dfx = 0;
    dfy = 0;
    return 1;
  }
  // if(status!=0)CDBDebug("DestPJ: %s",GeoDest->CRS.c_str());
  return 0;
}

int CImageWarper::reprojModelToLatLon(double &dfx, double &dfy) {
  if (proj_trans_generic(projSourceToLatlon, PJ_FWD, &dfx, sizeof(double), 1, &dfy, sizeof(double), 1, nullptr, 0, 0, nullptr, 0, 0) != 1) {
    return 1;
  }
  return 0;
}

int CImageWarper::reprojModelToLatLon(f8point &point) {
  if (proj_trans_generic(projSourceToLatlon, PJ_FWD, &point.x, sizeof(double), 1, &point.y, sizeof(double), 1, nullptr, 0, 0, nullptr, 0, 0) != 1) {
    return 1;
  }
  return 0;
}

int CImageWarper::reprojModelFromLatLon(double &dfx, double &dfy) {

  if (proj_trans_generic(projSourceToLatlon, PJ_INV, &dfx, sizeof(double), 1, &dfy, sizeof(double), 1, nullptr, 0, 0, nullptr, 0, 0) != 1) {
    return 1;
  }
  // if(status!=0)CDBDebug("DestPJ: %s",GeoDest->CRS.c_str());
  return 0;
}

int CImageWarper::reprojpoint_inv_topx(double &dfx, double &dfy) {
  if (reprojpoint_inv(dfx, dfy) != 0) return 1;
  dfx = (dfx - _geoDest->dfBBOX[0]) / (_geoDest->dfBBOX[2] - _geoDest->dfBBOX[0]) * double(_geoDest->dWidth);
  dfy = (dfy - _geoDest->dfBBOX[3]) / (_geoDest->dfBBOX[1] - _geoDest->dfBBOX[3]) * double(_geoDest->dHeight);
  return 0;
}

int CImageWarper::reprojpoint_inv(double &dfx, double &dfy) {

  if (proj_trans_generic(projSourceToDest, PJ_FWD, &dfx, sizeof(double), 1, &dfy, sizeof(double), 1, nullptr, 0, 0, nullptr, 0, 0) != 1) {
    //  // CDBError("ReprojException: %f %f",dfx,dfy);
    dfx = 0;
    dfy = 0;
    return 1;
  }
  return 0;
}
//   int CImageWarper::decodeCRS(CT::string *outputCRS, CT::string *inputCRS){
//     return decodeCRS(outputCRS,inputCRS,prj);
//   }
int CImageWarper::decodeCRS(CT::string *outputCRS, CT::string *inputCRS, std::vector<CServerConfig::XMLE_Projection *> *prj) {
  if (prj == NULL) {
    CDBError("decodeCRS: prj==NULL");
    return 1;
  }
  if (&(*prj) == NULL) {
    CDBError("decodeCRS: prj==NULL");
    return 1;
  }
  outputCRS->copy(inputCRS);
  // if(inputCRS->indexOf("+proj")!=-1)return 0;
  dMaxExtentDefined = 0;
  // CDBDebug("Check");
  //  outputCRS->decodeURLSelf();
  // CDBDebug("Check");
  // CDBDebug("Check %d",(*prj).size());
  for (size_t j = 0; j < (*prj).size(); j++) {
    // CDBDebug("Check");
    if (outputCRS->equals((*prj)[j]->attr.id.c_str())) {
      outputCRS->copy((*prj)[j]->attr.proj4.c_str());
      // outputCRS->concat("+over");
      if ((*prj)[j]->LatLonBox.size() == 1) {
        // if(getMaxExtentBBOX!=NULL)
        {
          dMaxExtentDefined = 1;
          dfMaxExtent[0] = (*prj)[j]->LatLonBox[0]->attr.minx;
          dfMaxExtent[1] = (*prj)[j]->LatLonBox[0]->attr.miny;
          dfMaxExtent[2] = (*prj)[j]->LatLonBox[0]->attr.maxx;
          dfMaxExtent[3] = (*prj)[j]->LatLonBox[0]->attr.maxy;
        }
      }
      break;
    }
  }
  //     CDBDebug("Check [%s]", outputCRS->c_str());
  if (outputCRS->indexOf("PROJ4:") == 0) {
    CT::string temp(outputCRS->c_str() + 6);
    outputCRS->copy(&temp);
  }
  return 0;
}

//   int CImageWarper::_decodeCRS(CT::string *CRS){
//     destinationCRS.copy(CRS);
//     dMaxExtentDefined=0;
//     //destinationCRS.decodeURL();
//     for(size_t j=0;j<(*prj).size();j++){
//       if(destinationCRS.equals((*prj)[j]->attr.id.c_str())){
//         destinationCRS.copy((*prj)[j]->attr.proj4.c_str());
//         if((*prj)[j]->LatLonBox.size()==1){
//           //if(getMaxExtentBBOX!=NULL)
//           {
//             dMaxExtentDefined=1;
//             dfMaxExtent[0]=(*prj)[j]->LatLonBox[0]->attr.minx;
//             dfMaxExtent[1]=(*prj)[j]->LatLonBox[0]->attr.miny;
//             dfMaxExtent[2]=(*prj)[j]->LatLonBox[0]->attr.maxx;
//             dfMaxExtent[3]=(*prj)[j]->LatLonBox[0]->attr.maxy;
//           }
//         }
//         break;
//       }
//     }
//     if(destinationCRS.indexOf("PROJ4:")==0){
//       CT::string temp(destinationCRS.c_str()+6);
//       destinationCRS.copy(&temp);
//     }
//     return 0;
//   }

int CImageWarper::initreproj(CDataSource *dataSource, CGeoParams *GeoDest, std::vector<CServerConfig::XMLE_Projection *> *_prj) {
  if (dataSource == NULL || GeoDest == NULL) {
    CDBError("dataSource==%s||GeoDest==%s", dataSource == NULL ? "NULL" : "not-null", GeoDest == NULL ? "NULL" : "not-null");
    return 1;
  }
  if (dataSource->nativeProj4.empty()) {
    dataSource->nativeProj4.copy(LATLONPROJECTION);
    // CDBWarning("dataSource->CRS.empty() setting to default latlon");
  }
  return initreproj(dataSource->nativeProj4.c_str(), GeoDest, _prj);
}

pthread_mutex_t CImageWarper_initreproj;
int CImageWarper::initreproj(const char *projString, CGeoParams *GeoDest, std::vector<CServerConfig::XMLE_Projection *> *_prj) {
  pthread_mutex_lock(&CImageWarper_initreproj);
  int status = _initreprojSynchronized(projString, GeoDest, _prj);
  pthread_mutex_unlock(&CImageWarper_initreproj);
  return status;
}
int CImageWarper::_initreprojSynchronized(const char *projString, CGeoParams *GeoDest, std::vector<CServerConfig::XMLE_Projection *> *_prj) {

  if (projString == NULL) {
    projString = LATLONPROJECTION;
  }
  prj = _prj;
  if (prj == NULL) {
    CDBError("prj==NULL");
    return 1;
  }

  this->_geoDest = GeoDest;

  CT::string sourceProjectionUndec = projString;

  std::tie(sourceProjectionUndec, std::ignore) = fixProjection(sourceProjectionUndec);
  CT::string sourceProjection = sourceProjectionUndec;
  if (decodeCRS(&sourceProjection, &sourceProjectionUndec, _prj) != 0) {
    CDBError("decodeCRS failed");
    return 1;
  }

  //    CDBDebug("sourceProjectionUndec %s, sourceProjection %s",sourceProjection.c_str(),sourceProjectionUndec.c_str());

  dMaxExtentDefined = 0;
  if (decodeCRS(&destinationCRS, &GeoDest->CRS, _prj) != 0) {
    CDBError("decodeCRS failed");
    return 1;
  }

  projSourceToDest = proj_create_crs_to_crs_with_cache(sourceProjection, destinationCRS, nullptr);
  if (projSourceToDest == nullptr) {
    CDBError("Invalid projection: from %s to %s", sourceProjection.c_str(), destinationCRS.c_str());
    return 1;
  }

  projSourceToLatlon = proj_create_crs_to_crs_with_cache(sourceProjection, CT::string(LATLONPROJECTION), nullptr);
  if (projSourceToLatlon == nullptr) {
    CDBError("Invalid projection: from %s to %s", destinationCRS.c_str(), LATLONPROJECTION);
    return 1;
  }

  projLatlonToDest = proj_create_crs_to_crs_with_cache(CT::string(LATLONPROJECTION), destinationCRS, nullptr);
  if (projLatlonToDest == nullptr) {
    CDBError("Invalid projection: from %s to %s", LATLONPROJECTION, destinationCRS.c_str());
    return 1;
  }

  initialized = true;
  // CDBDebug("sourceProjection = %s destinationCRS = %s",projString,destinationCRS.c_str());

  // Check if we have a projected coordinate system
  //  projUV p,pout;;
  requireReprojection = false;
  double y = 52;
  double x = 5;

  if (proj_trans_generic(projSourceToDest, PJ_INV, &x, sizeof(double), 1, &y, sizeof(double), 1, nullptr, 0, 0, nullptr, 0, 0) != 1) {
    requireReprojection = true;
  }

  if (y + 0.001 < 52 || y - 0.001 > 52 || x + 0.001 < 5 || x - 0.001 > 5) requireReprojection = true;

  return 0;
}

pthread_mutex_t CImageWarper_findExtent;
int CImageWarper::findExtent(CDataSource *dataSource, double *dfBBOX) {
  pthread_mutex_lock(&CImageWarper_findExtent);
  int status = findExtentUnSynchronized(dataSource, dfBBOX);
  pthread_mutex_unlock(&CImageWarper_findExtent);
  return status;
}

int CImageWarper::findExtentUnSynchronized(CDataSource *dataSource, double *dfBBOX) {
  // Find the outermost corners of the image

  bool useLatLonSourceProj = false;
  // Maybe it is defined in the configuration file:
  if (dataSource->cfgLayer->LatLonBox.size() > 0) {
    CServerConfig::XMLE_LatLonBox *box = dataSource->cfgLayer->LatLonBox[0];
    dfBBOX[1] = box->attr.miny;
    dfBBOX[3] = box->attr.maxy;
    dfBBOX[0] = box->attr.minx;
    dfBBOX[2] = box->attr.maxx;
    useLatLonSourceProj = true;

  } else {
    for (int k = 0; k < 4; k++) dfBBOX[k] = dataSource->dfBBOX[k];
    if (dfBBOX[1] > dfBBOX[3]) {
      dfBBOX[1] = dataSource->dfBBOX[3];
      dfBBOX[3] = dataSource->dfBBOX[1];
    }
    if (dfBBOX[0] > dfBBOX[2]) {
      dfBBOX[0] = dataSource->dfBBOX[2];
      dfBBOX[2] = dataSource->dfBBOX[0];
    }
  }

  // CDBDebug("findExtent for %s and %f %f %f %f", destinationCRS.c_str(), dfBBOX[0], dfBBOX[1], dfBBOX[2], dfBBOX[3]);
  ProjectionMapKey key = {sourceCRSString, destinationCRS, makeBBOX(dfBBOX)};
  bool found;
  BBOX bbox{};
  std::tie(found, bbox) = getBBOXProjection(key);

  if (found) {
#ifdef CIMAGEWARPER_DEBUG
    CDBDebug("FOUND AND REUSING!!! %s %s (%0.3f, %0.3f, %0.3f, %0.3f) to  (%0.3f, %0.3f, %0.3f, %0.3f)", key.sourceCRS.c_str(), key.destCRS.c_str(), key.extent.bbox[0], key.extent.bbox[1],
             key.extent.bbox[2], key.extent.bbox[3], bbox.bbox[0], bbox.bbox[1], bbox.bbox[2], bbox.bbox[3]);
#endif
    for (size_t j = 0; j < 4; j++) {
      dfBBOX[j] = bbox.bbox[j];
    }
    return 0;
  }

  // double tempy;
  double miny1 = dfBBOX[1];
  double maxy1 = dfBBOX[3];
  double minx1 = dfBBOX[0];
  // double minx2=dfBBOX[0];
  double maxx1 = dfBBOX[2];
  // double maxx2=dfBBOX[2];
  // CDBDebug("BBOX=(%f,%f,%f,%f)",dfBBOX[0],dfBBOX[1],dfBBOX[2],dfBBOX[3]);

  try {
    double nrTestX = 45;
    double nrTestY = 45;
    bool foundFirst = false;
    for (int y = 0; y < int(nrTestY) + 1; y++) {

      for (int x = 0; x < int(nrTestX) + 1; x++) {
        double stepX = double(x) / nrTestX;
        double stepY = double(y) / nrTestY;

        double testPosX = dfBBOX[0] * (1 - stepX) + dfBBOX[2] * stepX;
        double testPosY = dfBBOX[1] * (1 - stepY) + dfBBOX[3] * stepY;
        bool projError = false;
        double inY = testPosY;
        double inX = testPosX;

        // Make sure testPosX and testPosY are not NaN values
        if (testPosX == testPosX && testPosY == testPosY) {

          try {
            if (useLatLonSourceProj) {
              // Boundingbox is given in the projection definition, it is always given in latlon so we need to project it to the current projection
              if (reprojfromLatLon(inX, inY) != 0) projError = true;
            } else {
              if (reprojpoint_inv(inX, inY) != 0) projError = true;
            }
          } catch (int e) {
            projError = true;
          }

          if (projError == false) {
            double latX = inX, lonY = inY;
            if (reprojToLatLon(latX, lonY) != 0) projError = true;
            ;
            // CDBDebug("LatX,LatY == %f,%f  %3.3d,%3.3d -- %e,%e -- %f,%f %d",stepX,stepY,x,y,testPosX,testPosY,latX,lonY,projError);
            if (projError == false) {
              if (latX > -200 && latX < 400 && lonY > -180 && lonY < 180) {
                if (foundFirst == false) {
                  foundFirst = true;
                  minx1 = inX;
                  maxx1 = inX;
                  miny1 = inY;
                  maxy1 = inY;
                }
                // CDBDebug("testPos (%f;%f)\t proj (%f;%f)",testPosX,testPosY,latX,lonY);
                if (inX < minx1) minx1 = inX;
                if (inY < miny1) miny1 = inY;
                if (inX > maxx1) maxx1 = inX;
                if (inY > maxy1) maxy1 = inY;
              }
            }
          }
        }
      }
    }
  } catch (...) {
    CDBError("Unable to reproject");
    return 1;
  }

  dfBBOX[1] = miny1;
  dfBBOX[3] = maxy1;
  dfBBOX[0] = minx1;
  dfBBOX[2] = maxx1;

  if (dMaxExtentDefined == 0 && 1 == 0) {
    // CDBDebug("dataSource->nativeProj4 %s %d",dataSource->nativeProj4.c_str(), dataSource->nativeProj4.indexOf("geos")>0);
    if (dataSource->nativeProj4.indexOf("geos") != -1) {
      dfMaxExtent[0] = -82 * 2;
      dfMaxExtent[1] = -82;
      dfMaxExtent[2] = 82 * 2;
      dfMaxExtent[3] = 82;
      reprojfromLatLon(dfMaxExtent[0], dfMaxExtent[1]);
      reprojfromLatLon(dfMaxExtent[2], dfMaxExtent[3]);
      dMaxExtentDefined = 1;
    }
  }

  // Check if values are within allowable extent:
  if (dMaxExtentDefined == 1) {
    for (int j = 0; j < 2; j++)
      if (dfBBOX[j] < dfMaxExtent[j]) dfBBOX[j] = dfMaxExtent[j];
    for (int j = 2; j < 4; j++)
      if (dfBBOX[j] > dfMaxExtent[j]) dfBBOX[j] = dfMaxExtent[j];
    if (dfBBOX[0] <= dfBBOX[2])
      for (int j = 0; j < 4; j++) dfBBOX[j] = dfMaxExtent[j];
  }

  if (long(dfBBOX[0] * 100.) == long(dfBBOX[2]) * 100.) {
    dfBBOX[0] -= 1;
    dfBBOX[2] += 1;
  }

  if (long(dfBBOX[1] * 100.) == long(dfBBOX[3] * 100.)) {
    dfBBOX[1] -= 1;
    dfBBOX[3] += 1;
  }
#ifdef CIMAGEWARPER_DEBUG

  CDBDebug("INSERTING!!! %s %s (%0.3f, %0.3f, %0.3f, %0.3f) to  (%0.3f, %0.3f, %0.3f, %0.3f)", key.sourceCRS.c_str(), key.destCRS.c_str(), key.extent.bbox[0], key.extent.bbox[1], key.extent.bbox[2],
           key.extent.bbox[3], dfBBOX[0], dfBBOX[1], dfBBOX[2], dfBBOX[3]);
#endif
  addBBOXProjection(key, makeBBOX(dfBBOX));
  return 0;
};

void CImageWarper::reprojBBOX(double *df4PixelExtent) {
  double b[4], X, Y, xmin, xmax, ymin, ymax;
  for (int j = 0; j < 4; j++) b[j] = df4PixelExtent[j];
  // find XMin:
  xmin = b[0];
  Y = b[1];
  reprojpoint(xmin, Y);
  X = b[0];
  Y = b[3];
  reprojpoint(X, Y);
  if (X < xmin) xmin = X;

  // find YMin:
  for (int x = 0; x < 8; x++) {
    double step = x;
    step = step / 7.0f;
    double ratio = b[0] * (1 - step) + b[2] * step;
    X = ratio;
    Y = b[1];
    reprojpoint(X, Y);
    if (Y < ymin || x == 0) ymin = Y;
    X = ratio;
    Y = b[3];
    reprojpoint(X, Y);
    if (Y > ymax || x == 0) ymax = Y;
  }
  // find XMAx
  xmax = b[2];
  Y = b[1];
  reprojpoint(xmax, Y);
  X = b[2];
  Y = b[3];
  reprojpoint(X, Y);
  if (X > xmax) xmax = X;

  df4PixelExtent[0] = xmin;
  df4PixelExtent[1] = ymin;
  df4PixelExtent[2] = xmax;
  df4PixelExtent[3] = ymax;
}

CT::string CImageWarper::getProj4FromId(CDataSource *dataSource, CT::string projectionId) {
  CT::string bboxProj4Params;
  if (projectionId.equals("native")) {
    bboxProj4Params = dataSource->nativeProj4;
    return bboxProj4Params;
  }
  std::vector<CServerConfig::XMLE_Projection *> *prj = &dataSource->srvParams->cfg->Projection;
  for (size_t j = 0; j < (*prj).size(); j++) {
    if ((*prj)[j]->attr.id.equals(projectionId.trim())) {
      bboxProj4Params = (*prj)[j]->attr.proj4;
      return bboxProj4Params;
      break;
    }
  }
  return projectionId;
}

std::tuple<CT::string, double> CImageWarper::fixProjection(CT::string projectionString) {
  CProj4ToCF trans;
  CDF::Variable var;
  int status = trans.convertProjToCF(&var, projectionString);
  if (status == 0) {
    CDF::Attribute *majorAttribute = var.getAttributeNE("semi_major_axis");
    CDF::Attribute *minorAttribute = var.getAttributeNE("semi_minor_axis");
    if (majorAttribute != nullptr && minorAttribute != nullptr && majorAttribute->getType() == CDF_FLOAT && minorAttribute->getType() == CDF_FLOAT) {
      float semi_major_axis, semi_minor_axis;
      majorAttribute->getData<float>(&semi_major_axis, 1);
      minorAttribute->getData<float>(&semi_minor_axis, 1);
      if (semi_major_axis > 6000.0 && semi_major_axis < 7000.0) { // This is given in km's, and should be converted to meters
        double scaling = 1000.0;
        majorAttribute->setData<float>(CDF_FLOAT, semi_major_axis * scaling);
        minorAttribute->setData<float>(CDF_FLOAT, semi_minor_axis * scaling);

        CT::string newProjectionString;
        int status2 = trans.convertCFToProj(&var, &newProjectionString);
        //        printf("%s\n", projectionString.c_str());
        //        printf("%s\n\n", newProjectionString.c_str());
        if (status2 == 0) return std::make_tuple(newProjectionString, scaling);
      }
    }
  }

  return std::make_tuple(projectionString, 1.0);
}