#ifndef CNetCDFDataWriter_H
#define CNetCDFDataWriter_H

#include "Definitions.h"
#include "CStopWatch.h"
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CIBaseDataWriterInterface.h"
#include "CDebugger.h"

class CNetCDFDataWriter: public CBaseDataWriterInterface{
private:
  CT::string JSONdata;
  DEF_ERRORFUNCTION();
  class Settings{
  public:
    size_t width;
    size_t height;
    void *data;
  };
  
  template <class T>
  static void drawFunction(int x,int y,T val, void *_settings){
    Settings*settings = (Settings*)_settings;
    if(x>=0&&y>=0&&x<(int)settings->width&&y<(int)settings->height){
      ((T*)settings->data)[x+y*settings->width]=val;
    }
//     for(int x=0;x<settings->width;x++){
//       ((T*)settings->data)[x+0*settings->width]=10;
//       ((T*)settings->data)[x+(settings->height-1)*settings->width]=10;
//     }
//     for(int y=0;y<settings->height;y++){
//       ((T*)settings->data)[0+y*settings->width]=10;
//       ((T*)settings->data)[(settings->width-1)+y*settings->width]=10;
//     }
  };
  CDataSource * baseDataSource;
  CDFObject *destCDFObject;
  CT::string tempFileName;
  CServerParams *srvParam;
public:
  CNetCDFDataWriter();
  ~CNetCDFDataWriter();
  // Virtual functions
  int init(CServerParams *srvParam,CDataSource *dataSource, int nrOfBands);
  int addData(std::vector <CDataSource*> &dataSources);
  int writeFile(const char *fileName, int level);
  int end();
};

#endif

