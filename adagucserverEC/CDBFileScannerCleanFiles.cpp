#include "CDBFileScanner.h"
#include "CDBFactory.h"
#include "CDebugger.h"
#include "CReporter.h"
#include "adagucserver.h"
#include "CNetCDFDataWriter.h"
#include <set>

void CDBFileScanner::_removeFileFromTables(CT::string fileNamestr, CDataSource *dataSource) {
  CDBAdapter *dbAdapter = CDBFactory::getDBAdapter(dataSource->srvParams->cfg);
  for (size_t i = 0; i < dataSource->requiredDims.size(); i++) {
    CT::string tableName;
    CT::string colName = dataSource->requiredDims[i]->netCDFDimName;
    try {
      tableName =
          dbAdapter->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), colName.c_str(), dataSource);
    } catch (int e) {
      CDBWarning("Unable to create tableName from '%s' '%s' '%s'", dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), colName.c_str());
    }
    CDBDebug("DB: Removing from table %s and dimension %s file %s", tableName.c_str(), colName.c_str(), fileNamestr.c_str());
    dbAdapter->removeFile(tableName.c_str(), fileNamestr.c_str());
  }
}

int CDBFileScanner::cleanFiles(CDataSource *dataSource, int) {
  if (!dataSource->cfgLayer->FilePath[0]->attr.retentiontype.equals("filetimedate") || dataSource->cfgLayer->FilePath[0]->attr.retentionperiod.empty()) {
    return 0;
  }
  if (!(dataSource->cfg->Settings.size() == 1 && dataSource->cfg->Settings[0]->attr.enablecleanupsystem.equals("true"))) {
    CDBWarning("Layer wants to autocleanup, but attribute enablecleanupsystem in Settings is not set to true");
    return 1;
  }
  CT::string retentiontype = dataSource->cfgLayer->FilePath[0]->attr.retentiontype;
  CT::string retentionperiod = dataSource->cfgLayer->FilePath[0]->attr.retentionperiod;
  CDBDebug("Start Cleanfiles with retentiontype [%s] and retentionperiod [%s]", retentiontype.c_str(), retentionperiod.c_str());

  CDBAdapter *dbAdapter = CDBFactory::getDBAdapter(dataSource->srvParams->cfg);
  if (dataSource->cfgLayer->Dimension.size() == 0) {
    if (CAutoConfigure::autoConfigureDimensions(dataSource) != 0) {
      CREPORT_ERROR_NODOC("Unable to configure dimensions automatically", CReportMessage::Categories::GENERAL);
      return 1;
    }
  }
  try {
    CRequest::fillDimValuesForDataSource(dataSource, dataSource->srvParams);
  } catch (ServiceExceptionCode e) {
    CDBError("Exception in setDimValuesForDataSource");
    return 1;
  }

  CT::string tableNameForTimeDimension;
  CT::string colName = dataSource->requiredDims[0]->netCDFDimName;

  try {
    tableNameForTimeDimension =
        dbAdapter->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), colName.c_str(), dataSource);
  } catch (int e) {
    CDBError("Unable to create tableName from '%s' '%s' '%s'", dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), colName.c_str());
    return 1;
  }

  CTime ctime;
  ctime.init("seconds since 1970", "none");

  CT::string currentTime = CTime::currentDateTime();
  CTime::Date date = ctime.freeDateStringToDate(currentTime.c_str());

  CDBDebug("currentDate\t\t\t%s", ctime.dateToISOString(date).c_str());

  CT::string dateMinusRetentionPeriod = ctime.dateToISOString(ctime.subtractPeriodFromDate(date, retentionperiod.c_str()));

  CDBDebug("dateMinusRetentionPeriod\t%s", dateMinusRetentionPeriod.c_str());

  CDBStore::Store *store = dbAdapter->getBetween("0000-01-01T00:00:00Z", dateMinusRetentionPeriod.c_str(), colName.c_str(), tableNameForTimeDimension.c_str(), 10000);

  if (store != NULL && store->getSize() > 0) {
    for (size_t j = 0; j < store->getSize(); j++) {
      CT::string fileNamestr = store->getRecord(j)->get(0)->c_str();
      _removeFileFromTables(fileNamestr, dataSource);
      int status = remove(fileNamestr.c_str());
      if (status != 0) {
        CDBError("Unable to remove file from FS: [%s]", fileNamestr.c_str());
      }
    }
  }
  delete store;
  return 0;
}
