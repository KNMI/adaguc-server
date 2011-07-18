#ifndef CImageWarperRenderInterface_H  
#define CImageWarperRenderInterface_H
#include "CServerParams.h"
#include "CDataReader.h"
#include "CDrawImage.h"
#include "CImageWarper.h"
#include <stdlib.h>
#include "CDebugger.h"


class CImageWarperRenderInterface{
  public: 
    virtual ~CImageWarperRenderInterface(){};
    virtual void render(CImageWarper *warper,CDataSource *sourceImage,CDrawImage *drawImage) = 0;
    virtual int set(const char *settings) = 0;
};
#endif

