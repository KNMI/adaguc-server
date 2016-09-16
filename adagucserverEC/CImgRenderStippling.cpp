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

#include "CImgRenderStippling.h"
#include "CGenericDataWarper.h"
int CImgRenderStippling::set(const char *settings){
  return 0;
}

const char *CImgRenderStippling::className="CImgRenderStippling";



//Setup projection and all other settings for the tiles to draw
void CImgRenderStippling::render(CImageWarper *warper,CDataSource *dataSource,CDrawImage *drawImage){
  CDBDebug("render");
  Settings settings;
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();    
  settings.dfNodataValue    = dataSource->getDataObject(0)->dfNodataValue ;
  settings.legendValueRange = styleConfiguration->hasLegendValueRange;
  settings.legendLowerRange = styleConfiguration->legendLowerRange;
  settings.legendUpperRange = styleConfiguration->legendUpperRange;
  settings.hasNodataValue   = dataSource->getDataObject(0)->hasNodataValue;
  
  if(!settings.hasNodataValue){
    settings.hasNodataValue = true;
    settings.dfNodataValue = -100000.f;
    
  }
  settings.width=drawImage->Geo->dWidth;
  settings.height=drawImage->Geo->dHeight;
  
  settings.dataField = new float[settings.width*settings.height];
  for(size_t y=0;y<settings.height;y++){
    for(size_t x=0;x<settings.width;x++){
      settings.dataField[x+y*settings.width]=(float)settings.dfNodataValue;
    }
  }
  
  
  CDFType dataType=dataSource->getDataObject(0)->cdfVariable->getType();
  void *sourceData = dataSource->getDataObject(0)->cdfVariable->data;
  CGeoParams sourceGeo;
  
  sourceGeo.dWidth = dataSource->dWidth;
  sourceGeo.dHeight = dataSource->dHeight;
  sourceGeo.dfBBOX[0] = dataSource->dfBBOX[0];
  sourceGeo.dfBBOX[1] = dataSource->dfBBOX[1];
  sourceGeo.dfBBOX[2] = dataSource->dfBBOX[2];
  sourceGeo.dfBBOX[3] = dataSource->dfBBOX[3];
  sourceGeo.dfCellSizeX = dataSource->dfCellSizeX;
  sourceGeo.dfCellSizeY = dataSource->dfCellSizeY;
  sourceGeo.CRS = dataSource->nativeProj4;
  
  switch(dataType){
    case CDF_CHAR  :  GenericDataWarper::render<char>  (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
    case CDF_BYTE  :  GenericDataWarper::render<char>  (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
    case CDF_UBYTE :  GenericDataWarper::render<unsigned char> (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
    case CDF_SHORT :  GenericDataWarper::render<short> (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
    case CDF_USHORT:  GenericDataWarper::render<ushort>(warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
    case CDF_INT   :  GenericDataWarper::render<int>   (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
    case CDF_UINT  :  GenericDataWarper::render<uint>  (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
    case CDF_FLOAT :  GenericDataWarper::render<float> (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
    case CDF_DOUBLE:  GenericDataWarper::render<double>(warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
  }
  
 
  
  int xDistance = 22;
  int yDistance = 22;
  int discSize = 6;
  
  
  if(styleConfiguration!=NULL&&styleConfiguration->styleConfig!=NULL){
    if(styleConfiguration->styleConfig->Stippling.size()==1){\
      if(!styleConfiguration->styleConfig->Stippling[0]->attr.distancex.empty()){
        xDistance = styleConfiguration->styleConfig->Stippling[0]->attr.distancex.toInt();
      }
      if(!styleConfiguration->styleConfig->Stippling[0]->attr.distancey.empty()){
        yDistance = styleConfiguration->styleConfig->Stippling[0]->attr.distancey.toInt();
      }
      if(!styleConfiguration->styleConfig->Stippling[0]->attr.discradius.empty()){
        discSize = styleConfiguration->styleConfig->Stippling[0]->attr.discradius.toInt();
      }
    }
  }
  
  
  double bboxWidth = (dataSource->srvParams->Geo->dfBBOX[2]-dataSource->srvParams->Geo->dfBBOX[0]);
  double bboxHeight = (dataSource->srvParams->Geo->dfBBOX[3]-dataSource->srvParams->Geo->dfBBOX[1]);

  
  int startX=int(((-dataSource->srvParams->Geo->dfBBOX[0])/bboxWidth)*dataSource->srvParams->Geo->dWidth);
  startX = (startX%(xDistance*2))-(xDistance*2);
  int startY=int(((dataSource->srvParams->Geo->dfBBOX[1])/bboxHeight)*dataSource->srvParams->Geo->dHeight);
  startY = (startY%(yDistance*2))-(yDistance*2);
/*  
  
  
  CT::string text;
  text.print("%d   %d  %d",startX,startY,fstartX);
  drawImage->setText(text.c_str(),text.length(),8,8,CColor(0,0,0,0),10);*/


  
  for(int y=startY;y<(int)settings.height;y=y+yDistance){
    int oddeven=0;
    if((y-startY)%(yDistance*2)==0){
      oddeven = 1;
    }
    for(int x1=startX;x1<(int)settings.width;x1=x1+xDistance){
      int x=(x1)+((oddeven)*(xDistance/2));
      if(x>=0&&y>=0&&x<((int)settings.width)&&y<((int)settings.height)){
        float val = settings.dataField[x+y*settings.width];
        if(val != (float)settings.dfNodataValue && (val==val)){
          if(styleConfiguration->legendLog!=0){
            if(val>0){
              val=(float)(log10(val)/styleConfiguration->legendLog);
            }else {
              val=(float)(-styleConfiguration->legendOffset);
            }
          }
          int pcolorind=(int)(val*styleConfiguration->legendScale+styleConfiguration->legendOffset);
          if(pcolorind>=239)pcolorind=239;else if(pcolorind<=0)pcolorind=0;
          CColor c =drawImage->getColorForIndex(pcolorind);
          drawImage->setDisc(x,y,discSize,c,c);
        }
    }
    }
  }
  settings.dataField = new float[settings.width*settings.height];
  
  
  
  delete[] settings.dataField;
  return;
}
