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

#ifndef CImageWarper_H
#define CImageWarper_H
#define ACCEPT_USE_OF_DEPRECATED_PROJ_API_H 1
#include "CServerParams.h"
#include "CDataReader.h"
#include "CDrawImage.h"
#include <proj_api.h>
#include <math.h>
#include "CDebugger.h"
#include "CStopWatch.h"

#define LATLONPROJECTION "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"
void floatToString(char * string,size_t maxlen,float number);
void floatToString(char * string,size_t maxlen,int numdigits,float number);
void floatToString(char * string,size_t maxlen,float min, float max,float number);
class ProjectionKey{
public:
  double bbox[4];//Original boundingbox to look for
  double foundExtent[4];
  double dfMaxExtent[4];
  bool isSet;
  CT::string destinationCRS;//Projection to convert to
  CT::string sourceCRS;//Projection to convert from
  ProjectionKey(double *_box,double *_dfMaxExtent,CT::string source,CT::string dest);
  ProjectionKey();
  void setFoundExtent(double *_foundExtent);
};

class ProjectionStore{
public:
  std::vector <ProjectionKey> keys;
  ProjectionStore();
  ~ProjectionStore();
  static ProjectionStore *getProjectionStore();
  void clear();
};



class CImageWarper{
//  CNetCDFReader reader;
  private:

    
    double dfMaxExtent[4];
    int dMaxExtentDefined;
    
    DEF_ERRORFUNCTION();
    unsigned char pixel;
    CDataSource * _dataSource;
    CGeoParams * _geoDest;
    CT::string sourceCRSString;
    CT::string destinationCRS;
//     int _decodeCRS(CT::string *CRS);
    std::vector <CServerConfig::XMLE_Projection*> *prj;
    bool initialized;
    int _findExtentSynchronized(CDataSource *dataSource,double * dfBBOX);
    int _initreprojSynchronized(const char * projString,CGeoParams *GeoDest,std::vector <CServerConfig::XMLE_Projection*> *_prj);
  public:
    bool destNeedsDegreeRadianConversion,sourceNeedsDegreeRadianConversion,requireReprojection;
    CImageWarper(){
      prj=NULL;
      sourcepj=NULL;
      destpj=NULL;
      latlonpj=NULL;
      initialized =false;
      proj4Context = NULL;
    }
    ~CImageWarper(){
      if(initialized==true){
        closereproj();
        prj=NULL;
        sourcepj=NULL;
        destpj=NULL;
        latlonpj=NULL;
        initialized =false;
        proj4Context = NULL;
      }
    }
    projPJ sourcepj,destpj,latlonpj;
    projCtx proj4Context;
    CT::string getDestProjString(){
      return destinationCRS;
    }
    int initreproj(CDataSource *dataSource,CGeoParams *GeoDest,std::vector <CServerConfig::XMLE_Projection*> *prj);
    int initreproj(const char * projString,CGeoParams *GeoDest,std::vector <CServerConfig::XMLE_Projection*> *_prj);
    
    int closereproj();
    int reprojpoint(double &dfx,double &dfy);
    int reprojpoint(CPoint &p);
    int reprojpoint_inv(double &dfx,double &dfy);
    
    /**
     * Reprojects a point to screen pixel coordinates
     * Same as reprojpoint_inv, but reprojects a point to screen pixel coordinates (WMS Map coordinates). 
     * It uses the BBOX and WIDTH/HEIGHT for this.
     */
    int reprojpoint_inv_topx(double &dfx,double &dfy);
    int reprojpoint_inv(CPoint &p);
    int reprojModelToLatLon(double &dfx,double &dfy);
    int reprojModelFromLatLon(double &dfx,double &dfy);
    void reprojBBOX(double *df4PixelExtent);
    int reprojfromLatLon(double &dfx,double &dfy);
    int reprojToLatLon(double &dfx,double &dfy);
    //int decodeCRS(CT::string *outputCRS, CT::string *inputCRS);
    int decodeCRS(CT::string *outputCRS, CT::string *inputCRS,std::vector <CServerConfig::XMLE_Projection*> *prj);
    int findExtent(CDataSource *dataSource,double * dfBBOX);
    bool isProjectionRequired(){return requireReprojection;}
    
  };
  

#endif
