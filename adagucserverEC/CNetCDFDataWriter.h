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
  };
  CDataSource * baseDataSource;
  CDFObject *destCDFObject;
  CT::string tempFileName;
  CServerParams *srvParam;
  CDF::Dimension *projectionDimX,*projectionDimY;//Shorthand pointers to cdfdatamodel (do never delete!)
  CDF::Variable *projectionVarX,*projectionVarY;//Shorthand pointers to cdfdatamodel (do never delete!)
  void createProjectionVariables(CDFObject *cdfObject,int width,int height,double *bbox);
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

