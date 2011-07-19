#include "CImageWarper.h"
#include <iostream>
#include <vector>
const char * CImageWarper::className = "CImageWarper";

void floatToString(char * string,size_t maxlen,float number){
  int numdigits = 0;

  if(number==0.0f)numdigits=0;else{
    float tempp=number;
    if (tempp<0.0001&&tempp>-0.0001)tempp+=0.000001;
    numdigits = int(log10(fabs(tempp)));
  }
  if(numdigits>-3&&numdigits<4){
    if(numdigits <= -3)snprintf(string,maxlen,"%0.5f",number);
    if(numdigits == -2)snprintf(string,maxlen,"%0.4f",number);
    if(numdigits == -1)snprintf(string,maxlen,"%0.3f",number);
    if(numdigits == 0)snprintf(string,maxlen,"%0.3f",number);
    if(numdigits == 1)snprintf(string,maxlen,"%0.2f",number);
    if(numdigits == 2)snprintf(string,maxlen,"%0.1f",number);
    if(numdigits >= 3)snprintf(string,maxlen,"%0.0f",number);
  }
  else
    snprintf(string,maxlen,"%0.3e",number);
}

int CImageWarper::closereproj(){
  pj_free(sourcepj);
  pj_free(destpj);
  pj_free(latlonpj);
  return 0;
}

  int CImageWarper::reprojpoint(double &dfx,double &dfy){
    if(convertRadiansDegreesDst){
      dfx*=DEG_TO_RAD;
      dfy*=DEG_TO_RAD;
    }
    if(pj_transform(destpj,sourcepj, 1,0,&dfx,&dfy,NULL)!=0){
      //throw("reprojpoint error");
    }
    if(convertRadiansDegreesSrc){
      dfx/=DEG_TO_RAD;
      dfy/=DEG_TO_RAD;
    }
    return 0;
  }
  
  int CImageWarper::reprojToLatLon(double &dfx,double &dfy){
    if(convertRadiansDegreesDst){
      dfx*=DEG_TO_RAD;
      dfy*=DEG_TO_RAD;
    }
    if(pj_transform(destpj,latlonpj,1,0,&dfx,&dfy,NULL)!=0){
      //throw("reprojfromLatLon error");
    }
    dfx/=DEG_TO_RAD;
    dfy/=DEG_TO_RAD;    
    return 0;
  }
  
  
  int CImageWarper::reprojfromLatLon(double &dfx,double &dfy){
    dfx*=DEG_TO_RAD;
    dfy*=DEG_TO_RAD;
    
    if(pj_transform(latlonpj,destpj,1,0,&dfx,&dfy,NULL)!=0){
      //throw("reprojfromLatLon error");
    }
    //if(status!=0)CDBDebug("DestPJ: %s",GeoDest->CRS.c_str());
    if(convertRadiansDegreesDst){
      dfx/=DEG_TO_RAD;
      dfy/=DEG_TO_RAD;
    }
    return 0;
  }
  
  int CImageWarper::reprojpoint_inv(double &dfx,double &dfy){
    
    if(convertRadiansDegreesSrc){
      dfx*=DEG_TO_RAD;
      dfy*=DEG_TO_RAD;
    }
    if(pj_transform(sourcepj,destpj,1,0,&dfx,&dfy,NULL)!=0){
      //throw "reprojpoint_inv error";
    }
    if(convertRadiansDegreesDst){
      dfx/=DEG_TO_RAD;
      dfy/=DEG_TO_RAD;
    }
    return 0;
  }
  int CImageWarper::decodeCRS(CT::string *outputCRS, CT::string *inputCRS){
    return decodeCRS(outputCRS,inputCRS,prj);
  }
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
    dMaxExtentDefined=0;
    //CDBDebug("Check");
    outputCRS->decodeURL();
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
    //CDBDebug("Check");
    if(outputCRS->indexOf("PROJ4:")==0){
      CT::string temp(outputCRS->c_str()+6);
      outputCRS->copy(&temp);
    }
    return 0;
  }

  int CImageWarper::_decodeCRS(CT::string *CRS){
    destinationCRS.copy(CRS);
    dMaxExtentDefined=0;
    //destinationCRS.decodeURL();
    for(size_t j=0;j<(*prj).size();j++){
      if(destinationCRS.equals((*prj)[j]->attr.id.c_str())){
        destinationCRS.copy((*prj)[j]->attr.proj4.c_str());
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
    if(destinationCRS.indexOf("PROJ4:")==0){
      CT::string temp(destinationCRS.c_str()+6);
      destinationCRS.copy(&temp);
    }
    return 0;
  }

  int CImageWarper::initreproj(CDataSource *sourceImage,CGeoParams *GeoDest,std::vector <CServerConfig::XMLE_Projection*> *_prj){
    prj=_prj;
    if(prj==NULL){
      CDBError("prj==NULL");
      return 1;
    }
    if(sourceImage==NULL||GeoDest==NULL){
      CDBError("sourceImage==NULL||GeoDest==NULL");
      return 1;
    }
    if(sourceImage->nativeProj4.c_str()==NULL){
      sourceImage->nativeProj4.copy("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
      CDBWarning("sourceImage->CRS.c_str()==NULL setting to default latlon");
      //return 1;
    }
    if (!(sourcepj = pj_init_plus(sourceImage->nativeProj4.c_str()))){
      CDBError("SetSourceProjection: Invalid projection: %s",sourceImage->nativeProj4.c_str());
      return 1;
    }
    if(sourcepj==NULL){
      CDBError("SetSourceProjection: Invalid projection: %s",sourceImage->nativeProj4.c_str());
      return 1;
    }
    if (!(latlonpj = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"))){
      CDBError("SetLatLonProjection: Invalid projection: %s","+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
      return 1;
    }
    dMaxExtentDefined=0;
    if(_decodeCRS(&GeoDest->CRS)!=0){
      CDBError("decodeCRS failed");
      return 1;
    }
    if (!(destpj = pj_init_plus(destinationCRS.c_str()))){
      CDBError("SetDestProjection: Invalid projection: %s",destinationCRS.c_str());
      return 1;
    }
    // Check if we have a projected coordinate system
    //  projUV p,pout;;
    requireReprojection=false;
    double y=52; double x=5;
    x *= DEG_TO_RAD;y *= DEG_TO_RAD;
    pj_transform(destpj,sourcepj, 1,0,&x,&y,NULL);
    x /= DEG_TO_RAD;y /= DEG_TO_RAD;
    if(y+0.001<52||y-0.001>52||
       x+0.001<5||x-0.001>5)requireReprojection=true;
    //Check wether we should convert between radians and degrees for the dest and source projections
    
    if(destinationCRS.indexOf("longlat")>=0){
      convertRadiansDegreesDst = true;
    }else convertRadiansDegreesDst =false;
    
    if(sourceImage->nativeProj4.indexOf("longlat")>=0){
      convertRadiansDegreesSrc = true;
    }else convertRadiansDegreesSrc=false;
    
    return 0;

  }
  
  int CImageWarper::findExtent(CDataSource *sourceImage,double * dfBBOX){
  // Find the outermost corners of the image
    
    bool useLatLonSourceProj =false;
    //Maybe it is defined in the configuration file:
    if(sourceImage->cfgLayer->LatLonBox.size()>0){
      CServerConfig::XMLE_LatLonBox* box = sourceImage->cfgLayer->LatLonBox[0];
      dfBBOX[1]=box->attr.miny;
      dfBBOX[3]=box->attr.maxy;
      dfBBOX[0]=box->attr.minx;
      dfBBOX[2]=box->attr.maxx;
      useLatLonSourceProj=true;
      
    }else{
      for(int k=0;k<4;k++)dfBBOX[k]=sourceImage->dfBBOX[k];
      if(dfBBOX[1]>dfBBOX[3]){
        dfBBOX[1]=sourceImage->dfBBOX[3];
        dfBBOX[3]=sourceImage->dfBBOX[1];
      }
      if(dfBBOX[0]>dfBBOX[2]){
        dfBBOX[0]=sourceImage->dfBBOX[2];
        dfBBOX[2]=sourceImage->dfBBOX[0];
      }
    }
    double tempy;//tempx
    double miny1=dfBBOX[1];
   // double miny2=dfBBOX[1];
    double maxy1=dfBBOX[3];
    //double maxy2=dfBBOX[3];
    double minx1=dfBBOX[0];
    double minx2=dfBBOX[0];
    double maxx1=dfBBOX[2];
    double maxx2=dfBBOX[2];
//    CDBDebug("source: %f %f %f %f",dfBBOX[0],dfBBOX[1],dfBBOX[2],dfBBOX[3]);
    /*tempx=dfBBOX[0];
    reprojpoint_inv(tempx,miny1);
    tempx=dfBBOX[2];
    reprojpoint_inv(tempx,miny2);
    if(miny1>miny2)miny1=miny2;*/

    try{
      for(int x=0;x<3;x++){
        double step = x;step=step/2.0f;
        double out=dfBBOX[3];
        double ratio = dfBBOX[0]*(1-step)+dfBBOX[2]*step;
        double in = ratio;
        if(useLatLonSourceProj)reprojfromLatLon(in,out);else reprojpoint_inv(in,out);
      // CDBDebug("1: %f == %f  == %f ",ratio,out,HUGE_VAL);
        if(maxy1<out||x==0)maxy1=out;
        in=ratio;
        out=dfBBOX[1];
        if(useLatLonSourceProj)reprojfromLatLon(in,out);else reprojpoint_inv(in,out);
      // CDBDebug("3: %f == %f ",ratio,out);
        if(miny1>out||x==0)miny1=out;
        
        //tempx=dfBBOX[2];
        //reprojpoint_inv(tempx,maxy2);
        //if(maxy1<maxy2)maxy1=maxy2;
      }
    }catch(...){
      CDBError("Unable to reproject");
      return 1;
    }
    //CDBDebug("miny3:%f \tmaxy:%f ",miny1,maxy1);
    tempy=dfBBOX[1];
    if(useLatLonSourceProj)reprojfromLatLon(minx1,tempy);else reprojpoint_inv(minx1,tempy);
    tempy=dfBBOX[3];
    if(useLatLonSourceProj)reprojfromLatLon(minx2,tempy);else reprojpoint_inv(minx2,tempy);
    if(minx1>minx2)minx1=minx2;

    tempy=dfBBOX[1];
    if(useLatLonSourceProj)reprojfromLatLon(maxx1,tempy);else reprojpoint_inv(maxx1,tempy);

    tempy=dfBBOX[3];
    if(useLatLonSourceProj)reprojfromLatLon(maxx2,tempy);else reprojpoint_inv(maxx2,tempy);
    if(maxx1<maxx2)maxx1=maxx2;
    
    dfBBOX[1]=miny1;
    dfBBOX[3]=maxy1;
    dfBBOX[0]=minx1;
    dfBBOX[2]=maxx1;
    
    
    //Check if values are within allowable extent:
    if(dMaxExtentDefined==1){
      for(int j=0;j<2;j++)if(dfBBOX[j]<dfMaxExtent[j])dfBBOX[j]=dfMaxExtent[j];
      for(int j=2;j<4;j++)if(dfBBOX[j]>dfMaxExtent[j])dfBBOX[j]=dfMaxExtent[j];
      if(dfBBOX[0]<=dfBBOX[2])
        for(int j=0;j<4;j++)dfBBOX[j]=dfMaxExtent[j];

    }

//    CDBDebug("out: %f %f %f %f",dfBBOX[0],dfBBOX[1],dfBBOX[2],dfBBOX[3]);
    return 0;
  }
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

