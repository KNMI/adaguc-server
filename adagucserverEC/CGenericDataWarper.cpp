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

#include "CGenericDataWarper.h"
const char *GenericDataWarper::className="GenericDataWarper";

int GenericDataWarper::findPixelExtent(int *PXExtentBasedOnSource,CGeoParams*sourceGeoParams,CGeoParams*destGeoParams,CImageWarper*warper){
//  CDBDebug("start findPixelExtent");
    bool destNeedsDegreeRadianConversion = false;
    bool sourceNeedsDegreeRadianConversion = false;
        CT::string destinationCRS;
    warper->decodeCRS(&destinationCRS,&destGeoParams->CRS);
      if(destinationCRS.indexOf("longlat")>=0){
      destNeedsDegreeRadianConversion = true;
    }
    if(sourceGeoParams->CRS.indexOf("longlat")>=0){
      sourceNeedsDegreeRadianConversion = true;
    }
    
    double dfSourceExtW=(sourceGeoParams->dfBBOX[2]-sourceGeoParams->dfBBOX[0]);
    double dfSourceExtH=(sourceGeoParams->dfBBOX[3]-sourceGeoParams->dfBBOX[1]);
    double dfSourceW = double(sourceGeoParams->dWidth);
    double dfSourceH = double(sourceGeoParams->dHeight);
    
    int imageHeight = destGeoParams->dHeight;
    int imageWidth = destGeoParams->dWidth;
    double dfDestW = double(destGeoParams->dWidth);
    double dfDestH = double(destGeoParams->dHeight);

//     double dfSourcedExtW=dfSourceExtW/(dfSourceW);
//     double dfSourcedExtH=dfSourceExtH/(dfSourceH);
    double dfSourceOrigX=sourceGeoParams->dfBBOX[0];
    double dfSourceOrigY=sourceGeoParams->dfBBOX[1];
    
    double dfDestExtW = destGeoParams->dfBBOX[2]-destGeoParams->dfBBOX[0];
    double dfDestExtH = destGeoParams->dfBBOX[1]-destGeoParams->dfBBOX[3];
    double dfDestOrigX = destGeoParams->dfBBOX[0];
    double dfDestOrigY = destGeoParams->dfBBOX[3];

  /*  
    double multiDestX = double(imageWidth)/dfDestExtW;
    
    double multiDestY = double(imageHeight)/dfDestExtH;
    */
        int sourceDataWidth = sourceGeoParams->dWidth;
    int sourceDataHeight = sourceGeoParams->dHeight;
    
      int startX=0;
      int startY=0;
      bool firstExtent = true;
      
      bool OK = false;
      bool transFormationRequired=false;
      while(OK==false){
      
        OK=true;
        for(int y=startY;y<imageHeight-startY;y=y+1){
          for(int x=startX;x<imageWidth-startX;x=x+1){
            if(x==startX||y==startY||x==imageWidth-1-startX||y==imageHeight-1-startY){
              double px,py;
              px=(double(x)/dfDestW)*dfDestExtW+dfDestOrigX;
              py=(double(y)/dfDestH)*dfDestExtH+dfDestOrigY;
              //CDBDebug("pxpy: %f,%f",px,py);
              
              bool skip = false;
              if(warper->isProjectionRequired()){
                if(destNeedsDegreeRadianConversion){
                  px*=DEG_TO_RAD;
                  py*=DEG_TO_RAD;
                }
                //CDBDebug("pxpy: %f,%f",px,py);
                if(pj_transform(warper->destpj,warper->sourcepj, 1,0,&px,&py,NULL)){
                  skip = true;
//                   px/=DEG_TO_RAD;
//                   py/=DEG_TO_RAD;
//                   CDBDebug("skip %f %f",px,py);
                }
                if(sourceNeedsDegreeRadianConversion){
                  px/=DEG_TO_RAD;
                  py/=DEG_TO_RAD;
                }
              }
                //CDBDebug("pxpy: %f,%f",px,py);
              if(!skip){
                transFormationRequired=true;
                //CDBDebug("pxpy: %f,%f",px,py);
                px=((px-dfSourceOrigX)/dfSourceExtW)*dfSourceW;
                py=((py-dfSourceOrigY)/dfSourceExtH)*dfSourceH;
                //CDBDebug("pxpy: %f,%f",px,py);
                if(firstExtent){
                  PXExtentBasedOnSource[0]=int(px);
                  PXExtentBasedOnSource[1]=int(py);
                  PXExtentBasedOnSource[2]=int(px);
                  PXExtentBasedOnSource[3]=int(py);
                  firstExtent=false;
                }else{
                  if(px<PXExtentBasedOnSource[0])PXExtentBasedOnSource[0]=px;
                  if(px>PXExtentBasedOnSource[2])PXExtentBasedOnSource[2]=px;
                  if(py<PXExtentBasedOnSource[1])PXExtentBasedOnSource[1]=py;
                  if(py>PXExtentBasedOnSource[3])PXExtentBasedOnSource[3]=py;
                }
              
              }else{
             
                if(OK==true){
                  OK=false;
                   startX+=((double(imageWidth)/10.)+1);
                   startY+=((double(imageHeight)/10.)+1);
                //CDBDebug("PXEXTent error increasing to %d/%d",startX,startY);
                }
              }
            }
          }
        }
      }
      PXExtentBasedOnSource[0]-=2;
      PXExtentBasedOnSource[1]-=2;
      PXExtentBasedOnSource[2]+=2;
      PXExtentBasedOnSource[3]+=2;
      
      
      
      
       #ifdef GenericDataWarper_DEBUG
    CDBDebug("PXExtentBasedOnSource = [%d,%d,%d,%d]",PXExtentBasedOnSource[0],PXExtentBasedOnSource[1],PXExtentBasedOnSource[2],PXExtentBasedOnSource[3]);
#endif
    if(PXExtentBasedOnSource[0]<0)PXExtentBasedOnSource[0]=0;if(PXExtentBasedOnSource[0]>=sourceDataWidth)PXExtentBasedOnSource[0]=sourceDataWidth;
    if(PXExtentBasedOnSource[2]<0)PXExtentBasedOnSource[2]=0;if(PXExtentBasedOnSource[2]>=sourceDataWidth)PXExtentBasedOnSource[2]=sourceDataWidth;
    if(PXExtentBasedOnSource[1]<0)PXExtentBasedOnSource[1]=0;if(PXExtentBasedOnSource[1]>=sourceDataHeight)PXExtentBasedOnSource[1]=sourceDataHeight;
    if(PXExtentBasedOnSource[3]<0)PXExtentBasedOnSource[3]=0;if(PXExtentBasedOnSource[3]>=sourceDataHeight)PXExtentBasedOnSource[3]=sourceDataHeight;
    if(PXExtentBasedOnSource[1]>PXExtentBasedOnSource[3]){
      std::swap(PXExtentBasedOnSource[1],PXExtentBasedOnSource[3]);
    }
    if(PXExtentBasedOnSource[0]>PXExtentBasedOnSource[2]){
      std::swap(PXExtentBasedOnSource[0],PXExtentBasedOnSource[2]);
    }
    if(transFormationRequired==false){
      PXExtentBasedOnSource[0]=-1;
      PXExtentBasedOnSource[1]=-1;
      PXExtentBasedOnSource[2]=-1;
      PXExtentBasedOnSource[3]=-1;
    }
    
 #ifdef GenericDataWarper_DEBUG   
    CDBDebug("PXExtentBasedOnSource = [%d,%d,%d,%d]",PXExtentBasedOnSource[0],PXExtentBasedOnSource[1],PXExtentBasedOnSource[2],PXExtentBasedOnSource[3]);
#endif
    return 0;
}
