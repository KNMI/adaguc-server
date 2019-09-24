#include "CDBFileScanner.h"
#include "CDBFactory.h"
#include "CDebugger.h"
#include "CReporter.h"
#include "adagucserver.h"
#include "CNetCDFDataWriter.h"
#include <set>

int CDBFileScanner::cleanFiles(CDataSource *dataSource, int scanFlags) {
  CDBDebug("Cleanfiles");
  CDBAdapter * dbAdapter = CDBFactory::getDBAdapter(dataSource->srvParams->cfg);
  if(dataSource->cfgLayer->Dimension.size()==0){
    if(CAutoConfigure::autoConfigureDimensions(dataSource)!=0){
      CREPORT_ERROR_NODOC("Unable to configure dimensions automatically", CReportMessage::Categories::GENERAL);
      return 1;
    }
  }
  try{
    CRequest::fillDimValuesForDataSource(dataSource,dataSource->srvParams);
  }catch(ServiceExceptionCode e){
    CDBError("Exception in setDimValuesForDataSource");
    return 1;
  }
  CDBStore::Store *store = dbAdapter->getFilesAndIndicesForDimensions(dataSource,10);
  if(store!=NULL && store->getSize() > 0) {
    for (size_t c = 0;c < store->getColumnModel()->getSize(); c++){
      CDBDebug("%d %s", c, store->getColumnModel()->getName(c));
    }
    for (size_t j = 0;j< store->getSize(); j++) {
      CT::string fileNamestr = store->getRecord(j)->get(0)->c_str();
      CDBDebug("fileName from DB: %s",fileNamestr.c_str());
    }
  }
  delete store;
  return 0;
}

