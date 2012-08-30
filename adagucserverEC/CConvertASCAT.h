#ifndef CCONVERTASCAT_H
#define CCONVERTASCAT_H
#include "CDataSource.h"
class CConvertASCAT{
  private:
  DEF_ERRORFUNCTION();
  public:
  static int convertASCATHeader(CDFObject *cdfObject);
  static int convertASCATData(CDataSource *dataSource,int mode);
};
#endif
