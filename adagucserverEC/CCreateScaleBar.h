#ifndef CCreateScaleBar_H
#define CCreateScaleBar_H

#include "Definitions.h"
#include "CStopWatch.h"
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CDebugger.h"


class CCreateScaleBar{
 
public:
  /**
   * Create scalebar creates a scalebar image for given geoParams.
   * @param scaleBarImage The CDrawImage object to write the scalebar to
   * @param geoParams The projection information to create the scalebar from, uses boundingbox and CRS.
   * @return 0 on success nonzero on failure.
   */
  static int createScaleBar(CDrawImage *scaleBarImage,CGeoParams *geoParams){
    CCreateScaleBar::Props p=CCreateScaleBar::getScaleBarProperties(geoParams);
  
    int offsetX=3;
    int scaleBarHeight = 20;
    
    
    for(int j=0;j<2;j++){
      scaleBarImage->line(offsetX,scaleBarHeight-2-j,p.width*2+offsetX,scaleBarHeight-2-j,240);
    }
    
    int subDivXW = p.width/5;
    for(int j=1;j<5;j++){
      scaleBarImage->line(offsetX+subDivXW*j,scaleBarHeight-2,offsetX+subDivXW*j,scaleBarHeight-2-3,240);
    }
    
    scaleBarImage->line(offsetX,scaleBarHeight-2,offsetX,scaleBarHeight-2-7,240);
    scaleBarImage->line(offsetX+p.width,scaleBarHeight-2,offsetX+p.width,scaleBarHeight-2-7,240);
    scaleBarImage->line(offsetX+p.width*2,scaleBarHeight-2,offsetX+p.width*2,scaleBarHeight-2-7,240);
    
    
    CT::string units="";
    CT::string projection = geoParams->CRS.c_str();
    if(projection.equals("EPSG:3411"))units="meter";
    if(projection.equals("EPSG:3412"))units="meter";
    if(projection.equals("EPSG:3575"))units="meter";
    if(projection.equals("EPSG:4326"))units="degrees";
    if(projection.equals("EPSG:28992"))units="meter";
    if(projection.equals("EPSG:32661"))units="meter";
    if(projection.equals("EPSG:3857"))units="meter";
    if(projection.equals("EPSG:900913"))units="meter";
    if(projection.equals("EPSG:102100"))units="meter";
    
    
    if(units.equals("meter")){
      if(p.mapunits>1000){
        p.mapunits/=1000;units="km";
      }
    }
    
    
    CT::string valueStr;
    
    scaleBarImage->drawText(offsetX-2,8,0,"0",240);
    
    valueStr.print("%d",p.mapunits);
    scaleBarImage->drawText(offsetX+p.width-valueStr.length()*2.5+1,8,0,valueStr.c_str(),240);
    
    valueStr.print("%d",p.mapunits*2);
    scaleBarImage->drawText(offsetX+p.width*2-valueStr.length()*2.5+1,8,0,valueStr.c_str(),240);
    
    scaleBarImage->drawText(offsetX+p.width*2+10,18,0,units.c_str(),240);
    return 0;
  }
  
private:
  class Props{
  public:
    int width;
    int mapunits;
  };
  
  static Props getScaleBarProperties(CGeoParams *geoParams){
    float desiredWidth = 25;
    float realWidth = 0;
    float numMapUnits=1./10000000.;
    
    
    float boxWidth = geoParams->dfBBOX[2]-geoParams->dfBBOX[0];
    float pixelsPerUnit = float(geoParams->dWidth)/boxWidth;
    if(pixelsPerUnit<=0){
      throw (__LINE__);
    }
    
    float a = desiredWidth/pixelsPerUnit;
    
    
    do{
      numMapUnits*=10.;
      float divFactor=a/numMapUnits;
      if(divFactor==0)throw (__LINE__);
      realWidth=desiredWidth/divFactor;
      
    }while(realWidth<desiredWidth);
    
    do{
      numMapUnits/=2.;
      float divFactor=a/numMapUnits;
      realWidth=desiredWidth/divFactor;
      
    }while(realWidth>desiredWidth);
    
    do{
      numMapUnits*=1.5;
      float divFactor=a/numMapUnits;
      realWidth=desiredWidth/divFactor;
      
    }while(realWidth<desiredWidth);
    
    
    float roundedMapUnits = numMapUnits;
    
    float d=pow(10,round(log10(numMapUnits)+0.5)-1);
    
    roundedMapUnits=int(roundedMapUnits/d);
    if(roundedMapUnits<2.5)roundedMapUnits=2.5;
    if(roundedMapUnits>2.5&&roundedMapUnits<7.5)roundedMapUnits=5;
    if(roundedMapUnits>7.5)roundedMapUnits=10;
    roundedMapUnits=(roundedMapUnits*d);
    
    
    float divFactor=(desiredWidth/pixelsPerUnit)/roundedMapUnits;
    realWidth=desiredWidth/divFactor;
    Props p;
    
    p.width = realWidth;
    
    p.mapunits = roundedMapUnits;
    
    return p;
  }
  
};
#endif
