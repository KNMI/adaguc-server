#ifndef CGeoParams_H
#define CGeoParams_H
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
#endif
