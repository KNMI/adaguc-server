#ifndef CCONVERTADAGUCVECTOR_H
#define CCONVERTADAGUCVECTOR_H
#include "CDataSource.h"
class CConvertADAGUCVector{
  private:
  DEF_ERRORFUNCTION();
  public:
  static int convertADAGUCVectorHeader(CDFObject *cdfObject);
  static int convertADAGUCVectorData(CDataSource *dataSource,int mode);
};
#endif
