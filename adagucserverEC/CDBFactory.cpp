#include "CDBFactory.h"
#include "CDBAdapter.h"

CDBAdapter::~CDBAdapter(){
}

const char *CDBFactory::className="CDBFactory";

CDBAdapter *CDBFactory::staticCDBAdapter = NULL;

CDBAdapter *CDBFactory::getDBAdapter(CServerConfig::XMLE_Configuration *cfg){
  if(staticCDBAdapter == NULL){
    CDBDebug("CREATE");
    staticCDBAdapter = new CDBAdapterPostgreSQL();
    staticCDBAdapter->setConfig(cfg);
  }
  return staticCDBAdapter;
}

void CDBFactory::clear(){
  CDBDebug("CLEAR");
  delete staticCDBAdapter;
  staticCDBAdapter = NULL;
}