#include "CGeoParams.h"

void CoordinatesXYtoScreenXY(double &x,double &y,CGeoParams *geoParam){
  x-=geoParam->dfBBOX[0];
  y-=geoParam->dfBBOX[3];
  double bboxW=geoParam->dfBBOX[2]-geoParam->dfBBOX[0];
  double bboxH=geoParam->dfBBOX[1]-geoParam->dfBBOX[3];
  x/=bboxW;
  y/=bboxH;
  x*=double(geoParam->dWidth);
  y*=double(geoParam->dHeight);
}

void CoordinatesXYtoScreenXY(CPoint &p,CGeoParams *geoParam){
  p.x-=geoParam->dfBBOX[0];
  p.y-=geoParam->dfBBOX[3];
  double bboxW=geoParam->dfBBOX[2]-geoParam->dfBBOX[0];
  double bboxH=geoParam->dfBBOX[1]-geoParam->dfBBOX[3];
  p.x/=bboxW;
  p.y/=bboxH;
  p.x*=double(geoParam->dWidth);
  p.y*=double(geoParam->dHeight);
}

void CoordinatesXYtoScreenXY(CBBOX &b,CGeoParams *geoParam){
  CoordinatesXYtoScreenXY(b.left,b.top,geoParam);
  CoordinatesXYtoScreenXY(b.right,b.bottom,geoParam);
}

