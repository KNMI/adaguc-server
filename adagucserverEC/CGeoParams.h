#ifndef CGeoParams_H
#define CGeoParams_H
#include "CTypes.h"
class CGeoParams{
  public:
    int dWidth,dHeight;
    double dfBBOX[4];
    CT::string CRS;
    CGeoParams(){
      dWidth=1;dHeight=1;
    }
    int copy(CGeoParams * _Geo){
      if(_Geo==NULL)return 1;
      dWidth=_Geo->dWidth;
      dHeight=_Geo->dHeight;
      CRS.copy(&_Geo->CRS);
      for(int j=0;j<4;j++)dfBBOX[j]=_Geo->dfBBOX[j];
      return 0;
    }
};

class CBBOX{
public:
  double left,bottom,right,top;
  CBBOX& operator= (double f[4]){
    left=f[0];
    bottom=f[1];
    right=f[2];
    top=f[3];
    return *this;
  }      
};


class CPoint{
public:
  double x,y;
  CPoint& operator= (double f[2]){
    x=f[0];
    y=f[1];
    return *this;
  }      
};


void CoordinatesXYtoScreenXY(double &x,double &y,CGeoParams *geoParam);
void CoordinatesXYtoScreenXY(CPoint &p,CGeoParams *geoParam);
void CoordinatesXYtoScreenXY(CBBOX &b,CGeoParams *geoParam);
#endif

