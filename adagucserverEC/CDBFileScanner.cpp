/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include "CDBFileScanner.h"
#include "CDBFactory.h"
#include "CDebugger.h"
#include "CReporter.h"
#include "adagucserver.h"
#include "CNetCDFDataWriter.h"
#include "CCreateTiles.h"
#include <set>
#include "utils/LayerMetadataStore.h"
#include "utils/ConfigurationUtils.h"
const char *CDBFileScanner::className = "CDBFileScanner";
std::vector<CT::string> CDBFileScanner::tableNamesDone;
// #define CDBFILESCANNER_DEBUG
#define ISO8601TIME_LEN 32

#define CDBFILESCANNER_TILECREATIONFAILED -100

std::vector<std::string> CDBFileScanner::filesToDeleteFromDB;
bool CDBFileScanner::isTableAlreadyScanned(CT::string *tableName) {
  for (size_t t = 0; t < tableNamesDone.size(); t++) {
    if (tableNamesDone[t].equals(tableName->c_str())) {
      return true;
    }
  }
  return false;
}
void CDBFileScanner::markTableDirty(CT::string *tableName) {
  CDBDebug("Marking table dirty: %d %s", tableNamesDone.size(), tableName->c_str());
  for (size_t t = 0; t < tableNamesDone.size(); t++) {
    if (tableNamesDone[t].equals(tableName->c_str())) {
      tableNamesDone.erase(tableNamesDone.begin() + t);
      CDBDebug("Table marked dirty %d %s", tableNamesDone.size(), tableName->c_str());
      return;
    }
  }
}

/**
 *
 * @return Positive on error, zero on succes, negative on skip.
 */
int CDBFileScanner::createDBUpdateTables(CDataSource *dataSource, int &removeNonExistingFiles, std::vector<std::string> *fileList, bool recreateTables) {
  bool verbose = dataSource->srvParams->verbose;
  if (fileList->size() == 0) {
    CDBDebug("createDBUpdateTables: no files");
    return 0;
  }
#ifdef CDBFILESCANNER_DEBUG
  CDBDebug("createDBUpdateTables");
#endif
  int status = 0;
  CT::string query;
  dataSource->headerFileName = (*fileList)[0].c_str();

  CDBAdapter *dbAdapter = CDBFactory::getDBAdapter(dataSource->srvParams->cfg);

  CDFObject *cdfObject = NULL;
  try {
    cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObject(dataSource, dataSource->headerFileName.c_str());
    if (cdfObject == NULL) throw __LINE__;
  } catch (int e) {
    CDBError("Unable to get CDFObject for file %s", dataSource->headerFileName.c_str());
    return 1;
  }

  // Check if variable is in this file:

  if (cdfObject->getVariableNE(dataSource->getDataObject(0)->variableName.c_str()) == NULL) {
    CDBError("Variable %s does not exist in %s ", dataSource->getDataObject(0)->variableName.c_str(), dataSource->headerFileName.c_str());
    return 1;
  }

  if (dataSource->cfgLayer->Dimension.size() == 0) {

    if (CAutoConfigure::autoConfigureDimensions(dataSource) != 0) {
      CREPORT_ERROR_NODOC(CT::string("Unable to configure dimensions automatically"), CReportMessage::Categories::GENERAL);
      return 1;
    }
  }

#ifdef CDBFILESCANNER_DEBUG
  CDBDebug("dataSource->dimsAreAutoConfigured %d", dataSource->dimsAreAutoConfigured);
  CDBDebug("fileList->size() = %d", fileList->size());
#endif

  if (dataSource->cfgLayer->Dimension.size() == 0) {
    CREPORT_ERROR_NODOC(CT::string("Still No dims"), CReportMessage::Categories::GENERAL);
    return 1;
  }

#ifdef CDBFILESCANNER_DEBUG
  CDBDebug("dataSource->cfgLayer->Dimension.size() %d", dataSource->cfgLayer->Dimension.size());
#endif
  // Check and create all tables...
  for (size_t d = 0; d < dataSource->cfgLayer->Dimension.size(); d++) {

    /* A dimension where the default value is set to filetimedate is not a required dim and should not be queried from the db */
    if (dataSource->cfgLayer->Dimension[d]->attr.defaultV.equals("filetimedate")) {
      dataSource->cfgLayer->Dimension[d]->value.copy("0");
      dataSource->cfgLayer->Dimension[d]->attr.name.copy("none");
      dataSource->cfgLayer->Dimension[d]->attr.units.copy("none");
    }

    CT::string dimName = "";
    if (dataSource->cfgLayer->Dimension[d]->attr.name.empty() == false) {
      dimName = dataSource->cfgLayer->Dimension[d]->attr.name.c_str();
    }

    if (verbose) {
      CDBDebug("Checking dim [%s]", dimName.c_str());
    }

    CDataReader::DimensionType dtype = CDataReader::getDimensionType(cdfObject, dimName.c_str());
    if (dtype == CDataReader::dtype_none) {
      CDBWarning("dtype_none %d for  %s", dtype, dimName.c_str());
      if (CDataReader::addBlankDimVariable(cdfObject, dimName.c_str()) != NULL) {
        dtype = CDataReader::dtype_normal;
      }
    }

    bool isTimeDim = false;
    if (dtype == CDataReader::dtype_time || dtype == CDataReader::dtype_reference_time) {
      isTimeDim = true;
    }

    // Create database tableNames
    CT::string tableName;
    try {
      tableName =
          dbAdapter->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str(), dataSource);
    } catch (int e) {
      CDBError("Unable to create tableName from '%s' '%s' '%s'", dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str());
      return 1;
    }
#ifdef CDBFILESCANNER_DEBUG
    CDBDebug("Tablename = %s", tableName.c_str());
#endif

    int TABLETYPE_TIMESTAMP = 1;
    int TABLETYPE_INT = 2;
    int TABLETYPE_REAL = 3;
    int TABLETYPE_STRING = 4;

    int tableType = 0;

    // Check whether we already did this table in this scan
    bool skip = isTableAlreadyScanned(&tableName);
    if (skip == false) {

#ifdef CDBFILESCANNER_DEBUG
      CDBDebug("CreateDBUpdateTables: Updating dimension '%s' with table '%s' %d", dimName.c_str(), tableName.c_str(), isTimeDim);
#endif

      // Drop table if set
      if (recreateTables) {
        CDBDebug("Recreating table: Now dropping table %s", tableName.c_str());
        status = dbAdapter->dropTable(tableName.c_str());
      }

      // Create column names

      if (isTimeDim == true) {
        tableType = TABLETYPE_TIMESTAMP;
      } else {
        try {
          bool dimensionlessmode = false;

          if (dimName.equals("none")) {
#ifdef CDBFILESCANNER_DEBUG
            CDBDebug("dimensionlessmode");
#endif
            dimensionlessmode = true;
            status = dbAdapter->createDimTableInt(dimName.c_str(), tableName.c_str());
            tableType = TABLETYPE_INT;
          }

          CDF::Variable *dimVar = NULL;
          if (dimensionlessmode == false) {
            dimVar = cdfObject->getVariableNE(dimName.c_str());
            if (dimVar == NULL) {
              CREPORT_ERROR_NODOC(CT::string("Dimension ") + dimName + CT::string(" not found."), CReportMessage::Categories::GENERAL);
              throw(__LINE__);
            }
          }

          bool hasStatusFlag = false;
          if (dimensionlessmode == false) {
            std::vector<CDataSource::StatusFlag *> statusFlagList;
            CDataSource::readStatusFlags(dimVar, &statusFlagList);
            if (statusFlagList.size() > 0) hasStatusFlag = true;
            for (size_t i = 0; i < statusFlagList.size(); i++) delete statusFlagList[i];
            statusFlagList.clear();
            if (hasStatusFlag) {
              tableType = TABLETYPE_STRING;
              status = dbAdapter->createDimTableString(dimName.c_str(), tableName.c_str());
            } else {
              switch (dimVar->getType()) {
              case CDF_FLOAT:
              case CDF_DOUBLE:
                tableType = TABLETYPE_REAL;
                break;
              case CDF_STRING:
                tableType = TABLETYPE_STRING;
                break;
              default:
                tableType = TABLETYPE_INT;
                break;
              }
            }
          }
        } catch (int e) {
          CDBError("Exception defining table structure at line %d", e);
          return 1;
        }
      }
      if (tableType == 0) {
        CDBError("Unknown tabletype");
        return 1;
      }

      status = 1;
      if (tableType == TABLETYPE_TIMESTAMP) status = dbAdapter->createDimTableTimeStamp(dimName.c_str(), tableName.c_str());
      if (tableType == TABLETYPE_INT) status = dbAdapter->createDimTableInt(dimName.c_str(), tableName.c_str());
      if (tableType == TABLETYPE_REAL) status = dbAdapter->createDimTableReal(dimName.c_str(), tableName.c_str());
      if (tableType == TABLETYPE_STRING) status = dbAdapter->createDimTableString(dimName.c_str(), tableName.c_str());

      // if(status == 0){CDBDebug("OK: Table is available");}
      if (status == 1) {
        CDBError("FAIL: Table %s could not be created", tableName.c_str());
        return 1;
      }
      if (status == 2) {
        removeNonExistingFiles = 0;
        // removeExisting files can be set back to zero, because there are no files to remove (table is created)
        // note the int &removeNonExistingFiles as parameter of this function!
        //(Setting the value will have effect outside this function)
        // CDBDebug("OK: Table %s created, (check for unavailable files is off);",tableName);
        // if( addIndexToTable(DB,tableName.c_str(),dimName.c_str()) != 0)return 1;
      }
      // TODO set removeNonExistingFiles =0 when no records are in table

      if (removeNonExistingFiles == 1) {
        // The temporary table should always be dropped before filling.
        // We will do a complete new update, so store everything in an new table
        // Later we will rename this table
        //         CT::string tableName_temp(&tableName);
        //         if(removeNonExistingFiles==1){
        //           tableName_temp.concat("_temp");
        //         }
        // CDBDebug("Making empty temporary table %s ... ",tableName_temp.c_str());
        // CDBDebug("Check table %s ...\t",tableName.c_str());
        /*
                if(status==0){
                  //Table already exists....
                  CDBError("*** WARNING: Temporary table %s already exists. Is another process updating the database?
           ***",tableName_temp.c_str());


                  CDBDebug("*** DROPPING TEMPORARY TABLE: %s",query.c_str());
                  if(dbAdapter->dropTable(tableName_temp.c_str())!=0){
                    CDBError("Dropping table %s failed",tableName_temp.c_str());
                    return 1;
                  }

                  CDBDebug("Check table %s ... ",tableName_temp.c_str());
                  if(tableType == TABLETYPE_TIMESTAMP)status =
           dbAdapter->createDimTableTimeStamp(dimName.c_str(),tableName_temp.c_str()); if(tableType == TABLETYPE_INT
           )status = dbAdapter->createDimTableInt      (dimName.c_str(),tableName_temp.c_str()); if(tableType ==
           TABLETYPE_REAL     )status = dbAdapter->createDimTableReal     (dimName.c_str(),tableName_temp.c_str());
                  if(tableType == TABLETYPE_STRING   )status = dbAdapter->createDimTableString
           (dimName.c_str(),tableName_temp.c_str());

                  if(status == 0){CDBDebug("OK: Table is available");}
                  if(status == 1){CDBError("\nFAIL: Table %s could not be created",tableName_temp.c_str()); return 1;  }
                  if(status == 2){CDBDebug("OK: Table %s created",tableName_temp.c_str());
                    //Create a index on these files:
                    //if(addIndexToTable(DB,tableName_temp.c_str(),dimName.c_str())!= 0)return 1;
                  }
                }

                if(status == 0 || status == 1){CDBError("\nFAIL: Table %s could not be created",tableName_temp.c_str());
           return 1;  } if(status == 2){
                  //OK, Table did not exist, is created.
                  //Create a index on these files:
                  //if(addIndexToTable(DB,tableName_temp.c_str(),dimName.c_str()) != 0)return 1;
                }*/
      }
    }
  }

  return 0;
}

int CDBFileScanner::DBLoopFiles(CDataSource *dataSource, int removeNonExistingFiles, std::vector<std::string> *fileList, int scanFlags) {
  //  CDBDebug("DBLoopFiles");
  bool verbose = dataSource->srvParams->verbose;
  CT::string query;
  CDFObject *cdfObject = NULL;
  int status = 0;

  CDBAdapter *dbAdapter = CDBFactory::getDBAdapter(dataSource->srvParams->cfg);
  try {
    // Loop dimensions and files
    // CDBDebug("Checking files that are already in the database...");
    // char ISOTime[ISO8601TIME_LEN+1];
    CT::string isoString;
    size_t numberOfFilesAddedFromDB = 0;

    // Setup variables like tableNames and timedims for each dimension
    size_t numDims = dataSource->cfgLayer->Dimension.size();

    bool isTimeDim[numDims];
    CT::string dimNames[numDims];
    // CT::string tableColumns[numDims];
    CT::string tableNames[numDims];
    // CT::string tableNames_temp[numDims];
    bool skipDim[numDims];
    CT::string queryString;
    // CT::string VALUES;
    // CADAGUC_time *ADTime  = NULL;
    CTime *adagucTime;

    CDFObject *cdfObjectOfFirstFile = NULL;
    try {
      cdfObjectOfFirstFile = CDFObjectStore::getCDFObjectStore()->getCDFObject(dataSource, (*fileList)[0].c_str());
    } catch (int e) {
      CDBError("Unable to get CDFObject for file %s", (*fileList)[0].c_str());
      return 1;
    }

    for (size_t d = 0; d < dataSource->cfgLayer->Dimension.size(); d++) {

      dimNames[d].copy(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());

      isTimeDim[d] = false;
      skipDim[d] = false;

      /* A dimension where the default value is set to filetimedate is not a required dim and should not be queried from the db */
      if (dataSource->cfgLayer->Dimension[d]->attr.defaultV.equals("filetimedate")) {
        dataSource->cfgLayer->Dimension[d]->value.copy("0");
        dataSource->cfgLayer->Dimension[d]->attr.name.copy("none");
        dataSource->cfgLayer->Dimension[d]->attr.units.copy("none");
      }

      CDataReader::DimensionType dtype = CDataReader::getDimensionType(cdfObjectOfFirstFile, dimNames[d].c_str());

      if (dtype == CDataReader::dtype_none) {
        CDBWarning("dtype_none for %s", dtype, dimNames[d].c_str());
      }
      dimNames[d].toLowerCaseSelf();
      if (dtype == CDataReader::dtype_time || dtype == CDataReader::dtype_reference_time) {
        isTimeDim[d] = true;
      }
      if (verbose) {
        CDBDebug("Found dimension %d with name %s of type %d, istimedim: [%d]", d, dimNames[d].c_str(), dtype, isTimeDim[d]);
      }

      try {
        tableNames[d] =
            dbAdapter->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dimNames[d].c_str(), dataSource);
      } catch (int e) {
        CDBError("Unable to create tableName from '%s' '%s' '%s'", dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dimNames[d].c_str());
        return 1;
      }
#ifdef CDBFILESCANNER_DEBUG
      CDBDebug("Found table name %s", tableNames[d].c_str());
#endif
      //       //Create temporary tableName
      //       tableNames_temp[d].copy(&(tableNames[d]));
      //       if(removeNonExistingFiles==1){
      //         tableNames_temp[d].concat("_temp");
      //       }
      //
      skipDim[d] = isTableAlreadyScanned(&tableNames[d]);
      if (skipDim[d]) {
        if (dataSource->srvParams->verbose) {
          CDBDebug("Already scanned dimension '%s' with table '%s'.", dimNames[d].c_str(), tableNames[d].c_str());
        }
      } else {
        if (dataSource->cfgLayer->TileSettings.size() == 0) {
          if (verbose) {
            CDBDebug("Marking table done for dim '%s' with table '%s'.", dimNames[d].c_str(), tableNames[d].c_str());
          }
          tableNamesDone.push_back(tableNames[d]);
        }
      }
    }

    for (size_t j = 0; j < fileList->size(); j++) {
// Loop through all configured dimensions.
#ifdef CDBFILESCANNER_DEBUG
      CDBDebug("Loop through all configured dimensions.");
#endif

      CT::string fileDate = CDirReader::getFileDate((*fileList)[j].c_str());
      CT::string fileDateToCompareWith = fileDate;

      if (scanFlags & CDBFILESCANNER_RESCAN) {
        fileDateToCompareWith = "";
        CDBDebug("--rescan set: fileDate is ignored.");
      }

      CT::string dimensionTextList = "none";
      if (dataSource->cfgLayer->Dimension.size() > 0) {
        dimensionTextList.print("(%s", dataSource->cfgLayer->Dimension[0]->attr.name.c_str());
        for (size_t d = 1; d < dataSource->cfgLayer->Dimension.size(); d++) {
          dimensionTextList.printconcat(", %s", dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
        }
        dimensionTextList.concat(")");
      }

      /* If there is only one dimension for the list of files, and if this dimension is done, skip */
      if (dataSource->cfgLayer->Dimension.size() == 1) {
        if (skipDim[0] == true) {
          if (verbose) {
            CDBDebug("Assuming [%s] done", dataSource->cfgLayer->Dimension[0]->attr.name.c_str());
          }
          break;
        }
      }
      size_t numberOfFilesAddedToDbStore = 0;
      for (size_t d = 0; d < dataSource->cfgLayer->Dimension.size(); d++) {
        if (skipDim[d] == true) {
#ifdef CDBFILESCANNER_DEBUG
          CDBDebug("Assuming %d/%d [%s] done", j, d, dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
#endif
          continue;
        }
        {
          numberOfFilesAddedToDbStore += 1;
          numberOfFilesAddedFromDB = 0;
          int fileExistsInDB = 0;

// Delete files with non-matching creation date
#ifdef CDBFILESCANNER_DEBUG
          CDBDebug("removeFilesWithChangedCreationDate [%s] [%s] [%s]", tableNames[d].c_str(), (*fileList)[j].c_str(), fileDateToCompareWith.c_str());
#endif
          try {
            dbAdapter->removeFilesWithChangedCreationDate(tableNames[d].c_str(), (*fileList)[j].c_str(), fileDateToCompareWith.c_str());
          } catch (int e) {
            CDBWarning("Unable to remove files from db %d", e);
          }

// Check if file is already there
#ifdef CDBFILESCANNER_DEBUG
          CDBDebug("checkIfFileIsInTable");
#endif
          status = dbAdapter->checkIfFileIsInTable(tableNames[d].c_str(), (*fileList)[j].c_str());
          if (status == 0) {
            if (dataSource->srvParams->verbose) {
              CDBDebug("Already scanned file %s", (*fileList)[j].c_str());
            }

            // The file is there!
            fileExistsInDB = 1;
          } else {
            // The file is not there. If isAutoResourceEnabled and there is no file, force cleaning of
            // autoConfigureDimensions table.
            if (removeNonExistingFiles == 1) {
              if (fileList->size() == 1) {
                CT::string layerTableId;
                try {

                  layerTableId =
                      CDBFactory::getDBAdapter(dataSource->srvParams->cfg)
                          ->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), NULL, dataSource);

                } catch (int e) {
                  CDBError("Unable to get layerTableId for autoconfigure_dimensions");
                  return 1;
                }

                CDBDebug("Removing autoConfigureDimensions [%s_%s] and [%s]", tableNames[d].c_str(), dataSource->getLayerName(), layerTableId.c_str());

                CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->removeDimensionInfoForLayerTableAndLayerName(layerTableId.c_str(), NULL);
                CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->removeDimensionInfoForLayerTableAndLayerName(tableNames[d].c_str(), dataSource->getLayerName());

                if (dataSource->dimsAreAutoConfigured) {
                  XMLE_DELOBJ(dataSource->cfgLayer->Dimension);
                  dataSource->cfgLayer->Dimension.clear();
                }

                status = createDBUpdateTables(dataSource, removeNonExistingFiles, fileList, false);
                if (status > 0) {
                  CDBError("Exception at createDBUpdateTables");
                  throw(__LINE__);
                }
              }
            }
          }

          // The file metadata does not already reside in the db.
          // Therefore we need to read information from it
          if (fileExistsInDB == 0) {
#ifdef CDBFILESCANNER_DEBUG
            CDBDebug("fileExistsInDB == 0");
#endif
            try {

              if (d == 0) {
                if (verbose) {
                  CDBDebug("Adding: %zu/%zu %s\t %s", j, fileList->size(), dimensionTextList.c_str(), (*fileList)[j].c_str());
                }
              };
#ifdef CDBFILESCANNER_DEBUG
              CDBDebug("Creating new CDFObject");
#endif
              cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObject(dataSource, (*fileList)[j].c_str());
              if (cdfObject == NULL) {
                CDBError("cdfObject == NULL");
                throw(__LINE__);
              }

// Open the file
#ifdef CDBFILESCANNER_DEBUG
              CDBDebug("Opening file %s", (*fileList)[j].c_str());
#endif
              if (!verbose) {
                CDBDebug("Scan %s", (*fileList)[j].c_str());
              }
#ifdef CDBFILESCANNER_DEBUG
              CDBDebug("Looking for %s", dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
#endif
              // Check for the configured dimensions or scalar variables
              // 1 )Is this a scalar?
              CDF::Variable *dimVar = cdfObject->getVariableNE(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
              CDF::Dimension *dimDim = cdfObject->getDimensionNE(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());

              if (dataSource->cfgLayer->Dimension[d]->attr.name.equals("none")) {
#ifdef CDBFILESCANNER_DEBUG
                CDBDebug("Creating dummy dim none");
#endif
                dimVar = new CDF::Variable();
                dimVar->name = "none";
                cdfObject->addVariable(dimVar);
                dimDim = new CDF::Dimension();
                dimDim->name = dimVar->name;
                dimDim->setSize(1);
                cdfObject->addDimension(dimDim);
                dimVar->dimensionlinks.push_back(dimDim);
              }

              if (dimVar != NULL && dimDim == NULL) {
                // Check for scalar variable
                if (dimVar->dimensionlinks.size() == 0) {
#ifdef CDBFILESCANNER_DEBUG
                  CDBDebug("Found scalar variable %s with no dimension. Creating dim", dimVar->name.c_str());
#endif
                  dimDim = new CDF::Dimension();
                  dimDim->name = dimVar->name;
                  dimDim->setSize(1);
                  cdfObject->addDimension(dimDim);
                  dimVar->dimensionlinks.push_back(dimDim);
                }
                // Check if this variable has another dim attached
                if (dimVar->dimensionlinks.size() == 1) {
                  dimDim = dimVar->dimensionlinks[0];
#ifdef CDBFILESCANNER_DEBUG
                  CDBDebug("Using dimension %s for dimension variable %s", dimVar->dimensionlinks[0]->name.c_str(), dimVar->name.c_str());
#endif
                }
              }

              if ((dimDim == NULL || dimVar == NULL)) {
                CDBError("In file %s", (*fileList)[j].c_str());
                if (dimVar == NULL) {
                  CREPORT_ERROR_NODOC(CT::string("Variable ") + dataSource->cfgLayer->Variable[0]->value + CT::string(" for dimension ") + dataSource->cfgLayer->Dimension[d]->attr.name +
                                          CT::string(" not found"),
                                      CReportMessage::Categories::GENERAL);
                }
                if (dimDim == NULL) {
                  CREPORT_ERROR_NODOC(CT::string("For variable ") + dataSource->cfgLayer->Variable[0]->value + CT::string(" dimension ") + dataSource->cfgLayer->Dimension[d]->attr.name +
                                          CT::string(" not found"),
                                      CReportMessage::Categories::GENERAL);
                }

                throw(__LINE__);
              } else {
                bool hasStatusFlag = false;
                std::vector<CDataSource::StatusFlag *> statusFlagList;
                if (dimVar != NULL) {
                  CDF::Attribute *dimUnits = dimVar->getAttributeNE("units");
                  if (dimUnits == NULL) {
                    if (isTimeDim[d]) {
                      setStatusCode(HTTP_STATUSCODE_404_NOT_FOUND);
                      CREPORT_ERROR_NODOC(CT::string("No time units found for variable ") + dimVar->name, CReportMessage::Categories::GENERAL);
                      throw(__LINE__);
                    }
                    dimVar->setAttributeText("units", "1");
                    dimUnits = dimVar->getAttributeNE("units");
                  }

                  // Create adaguctime structure, when this is a time dimension.
                  if (isTimeDim[d]) {
                    adagucTime = CTime::GetCTimeInstance(dimVar);
                    if (adagucTime == nullptr) {
                      CDBDebug(CTIME_GETINSTANCE_ERROR_MESSAGE);
                      throw(__LINE__);
                    }
                  }

#ifdef CDBFILESCANNER_DEBUG
                  CDBDebug("Dimension type = %s", CDF::getCDFDataTypeName(dimVar->getType()).c_str());
#endif

#ifdef CDBFILESCANNER_DEBUG
                  CDBDebug("Reading dimension %s of length %d", dimVar->name.c_str(), dimDim->getSize());
#endif
                  status = 0;
                  if (dimVar->name.equals("none") == false) {
                    // Strings do never fit in a double.
                    if (dimVar->getType() != CDF_STRING) {
                      // Read the dimension data
                      status = dimVar->readData(CDF_DOUBLE);
                    } else {
                      // Read the dimension data
                      status = dimVar->readData(CDF_STRING);
                    }
                  }
                  // #ifdef CDBFILESCANNER_DEBUG
                  //                    CDBDebug("Reading dimension %s of length
                  //                    %d",dimVar->name.c_str(),dimDim->getSize());
                  // #endif
                  if (status != 0) {
                    CREPORT_ERROR_NODOC(CT::string("Unable to read variable data for ") + dimVar->name, CReportMessage::Categories::GENERAL);
                    throw(__LINE__);
                  }

                  // Check for status flag dimensions

                  CDataSource::readStatusFlags(dimVar, &statusFlagList);
                  if (statusFlagList.size() > 0) hasStatusFlag = true;
                }

                int exceptionAtLineNr = 0;

                bool requiresProjectionInfo = true;

                CDBAdapter::GeoOptions geoOptions;
                geoOptions.level = -1;
                geoOptions.proj4 = "EPSG:4236";
                geoOptions.bbox[0] = -1000;
                geoOptions.bbox[1] = -1000;
                geoOptions.bbox[2] = 1000;
                geoOptions.bbox[3] = 1000;

                if (requiresProjectionInfo) {
                  CDataReader reader;
                  // reader.enableReporting(false); //Functional tests fail if set to false
                  dataSource->addStep((*fileList)[j].c_str(), NULL);
                  reader.open(dataSource, CNETCDFREADER_MODE_OPEN_HEADER);
                  //                      CDBDebug("---> CRS:  [%s]",dataSource->nativeProj4.c_str());
                  //                      CDBDebug("---> BBOX: [%f %f %f
                  //                      %f]",dataSource->dfBBOX[0],dataSource->dfBBOX[1],dataSource->dfBBOX[2],dataSource->dfBBOX[3]);
                  /* crs = dataSource->nativeProj4.c_str();
                   minx = dataSource->dfBBOX[0];
                   miny = dataSource->dfBBOX[1];
                   maxx = dataSource->dfBBOX[2];
                   maxy = dataSource->dfBBOX[3];
                   level = 1 ; //Highest detail, highest resolution, most files.
                  */
                  geoOptions.level = 0;
                  geoOptions.proj4 = dataSource->nativeProj4.c_str();
                  geoOptions.bbox[0] = dataSource->dfBBOX[0];
                  geoOptions.bbox[1] = dataSource->dfBBOX[1];
                  geoOptions.bbox[2] = dataSource->dfBBOX[2];
                  geoOptions.bbox[3] = dataSource->dfBBOX[3];
                  geoOptions.indices[0] = 0;
                  geoOptions.indices[1] = 1;
                  geoOptions.indices[2] = 2;
                  geoOptions.indices[3] = 3;

                  CDF::Attribute *adagucTileLevelAttr = dataSource->getDataObject(0)->cdfObject->getAttributeNE("adaguctilelevel");

                  if (adagucTileLevelAttr != NULL) {
                    geoOptions.level = adagucTileLevelAttr->toString().toInt();
                    // CDBDebug( "Found adaguctilelevel %d in NetCDF header",geoOptions.level);
                  }
                }
                if (dimVar->name.equals("none")) {
                  dbAdapter->setFileInt(tableNames[d].c_str(), (*fileList)[j].c_str(), int(0), int(0), fileDate.c_str(), &geoOptions);
                }

                if (dimVar->name.equals("none") == false) {
                  try {
                    const double *dimValues = (double *)dimVar->data;

                    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    // Start looping over every netcdf dimension element
                    std::set<std::string> uniqueDimensionValueSet;
                    std::pair<std::set<std::string>::iterator, bool> uniqueDimensionValueRet;

                    bool dimIsUnique = true;

                    CT::string uniqueKey;
                    for (size_t i = 0; i < dimDim->length; i++) {

                      CT::string uniqueKey = "";
                      // Insert individual values of type char, short, int, float, double
                      if (dimVar->getType() != CDF_STRING) {
                        if (dimValues[i] != NC_FILL_DOUBLE) {
                          if (isTimeDim[d] == false) {
                            if (hasStatusFlag == true) {
                              uniqueKey.print("%s", CDataSource::getFlagMeaning(&statusFlagList, double(dimValues[i])));
                              uniqueDimensionValueRet = uniqueDimensionValueSet.insert(uniqueKey.c_str());
                              if (uniqueDimensionValueRet.second == true) {
                                dbAdapter->setFileString(tableNames[d].c_str(), (*fileList)[j].c_str(), uniqueKey.c_str(), int(i), fileDate.c_str(), &geoOptions);
                              } else {
                                dimIsUnique = false;
                              }
                            }
                            if (hasStatusFlag == false) {
                              switch (dimVar->getType()) {
                              case CDF_FLOAT:
                              case CDF_DOUBLE:
                                uniqueKey.print("%f", double(dimValues[i]));
                                uniqueDimensionValueRet = uniqueDimensionValueSet.insert(uniqueKey.c_str());
                                if (uniqueDimensionValueRet.second == true) {
                                  dbAdapter->setFileReal(tableNames[d].c_str(), (*fileList)[j].c_str(), double(dimValues[i]), int(i), fileDate.c_str(), &geoOptions);
                                } else {
                                  dimIsUnique = false;
                                }
                                break;
                              default:
                                uniqueKey.print("%d", int(dimValues[i]));
                                uniqueDimensionValueRet = uniqueDimensionValueSet.insert(uniqueKey.c_str());
                                if (uniqueDimensionValueRet.second == true) {
                                  dbAdapter->setFileInt(tableNames[d].c_str(), (*fileList)[j].c_str(), int(dimValues[i]), int(i), fileDate.c_str(), &geoOptions);
                                } else {
                                  dimIsUnique = false;
                                }
                                break;
                              }
                            }
                          } else {

                            // ADTime->PrintISOTime(ISOTime,ISO8601TIME_LEN,dimValues[i]);status = 0;//TODO make
                            // PrintISOTime return a 0 if succeeded

                            try {
                              uniqueKey = adagucTime->dateToISOString(adagucTime->getDate(dimValues[i]));
                              if (!dataSource->cfgLayer->Dimension[d]->attr.quantizeperiod.empty()) {
                                CT::string quantizemethod = "round";
                                CT::string quantizeperiod = dataSource->cfgLayer->Dimension[d]->attr.quantizeperiod;
                                if (!dataSource->cfgLayer->Dimension[d]->attr.quantizemethod.empty()) {
                                  quantizemethod = dataSource->cfgLayer->Dimension[d]->attr.quantizemethod;
                                }
                                // Start time quantization with quantizeperiod and quantizemethod
                                uniqueKey = CTime::quantizeTimeToISO8601(uniqueKey, quantizeperiod, quantizemethod);
                              }
                              uniqueKey.setSize(19);
                              uniqueKey.concat("Z");
                              dbAdapter->setFileTimeStamp(tableNames[d].c_str(), (*fileList)[j].c_str(), uniqueKey.c_str(), int(i), fileDate.c_str(), &geoOptions);

                            } catch (int e) {
                              CDBDebug("Exception occurred during time conversion: %d", e);
                            }
                          }
                        }
                      }

                      if (dimVar->getType() == CDF_STRING) {
                        const char *str = ((char **)dimVar->data)[i];
                        uniqueKey.print("%s", str);
                        uniqueDimensionValueRet = uniqueDimensionValueSet.insert(uniqueKey.c_str());
                        if (uniqueDimensionValueRet.second == true) {
                          dbAdapter->setFileString(tableNames[d].c_str(), (*fileList)[j].c_str(), uniqueKey.c_str(), int(i), fileDate.c_str(), &geoOptions);
                        } else {
                          dimIsUnique = false;
                        }
                      }

                      // Check if this insert is unique
                      if (dimIsUnique == false) {
                        CREPORT_ERROR_NODOC(CT::string("In file ") + (*fileList)[j].c_str() + CT::string(" dimension value [") + uniqueKey + CT::string("] not unique in dimension [") + dimVar->name +
                                                CT::string("]"),
                                            CReportMessage::Categories::GENERAL);
                      }
                    }
                  } catch (int linenr) {
                    CDBError("Exception at linenr %d", linenr);
                    exceptionAtLineNr = linenr;
                  }
                }
                // Cleanup statusflags
                for (size_t i = 0; i < statusFlagList.size(); i++) delete statusFlagList[i];
                statusFlagList.clear();

                // Cleanup adaguctime structure

                if (exceptionAtLineNr != 0) {
                  CDBError("Exception occured at line %d", exceptionAtLineNr);
                  throw(exceptionAtLineNr);
                }
              }

              // delete cdfObject;cdfObject=NULL;
              // cdfObject=CDFObjectStore::getCDFObjectStore()->deleteCDFObject(&cdfObject);
            } catch (int linenr) {
              CDBError("Exception in DBLoopFiles at line %d", linenr);
              CDBError(" *** SKIPPING FILE %s ***", (*fileList)[j].c_str());
              // Close cdfObject. this is only needed if an exception occurs, otherwise it does nothing...
              // delete cdfObject;cdfObject=NULL;

              // TODO CHECK cdfObject=CDFObjectStore::getCDFObjectStore()->deleteCDFObject(&cdfObject);
            }
          }
        }
      }
      // End of dimloop, start inserting our collected records in one statement
      if (numberOfFilesAddedToDbStore % 50 == 0) dbAdapter->addFilesToDataBase();
    }

    // End of dimloop, start inserting our collected records in one statement
    dbAdapter->addFilesToDataBase();

    if (removeNonExistingFiles == 1) {
      // Now delete files in the database a which are not on file system
      for (size_t d = 0; d < dataSource->cfgLayer->Dimension.size(); d++) {
        if (skipDim[d] == false) {
          CDBStore::Store *values = CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->getUniqueValuesOrderedByValue("path", 0, false, tableNames[d].c_str());
          if (values == NULL) {
            CDBError("No files found for %s ", dataSource->layerName.c_str());
          } else {
            CDBDebug("The database contains %d files", values->getSize());

            std::vector<std::string> oldList;
            std::vector<std::string> newList;
            for (size_t j = 0; j < values->getSize(); j++) {
              oldList.push_back(values->getRecord(j)->get(0)->c_str());
            }
            for (size_t i = 0; i < fileList->size(); i++) {
              newList.push_back((*fileList)[i].c_str());
            }
            CDBDebug("Comparing lists");
            filesToDeleteFromDB.clear();
            CDirReader::compareLists(oldList, newList, &handleFileFromDBIsMissing, &handleDirHasNewFile);
            CDBDebug("Found %d files in DB which are missing", filesToDeleteFromDB.size());
            for (size_t j = 0; j < filesToDeleteFromDB.size(); j++) {
              CDBDebug("Deleting file %s from db", filesToDeleteFromDB[j].c_str());
              CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->removeFile(tableNames[d].c_str(), filesToDeleteFromDB[j].c_str());
            }
          }

          delete values;
        }
      }
    }

    if (numberOfFilesAddedFromDB != 0) {
      CDBDebug("%d file(s) were already in the database", numberOfFilesAddedFromDB);
    }

  } catch (int linenr) {
#ifdef USEQUERYTRANSACTIONS
    DB->query("COMMIT");
#endif
    CDBError("Exception in DBLoopFiles at line %d", linenr);

    // TODO CHECK    cdfObject=CDFObjectStore::getCDFObjectStore()->deleteCDFObject(&cdfObject);
    return 1;
  }

  // delete cdfObject;cdfObject=NULL;
  // cdfObject=CDFObjectStore::getCDFObjectStore()->deleteCDFObject(&cdfObject);
  return 0;
}

int CDBFileScanner::updatedb(CDataSource *dataSource, CT::string *_tailPath, CT::string *_layerPathToScan, int scanFlags) {
  bool verbose = dataSource->srvParams->verbose;
  if (dataSource->dLayerType != CConfigReaderLayerTypeDataBase && dataSource->dLayerType != CConfigReaderLayerTypeBaseLayer) return 0;

  if (scanFlags & CDBFILESCANNER_CLEANFILES) {
    return cleanFiles(dataSource, scanFlags);
  }

  /* We only need to update the provided path in layerPathToScan. We will simply ignore the other directories */
  CT::string fileToUpdate;
  if (_layerPathToScan != NULL) {
    if (_layerPathToScan->length() != 0) {
      CT::string layerPath, layerPathToScan;
      layerPath.copy(dataSource->cfgLayer->FilePath[0]->value.c_str());
      layerPathToScan.copy(_layerPathToScan);
      layerPath = CDirReader::makeCleanPath(layerPath.c_str());
      layerPathToScan = CDirReader::makeCleanPath(layerPathToScan.c_str());

      /* If this is another directory we will simply ignore it. */
      if (layerPathToScan.startsWith(layerPath) == false) {
        // CDBDebug ("Skipping %s==%s\n",layerPath.c_str(),layerPathToScan.c_str());
        return CDBFILESCANNER_RETURN_FILEDOESNOTMATCH;
      }
      fileToUpdate = layerPathToScan;
    }
  }
  // This variable enables the query to remove files that no longer exist in the filesystem
  int removeNonExistingFiles = 1;

  int status;

  // Copy tailpath (can be provided to scan only certain subdirs)
  CT::string tailPath(_tailPath);

  tailPath = CDirReader::makeCleanPath(tailPath.c_str());

  // if tailpath is defined than removeNonExistingFiles must be zero
  if (tailPath.length() > 0) removeNonExistingFiles = 0;

  // if layerPathToScan is defined than removeNonExistingFiles must be zero
  if (fileToUpdate.length() > 0) removeNonExistingFiles = 0;

  if (scanFlags & CDBFILESCANNER_DONTREMOVEDATAFROMDB) {
    removeNonExistingFiles = 0;
  }
  CDBDebug("  ==> *** Starting update layer [%s] ***", dataSource->cfgLayer->Name[0]->value.c_str());

  if (verbose) {
    CDBDebug("Using path [%s], filter [%s] and tailpath [%s]", dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), tailPath.c_str());
  }

  CT::string filter = dataSource->cfgLayer->FilePath[0]->attr.filter.c_str();

  if (scanFlags & CDBFILESCANNER_IGNOREFILTER) {
    filter = "^.*$";
  }

  std::vector<std::string> fileList;
  if (fileToUpdate.length() == 0) {
    // No file specified, just scan the directory for matching filenames.
    try {
      if (scanFlags & CDBFILESCANNER_UPDATEDB_ONLYFILEFROMDEFAULTQUERY) {
        if (checkIfPathIsFile(dataSource->cfgLayer->FilePath[0]->value.c_str())) {
          fileList.push_back(dataSource->cfgLayer->FilePath[0]->value.c_str());
          CDBDebug("Obtained filename from layer configuration [%s]", dataSource->cfgLayer->FilePath[0]->value.c_str());
        } else {
          CDBDebug("CDBFILESCANNER_UPDATEDB_ONLYFILEFROMDEFAULTQUERY");

          std::string fileName;
          if (CAutoConfigure::getFileNameForDataSource(dataSource, fileName) != 0) {
            CDBDebug("Unable to getFileNameForDataSource");
            return 1;
          }
          fileList.push_back(fileName);
          CDBDebug("Queried file from database with filename [%s]", fileName.c_str());
        }
      } else {
        fileList = searchFileNames(dataSource->cfgLayer->FilePath[0]->value.c_str(), filter.c_str(), tailPath.c_str());
      }

    } catch (int linenr) {
      CDBDebug("Exception in searchFileNames [%s] [%s]", dataSource->cfgLayer->FilePath[0]->value.c_str(), filter.c_str(), tailPath.c_str());
      return 0;
    }
  } else {
    // File specified, check if it matches the layer filter.
    if (verbose) {
      CDBDebug("Checking specified fileToUpdate %s with filter %s", fileToUpdate.c_str(), filter.c_str());
    }
    CT::string fileToCheckAgainstRegexp = fileToUpdate.basename();
    if (fileToUpdate.equals(dataSource->cfgLayer->FilePath[0]->value) || CDirReader::testRegEx(fileToCheckAgainstRegexp, filter.c_str()) == 1) {
      if (verbose) {
        CDBDebug("Add specified file %s with filter %s for scanning", fileToCheckAgainstRegexp.c_str(), filter.c_str());
      }
      fileList.push_back(fileToUpdate.c_str());

    } else {
      if (verbose) {
        CDBDebug("Ignoring specified file %s does not match filter %s", fileToCheckAgainstRegexp.c_str(), filter.c_str());
      }
    }
  }
  if (verbose) {
    CDBDebug("Going to scan %d files", fileList.size());
  }

  // Include tiles: TODO this is a heavy routine!!
  if (tailPath.length() == 0) {
    if (dataSource->cfgLayer->TileSettings.size() == 1) {
      CDBDebug("Start including TileSettings path [%s]. (Already found %d non tiled files)", dataSource->cfgLayer->TileSettings[0]->attr.tilepath.c_str(), fileList.size());
      try {
        std::vector<std::string> fileListForTiles = searchFileNames(dataSource->cfgLayer->TileSettings[0]->attr.tilepath.c_str(), "^.*\\.nc$", tailPath.c_str());
        if (fileListForTiles.size() == 0) throw(__LINE__);
        CDBDebug("Found %d tiles", fileListForTiles.size());
        for (size_t j = 0; j < fileListForTiles.size(); j++) {
          fileList.push_back(fileListForTiles[j].c_str());
        }
      } catch (int linenr) {
        CDBDebug("No tiles found");
      }
    }
  }

  if (fileList.size() == 0) {
    if (verbose) {
      CDBWarning("No files found for layer %s", dataSource->cfgLayer->Name[0]->value.c_str());
    }
    return CDBFILESCANNER_RETURN_FILEDOESNOTMATCH;
  }

  try {
    // First check and create all tables... returns zero on success, positive on error, negative on already done.
    status = createDBUpdateTables(dataSource, removeNonExistingFiles, &fileList, scanFlags & CDBFILESCANNER_RECREATETABLES);
    if (status > 0) {
      CDBError("createDBUpdateTables failed");
      throw(__LINE__);
    }

    if (status == 0) {

      // Loop Through all files
      status = DBLoopFiles(dataSource, removeNonExistingFiles, &fileList, scanFlags);
      if (status != 0) {
        CDBError("DBLoopFiles failed");
        throw(__LINE__);
      }

      // Clean up if needed
      cleanFiles(dataSource, scanFlags);
    }
  } catch (int linenr) {
    CDBError("Exception in updatedb at line %d", linenr);
#ifdef USEQUERYTRANSACTIONS
    if (removeNonExistingFiles == 1) status = DB->query("COMMIT");
#endif

    // Clean up if needed
    cleanFiles(dataSource, scanFlags);

    return 1;
  }

// Close DB
// CDBDebug("COMMIT");
#ifdef USEQUERYTRANSACTIONS
  if (removeNonExistingFiles == 1) status = DB->query("COMMIT");
#endif

  /* Now Check autotile option */
  if (!(scanFlags & CDBFILESCANNER_DONOTTILE)) {
    if (dataSource->cfgLayer->TileSettings.size() == 1) {
      if (dataSource->cfgLayer->TileSettings[0]->attr.autotile.equals("true")) {
        for (size_t j = 0; j < fileList.size(); j++) {
          if (!(fileList[j].rfind(dataSource->cfgLayer->TileSettings[0]->attr.tilepath.c_str(), 0) == 0)) {
            CCreateTiles::createTilesForFile(dataSource, CDBFILESCANNER_CREATETILES + CDBFILESCANNER_UPDATEDB, fileList[j].c_str());
          }
        }
      }
    }
  }

  if (scanFlags & CDBFILESCANNER_UPDATEDB) {
    updateMetaDataTable(dataSource);
  }

  CDBDebug("  ==> *** Finished update layer [%s] ***", dataSource->cfgLayer->Name[0]->value.c_str());
  return 0;
}

// TODO READ FILE FROM DB!
std::vector<std::string> CDBFileScanner::searchFileNames(const char *path, CT::string expr, const char *tailPath) {
#ifdef CDBFILESCANNER_DEBUG
  CDBDebug("searchFileNames");
#endif
  if (path == NULL) {
    CDBError("No path defined");
    throw(__LINE__);
  }
  CT::string filePath = path;
  //  CDBDebug("filePath = %s",filePath.c_str());

  if (tailPath != NULL) {
    if (tailPath[0] == '/') {
      filePath.copy(tailPath);

      CT::string baseName = filePath.substring(filePath.lastIndexOf("/") + 1, -1);
      if (CDirReader::testRegEx(baseName.c_str(), expr.c_str()) != 1) {
        CDBWarning("Filter [%s] does not match path [%s]. Tailpath = [%s]", expr.c_str(), baseName.c_str(), tailPath);
        throw(__LINE__);
      }
    } else {
      if (strlen(tailPath) > 0) {
        filePath += "/";
        filePath.concat(tailPath);
      }
    }
  }

  if (checkIfPathIsFile(filePath)) {
    std::vector<std::string> fileList;
    fileList.push_back(filePath.c_str());
    //    CDBDebug("%s is a file",filePath.c_str());
    return fileList;
  } else {
    // Read directory

    filePath = CDirReader::makeCleanPath(filePath.c_str());
    try {
      CT::string fileFilterExpr(".*\\.nc$");
      if (expr.empty() == false) { // dataSource->cfgLayer->FilePath[0]->attr.filter.c_str()
        fileFilterExpr.copy(&expr);
      }
      //      CDBDebug("Reading directory %s with filter %s", filePath.c_str(), fileFilterExpr.c_str());

      CDirReader *dirReader = CCachedDirReader::getDirReader(filePath.c_str(), fileFilterExpr.c_str());
      dirReader->listDirRecursive(filePath.c_str(), fileFilterExpr.c_str());
      std::vector<std::string> fileList;

      // Delete all files that start with a "." from the filelist.
      for (size_t j = 0; j < dirReader->fileList.size(); j++) {
        if (CT::string(dirReader->fileList[j].c_str()).basename().c_str()[0] != '.') {
          fileList.push_back(dirReader->fileList[j].c_str());
        }
      }

      return fileList;
    } catch (const char *msg) {
      CDBDebug("Directory %s does not exist, silently skipping...", filePath.c_str());
      throw(__LINE__);
    }

    throw(__LINE__);
  }

  throw(__LINE__);
}
