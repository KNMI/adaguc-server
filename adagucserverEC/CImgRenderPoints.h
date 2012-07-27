#ifndef CIMGRENDERPOINTS_H
#define CIMGRENDERPOINTS_H
#include "CImageWarperRenderInterface.h"
class CImgRenderPoints:public CImageWarperRenderInterface{
private:
  DEF_ERRORFUNCTION();
public:
  void render(CImageWarper*, CDataSource*, CDrawImage*);
  int set(const char*);
  
};
#endif
