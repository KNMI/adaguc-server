#include "CCreateScaleBar.h"
  int CCreateScaleBar::createScaleBar(CDrawImage *scaleBarImage,CGeoParams *geoParams){
      
    CCreateScaleBar::Props p=CCreateScaleBar::getScaleBarProperties(geoParams);
  
    int offsetX=3;
    int scaleBarHeight = 23;
 
    
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
    
    scaleBarImage->drawText(offsetX-2,12,0,"0",240);
    
    valueStr.print("%g",(p.mapunits));
    scaleBarImage->drawText(offsetX+p.width-valueStr.length()*2.5+0,12,0,valueStr.c_str(),240);
    
    valueStr.print("%g",(p.mapunits*2));
    scaleBarImage->drawText(offsetX+p.width*2-valueStr.length()*2.5+0,12,0,valueStr.c_str(),240);
    
    scaleBarImage->drawText(offsetX+p.width*2+10,18,0,units.c_str(),240);
    return 0;
  }
  
  
  CCreateScaleBar::Props CCreateScaleBar::getScaleBarProperties(CGeoParams *geoParams){
    double desiredWidth = 25;
    double realWidth = 0;
    double numMapUnits=1./10000000.;
    
    
    double boxWidth = geoParams->dfBBOX[2]-geoParams->dfBBOX[0];
    double pixelsPerUnit = double(geoParams->dWidth)/boxWidth;
    if(pixelsPerUnit<=0){
      throw (__LINE__);
    }
    
    double a = desiredWidth/pixelsPerUnit;
    
    double divFactor=0;
    do{
      numMapUnits*=10.;
      divFactor=a/numMapUnits;
      if(divFactor==0)throw (__LINE__);
      realWidth=desiredWidth/divFactor;
      
    }while(realWidth<desiredWidth);
    
    do{
      numMapUnits/=2.;
      divFactor=a/numMapUnits;
      if(divFactor==0)throw (__LINE__);
      realWidth=desiredWidth/divFactor;
      
    }while(realWidth>desiredWidth);
    
    do{
      numMapUnits*=1.2;
      divFactor=a/numMapUnits;
      if(divFactor==0)throw (__LINE__);
      realWidth=desiredWidth/divFactor;
      
    }while(realWidth<desiredWidth);
    
    
    double roundedMapUnits = numMapUnits;
    
    double d=pow(10,round(log10(numMapUnits)+0.5)-1);
    
    roundedMapUnits=round(roundedMapUnits/d);
    if(roundedMapUnits<2.5)roundedMapUnits=2.5;
    if(roundedMapUnits>2.5&&roundedMapUnits<7.5)roundedMapUnits=5;
    if(roundedMapUnits>7.5)roundedMapUnits=10;
    roundedMapUnits=(roundedMapUnits*d);
    
    
    divFactor=(desiredWidth/pixelsPerUnit)/roundedMapUnits;
    realWidth=desiredWidth/divFactor;
    Props p;
    
    p.width = realWidth;
    
    
    p.mapunits =roundedMapUnits;
    
    return p;
  }
  
