#include "CCreateLegend.h"
#include "CDataReader.h"
#include "CImageDataWriter.h"

int CCreateLegend::renderDiscreteLegend (CDataSource *dataSource,
  CDrawImage *legendImage, 
  CStyleConfiguration *styleConfiguration, 
  bool rotate, 
  bool estimateMinMax) {
      #ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("legendtype discrete");
    #endif
    float cbW = 20;//legendWidth/8;
    float legendHeight = legendImage->Geo->dHeight;
    float cbH = legendHeight-13-13;

    int pLeft=4;
    int pTop=(int)(legendImage->Geo->dHeight-legendHeight);
    char szTemp[256];
    

    CT::string textformatting;

    /* Take the textformatting from the Style->Legend configuration */
    if(styleConfiguration != NULL && 
      styleConfiguration->styleConfig != NULL && 
      styleConfiguration->styleConfig->Legend.size()>0 && 
      !styleConfiguration->styleConfig->Legend[0]->attr.textformatting.empty())
      {
        textformatting = styleConfiguration->styleConfig->Legend[0]->attr.textformatting;
      }

    CDBDebug("styleTitle = [%s]", styleConfiguration->styleTitle.c_str());
    CDBDebug("ShadeInterval = [%s]", styleConfiguration->styleConfig->ShadeInterval[0]->value.c_str());
    
    CDBDebug("textformatting = [%s]", textformatting.c_str());



    //int dH=0;
    //cbW = 90.0/3.0;
    // We always need to have the min/max of the data
    // Always to show only the occuring data values in the legend,
    // and in some cases to stretch the colors over min max
    
    //Get the min/max values
    float minValue = CImageDataWriter::getValueForColorIndex(dataSource,0);
    float maxValue = CImageDataWriter::getValueForColorIndex(dataSource,239);
    if(styleConfiguration->legendHasFixedMinMax==false){
      estimateMinMax = true;
    }
    if(estimateMinMax){
      if(dataSource->statistics==NULL){
        dataSource->statistics = new CDataSource::Statistics();
        dataSource->statistics->calculate(dataSource);
      }
      minValue=(float)dataSource->statistics->getMinimum();
      maxValue=(float)dataSource->statistics->getMaximum();
      
    }
    // }
   
    // CDBDebug("Using %f and %f for legend values", minValue, maxValue);
    
    //Calculate the number of classes
    float legendInterval=styleConfiguration->shadeInterval;
    int numClasses=8; //Default number of classes, if legendInterval not specified
    if (legendInterval!=0){
      numClasses=int((maxValue-minValue)/legendInterval);
    }
    
    /*
     *   // and reduce the number of classes when required...
     *   if(!dataSource->stretchMinMax){
     *     while(numClasses>15){
     *       legendInterval*=2;//(maxValue-minValue);
     *       numClasses=int((maxValue-minValue)/legendInterval);
  }
  }*/
    #ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("LayerName = %s",dataSource->layerName.c_str());
    CDBDebug("minValue=%f maxValue=%f",minValue,maxValue);
    CDBDebug("scale=%f offset=%f",styleConfiguration->legendScale,styleConfiguration->legendOffset);  
    #endif
    float iMin=convertValueToClass(minValue,legendInterval);
    float iMax=convertValueToClass(maxValue,legendInterval)+legendInterval;
    
    
    
    #ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("iMin=%f iMax=%f",iMin,iMax);
    #endif
    
    //In case of auto scale and autooffset we will stretch the colors over the min/max values
    //Calculate new scale and offset for the new min/max:
    //When using min/max stretching, the classes need to be extended according to its shade itnerval
    if(dataSource->stretchMinMax==true){
      //Calculate new scale and offset for the new min/max:
      float ls=240/((iMax-iMin));
      float lo=-(iMin*ls);
      if(ls == 0){
        ls=240;
        lo=0;
        
      }
      styleConfiguration->legendScale=ls;
      styleConfiguration->legendOffset=lo;
      //Check for infinities
      if(
        styleConfiguration->legendScale!=styleConfiguration->legendScale||
        styleConfiguration->legendScale==INFINITY||
        styleConfiguration->legendScale==NAN||
        styleConfiguration->legendScale==-INFINITY||
        styleConfiguration->legendOffset!=styleConfiguration->legendOffset||
        styleConfiguration->legendOffset==INFINITY||
        styleConfiguration->legendOffset==NAN||
        styleConfiguration->legendOffset==-INFINITY){
        styleConfiguration->legendScale=240.0;
        styleConfiguration->legendOffset=0;
        }
    }
    
    
    bool discreteLegendOnInterval=false;
    bool definedLegendOnShadeClasses=false;
    bool definedLegendForFeatures=false;
    
    if(styleConfiguration->shadeIntervals!=NULL){
      if(styleConfiguration->shadeIntervals->size()>0){
        definedLegendOnShadeClasses=true;
      }
    }
     if(styleConfiguration->featureIntervals!=NULL){
      if(styleConfiguration->featureIntervals->size()>0){
        definedLegendForFeatures=true;
      }
    }
    if(legendInterval!=0){
      discreteLegendOnInterval=true;
    }
    
    #ifdef CIMAGEDATAWRITER_DEBUG
    CDBDebug("legendtype settext");
    #endif
    /**
     * Defined blocks based on defined interval
     */
    
    if(definedLegendOnShadeClasses){
      char szTemp[1024];
      for(size_t j=0;j<styleConfiguration->shadeIntervals->size();j++){
        CServerConfig::XMLE_ShadeInterval *s=(*styleConfiguration->shadeIntervals)[j];
        if(s->attr.min.empty()==false&&s->attr.max.empty()==false){
          int cY1 = int(cbH-(j*12));
          int cY2 = int(cbH-(((j+1)*12)-2));
          CColor color;
          if(s->attr.fillcolor.empty()==false){
            color=CColor(s->attr.fillcolor.c_str());
          }else{
            color=legendImage->getColorForIndex(CImageDataWriter::getColorIndexForValue(dataSource,parseFloat(s->attr.min.c_str())));
          }
          legendImage->rectangle(4+pLeft,cY2+pTop,int(cbW)+7+pLeft,cY1+pTop,color,CColor(0,0,0,255));
          
          if(s->attr.label.empty()){
            snprintf(szTemp,1000,"%s - %s",s->attr.min.c_str(),s->attr.max.c_str());
          }else{
            snprintf(szTemp,1000,"%s",s->attr.label.c_str());
          }
          
          legendImage->setText(szTemp,strlen(szTemp),int(cbW)+12+pLeft,cY2+pTop,248,-1);
        }
      }
    }
    
    
    if(definedLegendForFeatures){
      
      char szTemp[1024];
      for(size_t j=0;j<styleConfiguration->featureIntervals->size();j++){
        CServerConfig::XMLE_FeatureInterval *s=(*styleConfiguration->featureIntervals)[j];
//         if(s->attr.min.empty()==false&&s->attr.max.empty()==false){
          int cY1 = int(cbH-(j*12));
          int cY2 = int(cbH-(((j+1)*12)-2));
          CColor color;
         
            color=CColor(s->attr.fillcolor.c_str());
        
          legendImage->rectangle(4+pLeft,cY2+pTop,int(cbW)+7+pLeft,cY1+pTop,color,CColor(0,0,0,255));
     /*     
          if(s->attr.label.empty()){
            snprintf(szTemp,1000,"%s - %s",s->attr.min.c_str(),s->attr.max.c_str());
          }else{
            snprintf(szTemp,1000,"%s",s->attr.label.c_str());
          }*/
          snprintf(szTemp,1000,"%s",s->attr.label.c_str());
          legendImage->setText(szTemp,strlen(szTemp),int(cbW)+12+pLeft,cY2+pTop,248,-1);
        }
      
    }
    
    
    
    
    if(discreteLegendOnInterval){
      /* 
       * maxIterations is the maximum number of labels in the legendgraphic, more makes no sense as legend image is quite small 
       * Sometimes this function tries to render thousands of labels in a few pixels, maxIterations prevents this.       
       */
      int maxIterations = 250; 
      int currentIteration = 0;
      
      /**
       * Discrete blocks based on continous interval
       */
      
      numClasses=int((iMax-iMin)/legendInterval);
      
      int classSizeY=((legendHeight-24)/(numClasses));
      if(classSizeY>12)classSizeY=12;
      
      
      // Rounding of legend text depends on legendInterval
      if(legendInterval==0){
        CDBError("legendInterval is zero");
        return 1;
      }
      int textRounding=0;
      if(legendInterval!=0){
        float fracPart=legendInterval-int(legendInterval);
        if (fracPart == 0) {
          textRounding = 0;
        }else {
          textRounding=-int(log10(fracPart)-0.9999999f);
        }
      }
      
      
      int classNr=0;
      for(float j=iMin;j<iMax+legendInterval;j=j+legendInterval){
        currentIteration++;
        if (currentIteration> maxIterations)break;
        float v=j;
        int cY= int((cbH-(classNr-5))+6);
        
        int dDistanceBetweenClasses=(classSizeY-10);
        if(dDistanceBetweenClasses<4){dDistanceBetweenClasses=2;};
        if(dDistanceBetweenClasses>4)dDistanceBetweenClasses=4;
        cY-=dDistanceBetweenClasses;
        int cY2=int((cbH-(classNr+classSizeY-5))+6);
        classNr+=classSizeY;
        
        if(j<iMax)
        {
          int y=CImageDataWriter::getColorIndexForValue(dataSource,v);
          if(classSizeY>4){
            legendImage->rectangle(4+pLeft,cY2+pTop,int(cbW)+7+pLeft,cY+pTop,(y),248);
          }else{
            legendImage->rectangle(4+pLeft,cY2+pTop,int(cbW)+7+pLeft,cY+pTop,(y),(y));
          }
          if (textformatting.empty() == false) {
            CT::string textFormat;
            textFormat.print("%s - %s", textformatting.c_str(), textformatting.c_str());
            sprintf(szTemp, textFormat.c_str(),v,v+legendInterval);
          } else {
            if(textRounding<=0)sprintf(szTemp,"%2.0f - %2.0f",v,v+legendInterval);
            if(textRounding==1)sprintf(szTemp,"%2.1f - %2.1f",v,v+legendInterval);
            if(textRounding==2)sprintf(szTemp,"%2.2f - %2.2f",v,v+legendInterval);
            if(textRounding==3)sprintf(szTemp,"%2.3f - %2.3f",v,v+legendInterval);
            if(textRounding==4)sprintf(szTemp,"%2.4f - %2.4f",v,v+legendInterval);
            if(textRounding==5)sprintf(szTemp,"%2.5f - %2.5f",v,v+legendInterval);
            if(textRounding==5)sprintf(szTemp,"%2.6f - %2.6f",v,v+legendInterval);
            if(textRounding>6)sprintf(szTemp,"%f - %f",v,v+legendInterval);
          }
          int l=strlen(szTemp);
          legendImage->setText(szTemp,l,(int)cbW+10+pLeft,((cY+cY2)/2)-6+pTop,248,-1);
        }
        
      }
    }
    
    
    #ifdef CIMAGEDATAWRITER_DEBUG
    
    CDBDebug("set units");
    #endif
    //Get units
    CT::string units;
    if(dataSource->getDataObject(0)->getUnits().length()>0){
      units.concat(dataSource->getDataObject(0)->getUnits().c_str());
    }
    if(units.length()>0)legendImage->setText(units.c_str(),units.length(),2+pLeft,int(legendHeight)-14+pTop,248,-1);
    //legendImage->crop(4,4);
    return 0;
}