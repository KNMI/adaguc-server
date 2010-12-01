#ifndef CBaseDataWriterInterface_H
#define CBaseDataWriterInterface_H
#include "CServerParams.h"
#include "CDataSource.h"
#include "CImageWarper.h"
#include "CTypes.h"



class CBaseDataWriterInterface{
  public:
    virtual ~CBaseDataWriterInterface(){}
    CBaseDataWriterInterface(){}
    virtual int init(CServerParams *srvParam,CDataSource *dataSource,int NrOfBands) = 0;
    virtual int addData(std::vector <CDataSource*> &dataSources) = 0;
    virtual int end() = 0;
};
#endif
