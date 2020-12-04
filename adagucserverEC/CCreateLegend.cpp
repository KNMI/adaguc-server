#include "CCreateLegend.h"
#include "CCreateLegendRenderDiscreteLegend.cpp"
#include "CDataReader.h"
#include "CImageDataWriter.h"

const char * CCreateLegend::className = "CCreateLegend";

int CCreateLegend::createLegend(CDataSource *dataSource,CDrawImage *legendImage) {
  createLegend(dataSource, legendImage, false);
  return 0;
}

int CCreateLegend::createLegend(CDataSource *dataSource,CDrawImage *legendImage, bool rotate){
  #ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("createLegend");
  #endif
  
  if(dataSource->cfgLayer != NULL){
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    if(styleConfiguration!=NULL){
      if(styleConfiguration->styleConfig!=NULL){
        if(styleConfiguration->styleConfig->LegendGraphic.size() == 1){
          if(styleConfiguration->styleConfig->LegendGraphic[0]->attr.value.empty()==false){
            const char *fileName =styleConfiguration->styleConfig->LegendGraphic[0]->attr.value.c_str();
            legendImage->destroyImage();
            legendImage->createImage(fileName);
            return 0;
          }
        }
      }
    }
  }
  
  int status = 0;
  enum LegendType { undefined,continous,discrete,statusflag,cascaded};
  LegendType legendType=undefined;
  bool estimateMinMax=false;
  
  int legendPositiveUp = 1;
//   float legendWidth = legendImage->Geo->dWidth;
  float legendHeight = legendImage->Geo->dHeight;
  
  int pLeft=4;
  int pTop=(int)(legendImage->Geo->dHeight-legendHeight);
  char szTemp[256];
  
  if(dataSource->dLayerType==CConfigReaderLayerTypeCascaded){
    legendType = cascaded;
    CDBDebug("GetLegendGraphic for cascaded WMS is not yet supported");
    legendImage->crop(4);
    return 0;
  }
  
  CDataReader reader;
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  if (styleConfiguration == NULL) {
    CDBError("No style configuration");
    return 1;
  }
  CStyleConfiguration::RenderMethod renderMethod = styleConfiguration->renderMethod;
  
  if(renderMethod&RM_RGBA||renderMethod&RM_AVG_RGBA){
    
    //legendImage->setText("",5,0,0,248,-1);  
    legendImage->crop(4);
    return 0;
  }
  //legendImage->enableTransparency(true);
  //legendImage->rectangle(0,0,20,20,CColor(255,255,255,255),CColor(255,255,255,255));
  //legendImage->setText("RGBA",5,0,0,255,-1);  
  //legendImage->crop(40,40);
  //return 0;
  if(legendType == cascaded){
    legendImage->crop(4);
    return 0;
  }
  
  if(styleConfiguration->legendScale==0.0f || styleConfiguration->legendHasFixedMinMax==false){
    estimateMinMax = true;
  }else{
    estimateMinMax = false;
  }
   
  if(dataSource->srvParams->wmsExtensions.colorScaleRangeSet&&dataSource->srvParams->wmsExtensions.numColorBandsSet){
    estimateMinMax = false;
  }
  
  if(strlen(dataSource->getFileName())>0){
    if(estimateMinMax){
      #ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Opening CNETCDFREADER_MODE_OPEN_ALL");
      #endif
      status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_ALL);
    }else{
      #ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Opening CNETCDFREADER_MODE_OPEN_HEADER");
      #endif
      status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
    }
    
    if(status!=0){
      CDBError("Unable to open file");
      return 1;
    }
  }else{
    estimateMinMax = false;
    #ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("createLegend without any file information");
    #endif
    
  }
  //}
  #ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("Determine legendtype");
  #endif
  
  //Determine legendtype.
  if(dataSource->getDataObject(0)->hasStatusFlag){
    legendType = statusflag;
  }else if (renderMethod&RM_POINT) {
    if ((styleConfiguration!=NULL)&&(styleConfiguration->shadeIntervals!=NULL)&&(styleConfiguration->shadeIntervals->size()>0)) {
        legendType=discrete;
      } else {
        legendType=continous;
      }
  }else if(!(renderMethod&RM_SHADED||renderMethod&RM_CONTOUR)){
    legendType = continous;
  }else{
    if(!(renderMethod&RM_SHADED)){
      if(renderMethod&RM_NEAREST || renderMethod&RM_BILINEAR){
        legendType = continous;
      }else{
        legendType = cascaded;
      }
    }else{
      legendType = discrete;
    }
  }
  
   if(styleConfiguration->featureIntervals!=NULL){
      if(styleConfiguration->featureIntervals->size()>0){
        legendType = discrete;
      }
   }
  
  /*
   * if(legendType==continous){
   *   if(legendHeight>280)legendHeight=280;
}*/
  
  
  //Create a legend based on status flags.
  if( legendType == statusflag ){
    #ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("legendtype statusflag");
    #endif
    int dH=30;
    //cbW=LEGEND_WIDTH/3;cbW/=3;cbW*=3;cbW+=3;
    float cbW = 20;//legendWidth/8;
    float cbH = legendHeight-13-13-30;
    
    bool useShadeIntervals=false;
    if ((styleConfiguration!=NULL)&&(styleConfiguration->shadeIntervals!=NULL)&&(styleConfiguration->shadeIntervals->size()>0)) {
      useShadeIntervals=true;
    }
    
    int blockHeight = 18;
    int blockDistance=20;
    
    size_t numFlags=dataSource->getDataObject(0)->statusFlagList.size();
    while(numFlags*blockDistance>legendHeight-14&&blockDistance>5){
      blockDistance--;
      if(blockHeight>5){
        blockHeight--;
      }
    }
    
    CColor black(0,0,0,255);
    
    for(size_t j=0;j<numFlags;j++){
      float y=j*blockDistance+(cbH-numFlags*blockDistance+8);
      double value=dataSource->getDataObject(0)->statusFlagList[j]->value;
      if (useShadeIntervals) {
        CColor col=CImageDataWriter::getPixelColorForValue(dataSource, value);
        legendImage->rectangle(1+pLeft,int(2+dH+y)+pTop,(int)cbW+9+pLeft,(int)y+2+dH+blockHeight+pTop, col, black);
      } else {
        int c=CImageDataWriter::getColorIndexForValue(dataSource,value);
        legendImage->rectangle(1+pLeft,int(2+dH+y)+pTop,(int)cbW+9+pLeft,(int)y+2+dH+blockHeight+pTop,c,248);
      }
      
      CT::string flagMeaning;
      CDataSource::getFlagMeaningHumanReadable(&flagMeaning,&dataSource->getDataObject(0)->statusFlagList,value);
      CT::string legendMessage;
      legendMessage.print("%d) %s",(int)value,flagMeaning.c_str());
      legendImage->setText(legendMessage.c_str(),legendMessage.length(),(int)cbW+15+pLeft,(int)y+dH+2+pTop,248,-1);  
    }
//     CT::string units="status flag";
//     legendImage->setText(units.c_str(),units.length(),2+pLeft,int(legendHeight)-14+pTop,248,-1);
    //legendImage->crop(4,4);
  }
  
  //Draw a continous legend
  if( legendType == continous ){
    #ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("legendtype continous");
    #endif
    bool drawUpperTriangle = false;
    bool drawLowerTriangle = false;
    
    if(styleConfiguration->hasLegendValueRange){
      drawUpperTriangle = true;
      drawLowerTriangle = true;
      
      float minValue = CImageDataWriter::getValueForColorIndex(dataSource,0);
      float maxValue = CImageDataWriter::getValueForColorIndex(dataSource,239);
      if(styleConfiguration->legendLowerRange>=minValue){
        drawLowerTriangle = false;
      }
      if(styleConfiguration->legendUpperRange<=maxValue){
        drawUpperTriangle = false;
      }
    }
    
    
    drawUpperTriangle = true;
    drawLowerTriangle = true;
    
    float triangleShape = 1.1;
    
    
    
    float cbW = 16;//legendWidth/8;
    float triangleHeight = int(cbW/triangleShape);
    float cbH = legendHeight-13-13;
    int dH=0;
    
    if(drawUpperTriangle){
      dH+=int(triangleHeight);
      cbH-=triangleHeight;
    }
    if(drawLowerTriangle){
      cbH-=triangleHeight;
    }
    
    
    
    int minColor;
    int maxColor;
    for(int j=0;j<cbH;j++){
      //for(int i=0;i<cbW+3;i++){
      float c=(float(cbH*legendPositiveUp-(j+1))/cbH)*240.0f;
      for(int x=pLeft;x<pLeft+(int)cbW+1;x++){
        legendImage->setPixelIndexed(x,j+7+dH+pTop,int(c));
      }
      //legendImage->setPixelIndexed(i+pLeft,j+7+dH+pTop,int(c));
      //legendImage->line_noaa(pLeft,j+7+dH+pTop,pLeft+(int)cbW+1,j+7+dH+pTop,int(c));
      if(j==0)minColor = int(c);
      maxColor = int(c);
      // }
    }
    //legendImage->rectangle(pLeft,7+dH+pTop,(int)cbW+3+pLeft,(int)cbH+7+dH+pTop,248);
    
    legendImage->line(pLeft,6+dH+pTop,pLeft,(int)cbH+7+dH+pTop,0.8,248);
    legendImage->line((int)cbW+1+pLeft,6+dH+pTop,(int)cbW+1+pLeft,(int)cbH+7+dH+pTop,0.8,248);
    
    int triangleLX= pLeft;
    int triangleRX= (int)cbW+1+pLeft;
    int triangle1BY= 7+dH+pTop-1
    ;
    int triangleMX = (int)cbW+pLeft-int(cbW/2.0);
    int triangle1TY = int(7+dH+pTop-triangleHeight);
    
    int triangle2TY= (int)cbH+7+dH+pTop;
    int triangle2BY = int(cbH+7+dH+pTop+triangleHeight);
    
    if(drawUpperTriangle){
      //Draw upper triangle
      for(int j=0;j<(triangle1BY-triangle1TY)+1;j++){
        int dx=int((float(j)/float(triangle1BY-triangle1TY))*cbW/2.0);
        //int dx=int((float(j)/float((triangle2BY-triangle2TY)))*cbW/2.0);
//         legendImage->line(triangleLX+dx,triangle1BY-j,triangleRX-dx-0.5,triangle1BY-j,minColor);
        for(int x=triangleLX+dx;x<triangleRX-dx-0.5;x++){
          legendImage->setPixelIndexed(x,triangle1BY-j,int(minColor));
        }
      }
      legendImage->line(triangleLX,triangle1BY,triangleMX,triangle1TY-1,0.8,248);
      legendImage->line(triangleRX,triangle1BY,triangleMX,triangle1TY-1,0.8,248);
    }else{
      legendImage->line(triangleLX,triangle1BY+1,triangleRX,triangle1BY+1,0.8,248);
    }
    
    if(drawLowerTriangle){
      //Draw lower triangle
      for(int j=0;j<(triangle2BY-triangle2TY)+1;j++){
        int dx=int((float(j)/float((triangle2BY-triangle2TY)))*cbW/2.0);
        //legendImage->line(triangleLX+dx+0.5,triangle2TY+j,triangleRX-dx-0.5,triangle2TY+j,maxColor);
        for(int x=triangleLX+dx+0.5;x<triangleRX-dx-0.5;x++){
          legendImage->setPixelIndexed(x,triangle2TY+j,int(maxColor));
        }
      }

      legendImage->line(triangleLX,triangle2TY,triangleMX,triangle2BY+1,0.6,248);
      legendImage->line(triangleRX,triangle2TY,triangleMX,triangle2BY+1,0.6,248);
    }else{
      legendImage->line(triangleLX,triangle2TY,triangleRX,triangle2TY,0.8,248);
    }
    
    double classes=6;
    int tickRound=0;
    double min=CImageDataWriter::getValueForColorIndex(dataSource,0);
    double max=CImageDataWriter::getValueForColorIndex(dataSource,240);//TODO CHECK 239
    if(max == INFINITY)max=239;
    if(min == INFINITY)min=0;
    if(max == min)max=max+0.000001;
    double increment = (max-min)/classes;
    if(styleConfiguration->legendTickInterval>0){
      //classes=(max-min)/styleConfiguration->legendTickInterval;
      //classes=int((max-min)/double(styleConfiguration->legendTickInterval)+0.5);
      increment = double(styleConfiguration->legendTickInterval);
    }
    if(increment<=0)increment=1;
    
    
    
    
    if(styleConfiguration->legendTickRound>0){
      tickRound = int(round(log10(styleConfiguration->legendTickRound))+3);
    }
    
    if(increment>max-min){
      increment = max-min;
    }
    if((max-min)/increment>250)increment=(max-min)/250;
    //CDBDebug("%f %f %f",min,max,increment);
    if(increment<=0){
      CDBError("Increment is 0, setting to 1");
      increment=1;
    }
    classes=abs(int((max-min)/increment));
    
    if(styleConfiguration->legendLog!=0){
      for(int j=0;j<=classes;j++){
        double c=((double(classes*legendPositiveUp-j)/classes))*(cbH);
        double v=((double(j)/classes))*(240.0f);
        v-=styleConfiguration->legendOffset;
        if(styleConfiguration->legendScale != 0)v/=styleConfiguration->legendScale;
        if(styleConfiguration->legendLog!=0){v=pow(styleConfiguration->legendLog,v);}
        {
          float lineWidth=0.8;
          legendImage->line((int)cbW-1+pLeft,(int)c+6+dH+pTop,(int)cbW+6+pLeft,(int)c+6+dH+pTop,lineWidth,248);
          if(tickRound==0){
            floatToString(szTemp,255,min,max,v);
          }else{
            floatToString(szTemp,255,tickRound,v);
          }
          legendImage->setText(szTemp,strlen(szTemp),(int)cbW+10+pLeft,(int)c+dH+pTop+1,248,-1);
        }
      }
    }else{

    
      //CDBDebug("LEGEND: scale %f offset %f",styleConfiguration->legendScale,styleConfiguration->legendOffset);
      for(double j=min;j<max+increment;j=j+increment){
        //CDBDebug("%f",j);
        double lineY = cbH-((j-min)/(max-min))*cbH;
        //CDBDebug("%f %f %f",j,lineY,cbH);
        //double c=((double(classes*legendPositiveUp-j)/classes))*(cbH);
        double v=j;//pow(j,10);
        //v-=styleConfiguration->legendOffset;
        
        //       if(styleConfiguration->legendScale != 0)v/=styleConfiguration->legendScale;
        //       if(styleConfiguration->legendLog!=0){v=pow(styleConfiguration->legendLog,v);}
        
        if(lineY>=-2&&lineY<cbH+2)
        {
          float lineWidth=0.8;
          legendImage->line((int)cbW-1+pLeft,(int)lineY+6+dH+pTop,(int)cbW+6+pLeft,(int)lineY+6+dH+pTop,lineWidth,248);
          if(tickRound==0){
            floatToString(szTemp,255,min,max,v);
          }else{
            floatToString(szTemp,255,tickRound,v);
          }
          legendImage->setText(szTemp,strlen(szTemp),(int)cbW+10+pLeft,(int)lineY+dH+pTop+1,248,-1);
        }
      }
    }
    //Get units
    CT::string units;
    if(dataSource->getDataObject(0)->getUnits().length()>0){
      units.concat(dataSource->getDataObject(0)->getUnits().c_str());
    }
    if(units.length()==0){
      units="-";
    }
      
    legendImage->setText(units.c_str(),units.length(),2+pLeft,int(legendHeight)-14+pTop,248,-1);
    //legendImage->crop(2,2);    
    //return 0;
  }
  
  

  //Draw legend with fixed intervals
  if( legendType == discrete){
    if (renderDiscreteLegend(dataSource,legendImage, styleConfiguration, rotate, estimateMinMax) != 0){
      CDBError("renderDiscreteLegend did not succeed.");
      return 1;
    }
  }
  
  
  reader.close();
  
  
  #ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("cropping");
  #endif
  
  legendImage->crop(5);
  if (rotate) {
    CDBDebug("rotate");
    legendImage->rotate();
  }
  return 0;
} 


