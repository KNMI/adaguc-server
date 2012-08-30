#ifndef CCONVERTADAGUCPOINT_H
#define CCONVERTADAGUCPOINT_H
#include "CDataSource.h"
class CConvertADAGUCPoint{
  private:
  DEF_ERRORFUNCTION();
  public:
  static int convertADAGUCPointHeader(CDFObject *cdfObject);
  static int convertADAGUCPointData(CDataSource *dataSource,int mode);
};
#endif
