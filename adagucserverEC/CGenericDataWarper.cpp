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
    int sourceDataWidth = sourceGeoParams->dWidth;
    int sourceDataHeight = sourceGeoParams->dHeight;
      PXExtentBasedOnSource[0]=-1;
      PXExtentBasedOnSource[1]=-1;
      PXExtentBasedOnSource[2]=-1;
      PXExtentBasedOnSource[3]=-1;
    
//     PXExtentBasedOnSource[0]=0;
//     PXExtentBasedOnSource[1]=0;
//     PXExtentBasedOnSource[2]=sourceDataWidth;
//     PXExtentBasedOnSource[3]=sourceDataHeight;
//     return 0;
//  CDBDebug("start findPixelExtent");
//     bool destNeedsDegreeRadianConversion = false;
//     bool sourceNeedsDegreeRadianConversion = false;
//         CT::string destinationCRS;
//     warper->decodeCRS(&destinationCRS,&destGeoParams->CRS);
//       if(destinationCRS.indexOf("longlat")>=0){
//       destNeedsDegreeRadianConversion = true;
//     }
//     if(sourceGeoParams->CRS.indexOf("longlat")>=0){
//       sourceNeedsDegreeRadianConversion = true;
//     }
    
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

    
      int startX=0;
      int startY=0;
      int stopX=imageWidth;
      int stopY=imageHeight;
      bool firstExtent = true;
      bool needsProjection = warper->isProjectionRequired();
      bool OK = false;
      bool transFormationRequired=false;
      bool fullScan =false;
      
//       if(!needsProjection){
//         PXExtentBasedOnSource[0]=(double(0)/dfDestW)*dfDestExtW+dfDestOrigX;
//         PXExtentBasedOnSource[1]=(double(0)/dfDestH)*dfDestExtH+dfDestOrigY;
//         PXExtentBasedOnSource[2]=(double(imageWidth)/dfDestW)*dfDestExtW+dfDestOrigX;
//         PXExtentBasedOnSource[3]=(double(imageHeight)/dfDestH)*dfDestExtH+dfDestOrigY;
//         PXExtentBasedOnSource[0]=((PXExtentBasedOnSource[0]-dfSourceOrigX)/dfSourceExtW)*dfSourceW;
//         PXExtentBasedOnSource[1]=((PXExtentBasedOnSource[1]-dfSourceOrigY)/dfSourceExtH)*dfSourceH;
//         PXExtentBasedOnSource[2]=((PXExtentBasedOnSource[2]-dfSourceOrigX)/dfSourceExtW)*dfSourceW;
//         PXExtentBasedOnSource[3]=((PXExtentBasedOnSource[3]-dfSourceOrigY)/dfSourceExtH)*dfSourceH;
//         
//         
//         //CDBDebug("OK = [%d,%d,%d,%d]",PXExtentBasedOnSource[0],PXExtentBasedOnSource[1],PXExtentBasedOnSource[2],PXExtentBasedOnSource[3]);
//         transFormationRequired=true;
//       }else
      {
        while(OK==false){
          OK=true;
          
          bool attemptToContintue = true;
          
          if(warper->isProjectionRequired()){
            attemptToContintue = false;
            int incY = double(imageHeight)/4+0.5;
            int incX = double(imageWidth)/4+0.5;
  //           CDBDebug("%d %d",incX,incY);
            if(incY<1)incY=1;
            if(incX<1)incX=1;
            
            for(int y=startY;y<imageHeight+incY&&attemptToContintue==false;y=y+incY){
              for(int x=startX;x<imageWidth+incX&&attemptToContintue==false;x=x+incX){
  //               CDBDebug("Checking xy %d,%d",x,y);
                double px,py;
                px=(double(x)/dfDestW)*dfDestExtW+dfDestOrigX;
                py=(double(y)/dfDestH)*dfDestExtH+dfDestOrigY;
                if(warper->isProjectionRequired()){
                  if(warper->destNeedsDegreeRadianConversion){
                    px*=DEG_TO_RAD;
                    py*=DEG_TO_RAD;
                  }
                  if(!pj_transform(warper->destpj,warper->sourcepj, 1,0,&px,&py,NULL)){
                    attemptToContintue = true;
  //                   CDBDebug("OK!");
                  }
                }
              }
            }
          }
          
          
          

          if(attemptToContintue){
          
            for(int y=startY;y<stopY+1&&OK;y=y+1){
              for(int x=startX;x<stopX+1&&OK;x=x+1){
                if(x==startX||y==startY||x==stopX||y==stopY||fullScan==true){
                  double px,py;
                  px=(double(x)/dfDestW)*dfDestExtW+dfDestOrigX;
                  py=(double(y)/dfDestH)*dfDestExtH+dfDestOrigY;
                  //CDBDebug("pxpy: %f,%f",px,py);
                  
                  bool skip = false;
                  if(needsProjection)
                  {
                    if(warper->isProjectionRequired()){
                      if(warper->destNeedsDegreeRadianConversion){
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
                      if(warper->sourceNeedsDegreeRadianConversion){
                        px/=DEG_TO_RAD;
                        py/=DEG_TO_RAD;
                      }
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
                
                    if(OK==true&&fullScan==false){
                      OK=false;
                      fullScan = true;
    //                   if(x==startX)startX++;
    //                   if(x==stopX)stopX--;
    //                   if(y==startY)startY++;
    //                   if(y==stopY)stopY--;
    //                   break;
                    }
                  }
                }
              }
            }
            if(OK==true&&fullScan==true)break;
            PXExtentBasedOnSource[0]-=2;
            PXExtentBasedOnSource[1]-=2;
            PXExtentBasedOnSource[2]+=2;
            PXExtentBasedOnSource[3]+=2;
          }
        }

      }
      
      
      
       #ifdef GenericDataWarper_DEBUG
    
#endif
//CDBDebug("PXExtentBasedOnSource = [%d,%d,%d,%d]",PXExtentBasedOnSource[0],PXExtentBasedOnSource[1],PXExtentBasedOnSource[2],PXExtentBasedOnSource[3]);
    if(PXExtentBasedOnSource[1]>PXExtentBasedOnSource[3]){
      std::swap(PXExtentBasedOnSource[1],PXExtentBasedOnSource[3]);
    }
    if(PXExtentBasedOnSource[0]>PXExtentBasedOnSource[2]){
      std::swap(PXExtentBasedOnSource[0],PXExtentBasedOnSource[2]);
    }
    if(PXExtentBasedOnSource[0]<0)PXExtentBasedOnSource[0]=0;if(PXExtentBasedOnSource[0]>=sourceDataWidth)PXExtentBasedOnSource[0]=sourceDataWidth;
    if(PXExtentBasedOnSource[2]<0)PXExtentBasedOnSource[2]=0;if(PXExtentBasedOnSource[2]>=sourceDataWidth)PXExtentBasedOnSource[2]=sourceDataWidth;
    if(PXExtentBasedOnSource[1]<0)PXExtentBasedOnSource[1]=0;if(PXExtentBasedOnSource[1]>=sourceDataHeight)PXExtentBasedOnSource[1]=sourceDataHeight;
    if(PXExtentBasedOnSource[3]<0)PXExtentBasedOnSource[3]=0;if(PXExtentBasedOnSource[3]>=sourceDataHeight)PXExtentBasedOnSource[3]=sourceDataHeight;
    if(transFormationRequired==false){
      PXExtentBasedOnSource[0]=-1;
      PXExtentBasedOnSource[1]=-1;
      PXExtentBasedOnSource[2]=-1;
      PXExtentBasedOnSource[3]=-1;
    }
    
 #ifdef GenericDataWarper_DEBUG   
    CDBDebug("PXExtentBasedOnSource = [%d,%d,%d,%d]",PXExtentBasedOnSource[0],PXExtentBasedOnSource[1],PXExtentBasedOnSource[2],PXExtentBasedOnSource[3]);
#endif
    //CDBDebug("PXExtentBasedOnSource = [%d,%d,%d,%d]",PXExtentBasedOnSource[0],PXExtentBasedOnSource[1],PXExtentBasedOnSource[2],PXExtentBasedOnSource[3]);
    return 0;
}
