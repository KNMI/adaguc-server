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

#include "CImageWarper.h"
#include <iostream>
#include <vector>
const char * CImageWarper::className = "CImageWarper";

extern ProjectionStore projectionStore;
ProjectionStore projectionStore;
ProjectionStore *ProjectionStore::getProjectionStore(){
  return &projectionStore;
}

// extern CImageWarper imageWarper;
// CImageWarper imageWarper;
// CImageWarper *CImageWarper::getCImageWarper(){
//   return &imageWarper;
// }



ProjectionKey::ProjectionKey(){
}
pthread_mutex_t ProjectionKey_ProjectionKey;
ProjectionKey::ProjectionKey(double *_box,double *_dfMaxExtent,CT::string source,CT::string dest){
  pthread_mutex_lock(&ProjectionKey_ProjectionKey);
  for(int j=0;j<4;j++){
    bbox[j]=_box[j];
    dfMaxExtent[j]=_dfMaxExtent[j];
  }
  sourceCRS = source;
  destinationCRS = dest;
  isSet = false;
  pthread_mutex_unlock(&ProjectionKey_ProjectionKey);
}

pthread_mutex_t ProjectionKey_setFoundExtent;
void ProjectionKey::setFoundExtent(double *_foundExtent){
  pthread_mutex_lock(&ProjectionKey_setFoundExtent);
  for(int j=0;j<4;j++){
    foundExtent[j]=_foundExtent[j];
  }
  isSet = true;
  pthread_mutex_unlock(&ProjectionKey_setFoundExtent);
}

ProjectionStore::ProjectionStore(){}
ProjectionStore::~ProjectionStore(){clear();}



pthread_mutex_t ProjectionKey_clear;
void ProjectionStore::clear(){
  pthread_mutex_lock(&ProjectionKey_clear);
  keys.clear();
  pthread_mutex_unlock(&ProjectionKey_clear);
}


void floatToString(char * string,size_t maxlen,int numdigits,float number){
  //snprintf(string,maxlen,"%0.2f",number);
  //return;
  if(numdigits>-3&&numdigits<4){
    if(numdigits <= -3)snprintf(string,maxlen,"%0.5f",float(floor(number*100000.0+0.5)/100000));
    if(numdigits == -2)snprintf(string,maxlen,"%0.4f",float(floor(number*10000.0+0.5)/10000));
    if(numdigits == -1)snprintf(string,maxlen,"%0.3f",float(floor(number*1000.0+0.5)/1000));
    if(numdigits == 0)snprintf(string,maxlen,"%0.3f",float(floor(number*1000.0+0.5)/1000));
    if(numdigits == 1)snprintf(string,maxlen,"%0.2f",float(floor(number*100.0+0.5)/100));
    if(numdigits == 2)snprintf(string,maxlen,"%0.1f",float(floor(number*10.0+0.5)/10));
    if(numdigits >= 3)snprintf(string,maxlen,"%0.1f",float(floor(number+0.5)));
  }
  else
    snprintf(string,maxlen,"%0.3e",number);
}

void floatToString(char * string,size_t maxlen,float number){
  int numdigits = 0;
  
  if(number==0.0f)numdigits=0;else{
    float tempp=number;
    if (tempp<0.00000001&&tempp>-0.00000001)tempp+=0.00000001;
    numdigits = int(log10(fabs(tempp)))+1;
    
  }
  floatToString(string,maxlen,numdigits,number);
}

void floatToString(char * string,size_t maxlen,float min, float max,float number){
  float range = fabs(max-min);
  int digits=(int)log10(range)+1;
  floatToString(string,maxlen,digits,number);
}

int CImageWarper::closereproj(){
  if(initialized){
    if(sourcepj!=NULL){pj_free(sourcepj);sourcepj=NULL;}
    if(destpj!=NULL){pj_free(destpj);destpj=NULL;}
    if(latlonpj!=NULL){pj_free(latlonpj);latlonpj=NULL;}
    if(proj4Context!=NULL){pj_ctx_free(proj4Context);proj4Context=NULL;}
  }
  initialized=false;
  return 0;
}

int CImageWarper::reprojpoint(double &dfx,double &dfy){
  if(destNeedsDegreeRadianConversion){
    dfx*=DEG_TO_RAD;
    dfy*=DEG_TO_RAD;
  }
  if(pj_transform(destpj,sourcepj, 1,0,&dfx,&dfy,NULL)!=0){
    //throw("reprojpoint error");
    return 1;
   // CDBError("ReprojException");
  }
  if(dfx!=dfx||dfy!=dfy){
       dfx=0;dfy=0;
      return 1;
    }
    if(dfx== HUGE_VAL||dfy== HUGE_VAL){
       dfx=0;dfy=0;
      return 1;
    }
  if(sourceNeedsDegreeRadianConversion){
    dfx/=DEG_TO_RAD;
    dfy/=DEG_TO_RAD;
  }
  return 0;
}
int CImageWarper::reprojpoint(CPoint &p){
  return reprojpoint(p.x,p.y);
}
int CImageWarper::reprojpoint_inv(CPoint &p){
  return reprojpoint_inv(p.x,p.y);
}


  int CImageWarper::reprojToLatLon(double &dfx,double &dfy){
    if(destNeedsDegreeRadianConversion){
      dfx*=DEG_TO_RAD;
      dfy*=DEG_TO_RAD;
    }
    if(pj_transform(destpj,latlonpj,1,0,&dfx,&dfy,NULL)!=0){
      //throw("reprojfromLatLon error");
      dfx=0;dfy=0;
      return 1;
    }
    dfx/=DEG_TO_RAD;
    dfy/=DEG_TO_RAD;    
    return 0;
  }

  int CImageWarper::reprojfromLatLon(double &dfx,double &dfy){
    if(dfx<-180||dfx>180||dfy<-90||dfy>90){ dfx=0;dfy=0;return 1;}
    dfx*=DEG_TO_RAD;
    dfy*=DEG_TO_RAD;
    
    if(pj_transform(latlonpj,destpj,1,0,&dfx,&dfy,NULL)!=0){
      //CDBError("Projection error");
      dfx=0;dfy=0;
      return 1;
    }
    if(dfx!=dfx||dfy!=dfy){
       dfx=0;dfy=0;
      return 1;
    }
    if(dfx== HUGE_VAL||dfy== HUGE_VAL){
       dfx=0;dfy=0;
      return 1;
    }
    //if(status!=0)CDBDebug("DestPJ: %s",GeoDest->CRS.c_str());
    if(destNeedsDegreeRadianConversion){
      dfx/=DEG_TO_RAD;
      dfy/=DEG_TO_RAD;
    }
    return 0;
  }

 
  int CImageWarper::reprojModelToLatLon(double &dfx,double &dfy){
    if(sourceNeedsDegreeRadianConversion){
      dfx*=DEG_TO_RAD;
      dfy*=DEG_TO_RAD;
    }
    if(pj_transform(sourcepj,latlonpj,1,0,&dfx,&dfy,NULL)!=0){
     
     return 1;
    }
    dfx/=DEG_TO_RAD;
    dfy/=DEG_TO_RAD;    
    return 0;
  }
  
  int CImageWarper::reprojModelFromLatLon(double &dfx,double &dfy){
    dfx*=DEG_TO_RAD;
    dfy*=DEG_TO_RAD;
    
    if(pj_transform(latlonpj,sourcepj,1,0,&dfx,&dfy,NULL)!=0){
     return 1;
    }
    //if(status!=0)CDBDebug("DestPJ: %s",GeoDest->CRS.c_str());
    if(sourceNeedsDegreeRadianConversion){
      dfx/=DEG_TO_RAD;
      dfy/=DEG_TO_RAD;
    }
    return 0;
  }
    
  
  int CImageWarper::reprojpoint_inv_topx(double &dfx,double &dfy){
    if (reprojpoint_inv(dfx,dfy) != 0) return 1;
    dfx = (dfx - _geoDest->dfBBOX[0] ) / (_geoDest->dfBBOX[2]- _geoDest->dfBBOX[0]) * double(_geoDest->dWidth);
    dfy = (dfy - _geoDest->dfBBOX[3] ) / (_geoDest->dfBBOX[1]- _geoDest->dfBBOX[3]) * double(_geoDest->dHeight);
    return 0;
  }
  
  int CImageWarper::reprojpoint_inv(double &dfx,double &dfy){
    
    if(sourceNeedsDegreeRadianConversion){
      dfx*=DEG_TO_RAD;
      dfy*=DEG_TO_RAD;
    }
    if(pj_transform(sourcepj,destpj,1,0,&dfx,&dfy,NULL)!=0){
     //CDBError("ReprojException: %f %f",dfx,dfy);
     dfx=0;
     dfy=0;
     return 1;
    }
    if(destNeedsDegreeRadianConversion){
      dfx/=DEG_TO_RAD;
      dfy/=DEG_TO_RAD;
    }
    return 0;
  }
//   int CImageWarper::decodeCRS(CT::string *outputCRS, CT::string *inputCRS){
//     return decodeCRS(outputCRS,inputCRS,prj);
//   }
  int CImageWarper::decodeCRS(CT::string *outputCRS, CT::string *inputCRS,std::vector <CServerConfig::XMLE_Projection*> *prj){
    if(prj==NULL){
      CDBError("decodeCRS: prj==NULL");
      return 1;
    }
    if(&(*prj)==NULL){
      CDBError("decodeCRS: prj==NULL");
      return 1;
    }
    outputCRS->copy(inputCRS);
    //if(inputCRS->indexOf("+proj")!=-1)return 0;
    dMaxExtentDefined=0;
    //CDBDebug("Check");
    // outputCRS->decodeURLSelf();
    //CDBDebug("Check");
    //CDBDebug("Check %d",(*prj).size());
    for(size_t j=0;j<(*prj).size();j++){
      //CDBDebug("Check");
      if(outputCRS->equals((*prj)[j]->attr.id.c_str())){
        outputCRS->copy((*prj)[j]->attr.proj4.c_str());
        if((*prj)[j]->LatLonBox.size()==1){
          //if(getMaxExtentBBOX!=NULL)
          {
            dMaxExtentDefined=1;
            dfMaxExtent[0]=(*prj)[j]->LatLonBox[0]->attr.minx;
            dfMaxExtent[1]=(*prj)[j]->LatLonBox[0]->attr.miny;
            dfMaxExtent[2]=(*prj)[j]->LatLonBox[0]->attr.maxx;
            dfMaxExtent[3]=(*prj)[j]->LatLonBox[0]->attr.maxy;
          }
        }
        break;
      }
    }
//     CDBDebug("Check [%s]", outputCRS->c_str());
    if(outputCRS->indexOf("PROJ4:")==0){
      CT::string temp(outputCRS->c_str()+6);
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

  int CImageWarper::initreproj(CDataSource *dataSource,CGeoParams *GeoDest,std::vector <CServerConfig::XMLE_Projection*> *_prj){
    if(dataSource==NULL||GeoDest==NULL){
      CDBError("dataSource==%s||GeoDest==%s", dataSource==NULL?"NULL":"not-null", GeoDest==NULL?"NULL":"not-null");
      return 1;
    }
    if(dataSource->nativeProj4.empty()){
      dataSource->nativeProj4.copy(LATLONPROJECTION);
      //CDBWarning("dataSource->CRS.empty() setting to default latlon");
    }
    return initreproj(dataSource->nativeProj4.c_str(),GeoDest,_prj);
  }
  
  pthread_mutex_t CImageWarper_initreproj;
  int CImageWarper::initreproj(const char * projString,CGeoParams *GeoDest,std::vector <CServerConfig::XMLE_Projection*> *_prj){
    pthread_mutex_lock(&CImageWarper_initreproj);
    int status = _initreprojSynchronized(projString,GeoDest, _prj);
    pthread_mutex_unlock(&CImageWarper_initreproj);
    return status;
  }
  int CImageWarper::_initreprojSynchronized(const char * projString,CGeoParams *GeoDest,std::vector <CServerConfig::XMLE_Projection*> *_prj){
    
    if(projString==NULL){
      projString = LATLONPROJECTION;
    }
    prj=_prj;
    if(prj==NULL){
      CDBError("prj==NULL");
      return 1;
    }
    
    this->_geoDest = GeoDest;
   
    if(proj4Context!=NULL){
      pj_ctx_free(proj4Context);
    }
    proj4Context = pj_ctx_alloc();
    
    CT::string sourceProjectionUndec = projString;
    CT::string sourceProjection = projString;
    if(decodeCRS(&sourceProjection,&sourceProjectionUndec,_prj)!=0){
      CDBError("decodeCRS failed");
      return 1;
    }
    
//    CDBDebug("sourceProjectionUndec %s, sourceProjection %s",sourceProjection.c_str(),sourceProjectionUndec.c_str());
   
    if (!(sourcepj = pj_init_plus_ctx(proj4Context,sourceProjection.c_str()))){
      CDBError("SetSourceProjection: Invalid projection: %s",sourceProjection.c_str());
      return 1;
    }
    if(sourcepj==NULL){
      CDBError("SetSourceProjection: Invalid projection: %s",sourceProjection.c_str());
      return 1;
    }
    if (!(latlonpj = pj_init_plus_ctx(proj4Context,LATLONPROJECTION))){
      CDBError("SetLatLonProjection: Invalid projection: %s",LATLONPROJECTION);
      return 1;
    }
    dMaxExtentDefined=0;
    if(decodeCRS(&destinationCRS,&GeoDest->CRS,_prj)!=0){
      CDBError("decodeCRS failed");
      return 1;
    }
    
    if (!(destpj = pj_init_plus_ctx(proj4Context,destinationCRS.c_str()))){
      CDBError("SetDestProjection: Invalid projection: %s",destinationCRS.c_str());
      return 1;
    }
    initialized = true;
    //CDBDebug("sourceProjection = %s destinationCRS = %s",projString,destinationCRS.c_str());
    
    // Check if we have a projected coordinate system
    //  projUV p,pout;;
    requireReprojection=false;
    double y=52; double x=5;
    x *= DEG_TO_RAD;y *= DEG_TO_RAD;
    if(pj_transform(destpj,sourcepj, 1,0,&x,&y,NULL)!=0)requireReprojection=true;
    x /= DEG_TO_RAD;y /= DEG_TO_RAD;
    if(y+0.001<52||y-0.001>52||
       x+0.001<5||x-0.001>5)requireReprojection=true;
    //Check wether we should convert between radians and degrees for the dest and source projections
    
    if(destinationCRS.indexOf("longlat")>=0){
      destNeedsDegreeRadianConversion = true;
    }else destNeedsDegreeRadianConversion =false;
    
    sourceCRSString = sourceProjection.c_str();
    if(sourceCRSString.indexOf("longlat")>=0){
      sourceNeedsDegreeRadianConversion = true;
    }else sourceNeedsDegreeRadianConversion=false;
    
    return 0;

  }
  
    
  
  pthread_mutex_t CImageWarper_findExtent;
  int CImageWarper::findExtent(CDataSource *dataSource,double * dfBBOX){
    pthread_mutex_lock(&CImageWarper_findExtent);
    int status = _findExtentSynchronized(dataSource,dfBBOX);
    pthread_mutex_unlock(&CImageWarper_findExtent);
    return status;
  }
    
  int CImageWarper::_findExtentSynchronized(CDataSource *dataSource,double * dfBBOX){
  // Find the outermost corners of the image
  
  
  //CDBDebug("findExtent for %s",destinationCRS.c_str());
    bool useLatLonSourceProj =false;
    //Maybe it is defined in the configuration file:
    if(dataSource->cfgLayer->LatLonBox.size()>0){
      CServerConfig::XMLE_LatLonBox* box = dataSource->cfgLayer->LatLonBox[0];
      dfBBOX[1]=box->attr.miny;
      dfBBOX[3]=box->attr.maxy;
      dfBBOX[0]=box->attr.minx;
      dfBBOX[2]=box->attr.maxx;
      useLatLonSourceProj=true;
      
    }else{
      for(int k=0;k<4;k++)dfBBOX[k]=dataSource->dfBBOX[k];
      if(dfBBOX[1]>dfBBOX[3]){
        dfBBOX[1]=dataSource->dfBBOX[3];
        dfBBOX[3]=dataSource->dfBBOX[1];
      }
      if(dfBBOX[0]>dfBBOX[2]){
        dfBBOX[0]=dataSource->dfBBOX[2];
        dfBBOX[2]=dataSource->dfBBOX[0];
      }
    }
    

    
    ProjectionKey pKey(dfBBOX,dfMaxExtent,sourceCRSString,destinationCRS);

    for(size_t j=0;j<projectionStore.keys.size();j++){
      if(projectionStore.keys[j].isSet == true){
        bool match = true;
        if(!projectionStore.keys[j].destinationCRS.equals(pKey.destinationCRS.c_str()))match=false;
        //if(!match){CDBDebug("DESTCRS DO NOT MATCH %s",pKey.destinationCRS.c_str());}
        if(match){
          for(int i=0;i<4;i++){
            if(projectionStore.keys[j].bbox[i] != pKey.bbox[i]){
              //CDBDebug("%f != %f",projectionStore.keys[j].bbox[i],pKey.bbox[i]);
              match = false;break;
            }
          }
          //if(!match){CDBDebug("BBOX DO NOT MATCH");}
          if(match){
            if(!projectionStore.keys[j].sourceCRS.equals(pKey.sourceCRS.c_str()))match=false;
            //if(!match){CDBDebug("SOURCECRS DO NOT MATCH");}
            if(match){
              for(int i=0;i<4;i++){
                dfBBOX[i] = projectionStore.keys[j].foundExtent[i];
              }
              //CDBDebug("FOUND PROJECTION KEY");
              return 0;
            }
          }
        }
      }
    }
   
    
    
   
    
    //double tempy;
    double miny1=dfBBOX[1];
    double maxy1=dfBBOX[3];
    double minx1=dfBBOX[0];
    //double minx2=dfBBOX[0];
    double maxx1=dfBBOX[2];
    //double maxx2=dfBBOX[2];
    //CDBDebug("BBOX=(%f,%f,%f,%f)",dfBBOX[0],dfBBOX[1],dfBBOX[2],dfBBOX[3]);
    try{
      double nrTestX=45;
      double nrTestY=45;
      bool foundFirst = false;
      for(int y=0;y<int(nrTestY)+1;y++){
          
        for(int x=0;x<int(nrTestX)+1;x++){
          double stepX = double(x)/nrTestX;
          double stepY = double(y)/nrTestY;
         
          
          double testPosX = dfBBOX[0]*(1-stepX)+dfBBOX[2]*stepX;
          double testPosY = dfBBOX[1]*(1-stepY)+dfBBOX[3]*stepY;
          double inY = testPosY;
          double inX = testPosX;
          bool projError=false;
          
          if(testPosX==testPosX && testPosY == testPosY){
            try{
              if(useLatLonSourceProj){
                //Boundingbox is given in the projection definition, it is always given in latlon so we need to project it to the current projection
                if(reprojfromLatLon(inX,inY)!=0)projError=true;
              }else {
                if(reprojpoint_inv(inX,inY)!=0)projError=true;
              }
            }catch(int e){
              projError=true;
            }
          }else{
              projError=true;
          }
          
          if(projError==false){
            double latX=inX,lonY=inY;
            if(reprojToLatLon(latX,lonY)!=0)projError=true;;
            //CDBDebug("LatX,LatY == %f,%f  %3.3d,%3.3d -- %e,%e -- %f,%f %d",stepX,stepY,x,y,testPosX,testPosY,latX,lonY,projError);
            if(projError==false){
              if(latX>-200&&latX<400&&lonY>-180&&lonY<180){
                if(foundFirst == false){
                  foundFirst = true;
                  minx1=inX;
                  maxx1=inX;
                  miny1=inY;
                  maxy1=inY;
                }
                //CDBDebug("testPos (%f;%f)\t proj (%f;%f)",testPosX,testPosY,latX,lonY);
                if(inX<minx1)minx1=inX;
                if(inY<miny1)miny1=inY;
                if(inX>maxx1)maxx1=inX;
                if(inY>maxy1)maxy1=inY;
              }
            }
          }
        }
          
      }
    }catch(...){
      CDBError("Unable to reproject");
      return 1;
    }
   
    dfBBOX[1]=miny1;
    dfBBOX[3]=maxy1;
    dfBBOX[0]=minx1;
    dfBBOX[2]=maxx1;
    
    if(dMaxExtentDefined==0&&1==0){
      //CDBDebug("dataSource->nativeProj4 %s %d",dataSource->nativeProj4.c_str(), dataSource->nativeProj4.indexOf("geos")>0);
      if( dataSource->nativeProj4.indexOf("geos")!=-1){
        dfMaxExtent[0]=-82*2;
        dfMaxExtent[1]=-82;
        dfMaxExtent[2]=82*2;
        dfMaxExtent[3]=82;
        reprojfromLatLon(dfMaxExtent[0],dfMaxExtent[1]);
        reprojfromLatLon(dfMaxExtent[2],dfMaxExtent[3]);
        dMaxExtentDefined=1;
      }
    }
   
    //Check if values are within allowable extent:
    if(dMaxExtentDefined==1){
      for(int j=0;j<2;j++)if(dfBBOX[j]<dfMaxExtent[j])dfBBOX[j]=dfMaxExtent[j];
      for(int j=2;j<4;j++)if(dfBBOX[j]>dfMaxExtent[j])dfBBOX[j]=dfMaxExtent[j];
      if(dfBBOX[0]<=dfBBOX[2])
        for(int j=0;j<4;j++)dfBBOX[j]=dfMaxExtent[j];

    }
    
    if (long(dfBBOX[0]*100.) == long(dfBBOX[2])*100.){
      dfBBOX[0]-=1;
      dfBBOX[2]+=1;
    }
    
    if (long(dfBBOX[1]*100.) == long(dfBBOX[3]*100.)){
      dfBBOX[1]-=1;
      dfBBOX[3]+=1;
    }
   
    pKey.setFoundExtent(dfBBOX);
    projectionStore.keys.push_back(pKey);
   
    //CDBDebug("out: %f %f %f %f",dfBBOX[0],dfBBOX[1],dfBBOX[2],dfBBOX[3]);
    return 0;
  };
  
  
  
  
  void CImageWarper::reprojBBOX(double *df4PixelExtent){
    double b[4],X,Y,xmin,xmax,ymin,ymax;
    for(int j=0;j<4;j++)b[j]=df4PixelExtent[j];
    //find XMin:
    xmin=b[0];Y=b[1];reprojpoint(xmin,Y);
       X=b[0];Y=b[3];reprojpoint(X,Y);
    if(X<xmin)xmin=X;

    //find YMin:
    for(int x=0;x<8;x++){
      double step = x;step=step/7.0f;
      double ratio = b[0]*(1-step)+b[2]*step;
      X=ratio;Y=b[1];reprojpoint(X,Y);
      if(Y<ymin||x==0)ymin=Y;
      X=ratio;Y=b[3];reprojpoint(X,Y);
      if(Y>ymax||x==0)ymax=Y;
    }
    //find XMAx
    xmax=b[2];Y=b[1];reprojpoint(xmax,Y);
       X=b[2];Y=b[3];reprojpoint(X,Y);
    if(X>xmax)xmax=X;
    
    df4PixelExtent[0]=xmin;
    df4PixelExtent[1]=ymin;
    df4PixelExtent[2]=xmax;
    df4PixelExtent[3]=ymax;
  }

