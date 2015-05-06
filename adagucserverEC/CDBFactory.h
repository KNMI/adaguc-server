#ifndef CDBFACTORY_H
#define CDBFACTORY_H

#include "CDBAdapter.h"
#include "CDBAdapterPostgreSQL.h"

class CDBFactory{
private:
  private:
    DEF_ERRORFUNCTION();
public:
  static CDBAdapter *staticCDBAdapter;

  static CDBAdapter *getDBAdapter(CServerConfig::XMLE_Configuration *cfg);
  static void clear();
};

#endif