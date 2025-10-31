#include "CDBFileScanner.h"
#include "CDBFactory.h"
#include "CDebugger.h"
#include "CReporter.h"
#include "adagucserver.h"
#include "CNetCDFDataWriter.h"
#include <set>
std::set<std::string> CDBFileScanner::filesDeletedFromFS;

void CDBFileScanner::_removeFileFromTables(CT::string fileNamestr, CDataSource *dataSource) {
  CDBAdapterPostgreSQL *dbAdapter = CDBFactory::getDBAdapter(dataSource->srvParams->cfg);
  auto tableList = dbAdapter->getTableNames(dataSource);
  for (auto tableName : tableList) {
    CDBDebug("DB: Removing from table %s and file %s", tableName.c_str(), fileNamestr.c_str());
    dbAdapter->removeFile(tableName.c_str(), fileNamestr.c_str());
  }
}

std::pair<int, std::set<std::string>> CDBFileScanner::cleanFiles(CDataSource *dataSource, int) {
  if (dataSource->cfgLayer->FilePath[0]->attr.retentiontype.empty() || dataSource->cfgLayer->FilePath[0]->attr.retentionperiod.empty()) {
    return std::make_pair(0, filesDeletedFromFS);
  }

  if (!dataSource->cfgLayer->FilePath[0]->attr.retentiontype.equals(CDBFILESCANNER_RETENTIONTYPE_DATATIME)) {
    CDBDebug("retentiontype is not set to \"%s\" but  to \"%s\", ignoring", CDBFILESCANNER_RETENTIONTYPE_DATATIME, dataSource->cfgLayer->FilePath[0]->attr.retentiontype.c_str());
    return std::make_pair(0, filesDeletedFromFS);
  }
  CT::string enableCleanupSystem = dataSource->cfg->Settings.size() == 1 ? dataSource->cfg->Settings[0]->attr.enablecleanupsystem : "false";
  bool enableCleanupIsTrue = enableCleanupSystem.equals("true");
  bool enableCleanupIsInform = enableCleanupSystem.equals("dryrun");
  int cleanupSystemLimit = dataSource->cfg->Settings.size() == 1 && !dataSource->cfg->Settings[0]->attr.cleanupsystemlimit.empty() ? dataSource->cfg->Settings[0]->attr.cleanupsystemlimit.toInt()
                                                                                                                                   : CDBFILESCANNER_CLEANUP_DEFAULT_LIMIT;
  if (!enableCleanupIsTrue && !enableCleanupIsInform) {
    CDBWarning("Layer wants to autocleanup, but attribute enablecleanupsystem in Settings is not set to true or dryrun but to %s", enableCleanupSystem.c_str());
    return std::make_pair(1, filesDeletedFromFS);
  }

  CT::string retentiontype = dataSource->cfgLayer->FilePath[0]->attr.retentiontype;
  CT::string retentionperiod = dataSource->cfgLayer->FilePath[0]->attr.retentionperiod;
  CDBDebug("Start Cleanfiles with retentiontype [%s] and retentionperiod [%s], limit %d", retentiontype.c_str(), retentionperiod.c_str(), cleanupSystemLimit);

  if (enableCleanupIsInform) {
    CDBDebug("Note that mode is set to dryrun only, no actual deleting");
  }
  CDBAdapterPostgreSQL *dbAdapter = CDBFactory::getDBAdapter(dataSource->srvParams->cfg);
  if (dataSource->cfgLayer->Dimension.size() == 0) {
    if (CAutoConfigure::autoConfigureDimensions(dataSource) != 0) {
      CREPORT_ERROR_NODOC("Unable to configure dimensions automatically", CReportMessage::Categories::GENERAL);
      return std::make_pair(1, filesDeletedFromFS);
    }
  }
  try {
    CRequest::fillDimValuesForDataSource(dataSource, dataSource->srvParams);
  } catch (ServiceExceptionCode e) {
    return std::make_pair(1, filesDeletedFromFS);
  }

  CT::string tableNameForTimeDimension;

  CT::string colName = "time";

  // If this datasource has a reference_time, give preference to that.
  for (size_t j = 0; j < dataSource->requiredDims.size(); j += 1) {
    if (dataSource->requiredDims[j]->netCDFDimName.equals("forecast_reference_time")) {
      colName = "forecast_reference_time";
    }
  }

  CDBDebug("Checking dimension %s", colName.c_str());

  try {
    tableNameForTimeDimension =
        dbAdapter->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), colName.c_str(), dataSource);
  } catch (int e) {
    CDBError("Unable to create tableName from '%s' '%s' '%s'", dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), colName.c_str());
    return std::make_pair(1, filesDeletedFromFS);
  }

  CTime *ctime = CTime::GetCTimeEpochInstance();

  CT::string currentTime = CTime::currentDateTime();
  CTime::Date date = ctime->freeDateStringToDate(currentTime.c_str());

  // CDBDebug("currentDate\t\t\t%s", ctime->dateToISOString(date).c_str());

  CT::string dateMinusRetentionPeriod = ctime->dateToISOString(ctime->subtractPeriodFromDate(date, retentionperiod.c_str()));

  // CDBDebug("dateMinusRetentionPeriod\t%s", dateMinusRetentionPeriod.c_str());

  CDBStore::Store *store = dbAdapter->getBetween("0000-01-01T00:00:00Z", dateMinusRetentionPeriod.c_str(), colName.c_str(), tableNameForTimeDimension.c_str(), cleanupSystemLimit);
  if (store != NULL && store->getSize() > 0) {
    CDBDebug("Found (at least) %d files which are too old.", store->getSize());
    for (size_t j = 0; j < store->getSize(); j++) {
      std::string fileNamestr = store->getRecord(j)->get(0)->c_str();
      if (enableCleanupIsInform) {
        CDBDebug("[INFO] Would clean: [%s]", fileNamestr.c_str());
      }
      if (enableCleanupIsTrue) {
        _removeFileFromTables(fileNamestr.c_str(), dataSource);
        /* Don't delete files which where already deleted */
        if (filesDeletedFromFS.find(fileNamestr) == filesDeletedFromFS.end()) {
          int status = remove(fileNamestr.c_str());
          if (status != 0) {
            CDBError("Unable to remove file from FS: [%s]", fileNamestr.c_str());
          } else {
            filesDeletedFromFS.insert(fileNamestr);
            // File is not present anymore, also remove it from the dirreaders
            CCachedDirReader::removeFileFromCachedList(fileNamestr);
          }
        }
      }
    }
  } else {
    CDBDebug("Nothing to clean");
  }
  delete store;
  CDBDebug("Succesfully finished cleaning files");
  return std::make_pair(0, filesDeletedFromFS);
}
