/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2015-05-06
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
#ifdef ADAGUC_USE_SQLITE
#include "CDBAdapterSQLLite.h"
#include <set>
#include "CDebugger.h"
#include "CTime.h"

//#define CDBAdapterSQLLite_DEBUG

const char *CDBAdapterSQLLite::className="CDBAdapterSQLLite";

const char *CDBAdapterSQLLite::CSQLLiteDB::className="CSQLLiteDB";

CDBAdapterSQLLite::CSQLLiteDB::CSQLLiteDB(){
  db = NULL;
  errorMessage ="";
  zErrMsg = NULL;
}

CDBAdapterSQLLite::CSQLLiteDB::~CSQLLiteDB(){
  close();
  zErrMsg = NULL;
}

std::set<std::string> CDBAdapterSQLLite::CSQLLiteDB::columnSet;
std::vector<std::string> CDBAdapterSQLLite::CSQLLiteDB::columnNames;
std::vector<std::string> CDBAdapterSQLLite::CSQLLiteDB::queryValues;
std::pair<std::set<std::string>::iterator,bool> CDBAdapterSQLLite::CSQLLiteDB::ret;


int CDBAdapterSQLLite::CSQLLiteDB::callbacknoresults(void *NotUsed, int argc, char **argv, char **azColName){
  return 0;
};
int CDBAdapterSQLLite::CSQLLiteDB::callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  //Set column names
  for(i=0; i<argc; i++){
    ret = columnSet.insert(azColName[i]); 
    if (ret.second==false){
      break;
    }
    columnNames.push_back(azColName[i]);
  }
  
  for(i=0; i<argc; i++){
    if(argv[i]!=NULL){
    queryValues.push_back(argv[i]); 
    }else{
      queryValues.push_back(""); 
    }
  }
 

  return 0;
}

int CDBAdapterSQLLite::CSQLLiteDB::close(){
  if(db!=NULL){
    sqlite3_close(db);
    db = NULL;
  }
  return 0;
};

int CDBAdapterSQLLite::CSQLLiteDB::connect(const char * pszOptions){

  int rc = sqlite3_open(pszOptions, &db);
  if( rc ){
    CDBError( "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    db = NULL;
    return 1 ;
  }

  return 0;
  
};

int CDBAdapterSQLLite::CSQLLiteDB::checkTable(const char * pszTableName,const char *pszColumns){
  // 0 = no change
  // 1 = error
  // 2 = table created
  
  
  CT::string q;
  q.print("select * from %s limit 1",pszTableName);
  int status = query(q.c_str());
  
  if(status == 0){
#ifdef CDBAdapterSQLLite_DEBUG    
    CDBDebug("Table exists");
#endif
    return 0;
  }
  
  q.print("CREATE TABLE %s (%s)",pszTableName,pszColumns);
  status = query(q.c_str());
  if(status == 0){
#ifdef CDBAdapterSQLLite_DEBUG        
    CDBDebug("created");
#endif
    return 2;
  }
#ifdef CDBAdapterSQLLite_DEBUG      
  CDBDebug("Table creation failed: %s",q.c_str());
#endif  
  
  return 1;
}

CDBStore::Store* CDBAdapterSQLLite::CSQLLiteDB::queryToStore(const char *pszQuery,bool throwException){
#ifdef CDBAdapterSQLLite_DEBUG
  CDBDebug("queryToStore %s",pszQuery);
#endif  
  columnSet.clear();
  queryValues.clear();
  columnNames.clear();
  int rc = sqlite3_exec(db, pszQuery, CDBAdapterSQLLite::CSQLLiteDB::callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    //CDBError("SQL error: %s\n", zErrMsg);
    errorMessage = zErrMsg;
    sqlite3_free(zErrMsg);
    return NULL;
  }
  
  
  
  std::set<std::string>::iterator it;


  size_t numCols = columnNames.size();
#ifdef CDBAdapterSQLLite_DEBUG  
  CDBDebug("Query succeeded:numCols =  %d",numCols);
#endif
  if(numCols == 0){
    return new CDBStore::Store(new CDBStore::ColumnModel(0));
  }
  
  size_t numRows = queryValues.size() / numCols;

#ifdef CDBAdapterSQLLite_DEBUG  
  CDBDebug("Numcols = %d numRows = %d",numCols,numRows);
#endif
  
  CDBStore::ColumnModel *colModel = new CDBStore::ColumnModel(numCols);
  
  
  for (size_t colNumber=0;colNumber<columnNames.size();colNumber++){    
    colModel->setColumn(colNumber,columnNames[colNumber].c_str());
  }
  
  
  CDBStore::Store *store=new CDBStore::Store(colModel);
  
  if(numRows == 0){
    return store;
  }

  
  
  for(size_t rowNumber=0;rowNumber<numRows;rowNumber++){
    CDBStore::Record *record = new CDBStore::Record(colModel);
    for(size_t colNumber=0;colNumber<numCols;colNumber++)record->push(colNumber,queryValues[colNumber+rowNumber*numCols].c_str());
    store->push(record);
  }
  
  
  return store;
}

CDBStore::Store* CDBAdapterSQLLite::CSQLLiteDB::queryToStore(const char *pszQuery){
  return queryToStore(pszQuery,false);
}


int CDBAdapterSQLLite::CSQLLiteDB::query(const char *pszQuery){
  
  int rc = sqlite3_exec(db, pszQuery, CDBAdapterSQLLite::CSQLLiteDB::callbacknoresults, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    errorMessage = zErrMsg;
    sqlite3_free(zErrMsg);
    return 1;
  }
  
  return 0;
}



CDBAdapterSQLLite::CDBAdapterSQLLite(){
#ifdef CDBAdapterSQLLite_DEBUG
  CDBDebug("CDBAdapterSQLLite()");
#endif  
  dataBaseConnection = NULL;
}

CDBAdapterSQLLite::~CDBAdapterSQLLite(){
#ifdef CDBAdapterSQLLite_DEBUG  
  CDBDebug("~CDBAdapterSQLLite()");
#endif  
  if(dataBaseConnection!=NULL){
    dataBaseConnection->close();
  }
  delete dataBaseConnection ;
  dataBaseConnection = NULL;
}


int CDBAdapterSQLLite::setConfig(CServerConfig::XMLE_Configuration *cfg){
  configurationObject = cfg;
  return 0;
}

CDBStore::Store *CDBAdapterSQLLite::getMax(const char *name,const char *table){
  CSQLLiteDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }
  
  CT::string query;
  
  query.print("select max(%s) from %s",name,table);
  CDBStore::Store *maxStore = DB->queryToStore(query.c_str());
  if(maxStore == NULL){
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for  %s",name);
    CDBError("query failed"); return NULL;
  }
  return maxStore;
};

CDBStore::Store *CDBAdapterSQLLite::getMin(const char *name,const char *table){
  CSQLLiteDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }
  CT::string query;
  
  
  query.print("select min(%s) from %s",name,table);
  CDBStore::Store *maxStore = DB->queryToStore(query.c_str());
  if(maxStore == NULL){
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for  %s",name);
    CDBError("query failed"); return NULL;
  }
  return maxStore;
};


CDBStore::Store *CDBAdapterSQLLite::getUniqueValuesOrderedByValue(const char *name, int limit, bool orderDescOrAsc,const char *table){
  CSQLLiteDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }
  
  CT::string query;
  query.print("select %s from %s group by %s order by %s %s",name,table,name,name,orderDescOrAsc?"asc":"desc");
  if(limit>0){
    query.printconcat(" limit %d",limit);
  }
  CDBStore::Store* store = DB->queryToStore(query.c_str());
  if(store == NULL){
    CDBDebug("Query %s failed",query.c_str());
  }
  return store;
}

CDBStore::Store *CDBAdapterSQLLite::getUniqueValuesOrderedByIndex(const char *name, int limit, bool orderDescOrAsc,const char *table){
  CSQLLiteDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }
  
  CT::string query;
   query.print("select distinct %s,dim%s from %s order by dim%s,%s",name,name,table,name,name);
  if(limit>0){
    query.printconcat(" limit %d",limit);
  }
  return DB->queryToStore(query.c_str());
}



CDBStore::Store *CDBAdapterSQLLite::getReferenceTime(const char *netcdfReferenceTimeDimName,const char *netcdfTimeDimName,const char *timeValue,const char *timeTableName,const char *referenceTimeTableName){
  
  //CDBDebug("SQLITE::getReferenceTime searching for %s",timeValue);
  CSQLLiteDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }
  CT::string query;
  //                                 netcdfReferenceTimeDimName           netcdfTimeDimName-netcdfReferenceTimeDimName      referenceTimeTableName         timeTableName netcdfTimeDimName 'timeValue'
  query.print("select * from (select %s,%s as age from ( select * from %s)a0 ,( select * from %s where %s = '%s')a1 where a0.path = a1.path )a0",
                    netcdfReferenceTimeDimName,
                    netcdfTimeDimName,
                    referenceTimeTableName,
                    timeTableName,
                    netcdfTimeDimName,
                    timeValue
                    );
  
  CDBStore::Store*store = DB->queryToStore(query.c_str())  ;
  if(store == NULL)return NULL;

  CTime ctime;  
  ctime.init("seconds since 1970");
  
  double minDifference = INFINITY;
  bool differenceWasFound = false;
  CT::string foundReferenceTime;
  
  for(size_t j=0;j<store->getSize();j++){
    try{
      CDBStore::Record*record = store->getRecord(j);
      CT::string forecast_reference_time_value = record->get(0)->c_str();
      CT::string time_value = record->get(1)->c_str();
      double refTimeAsEpoch,timeAsEpoch,difference ;
  
      //Convert reftime to epoch
      try{
        refTimeAsEpoch = ctime.dateToOffset( ctime.ISOStringToDate(forecast_reference_time_value.c_str()));
      }catch(int e){
        CDBDebug("Unable to convert %s to epoch",forecast_reference_time_value.c_str());
        throw(__LINE__);
      }
      //Convert time to epoch
      try{
        timeAsEpoch = ctime.dateToOffset( ctime.ISOStringToDate(time_value.c_str()));
      }catch(int e){
        CDBDebug("Unable to convert %s to epoch",time_value.c_str());
        throw(__LINE__);
      }
      difference = refTimeAsEpoch-timeAsEpoch;
      
      if(difference<minDifference){
        minDifference= difference;
        foundReferenceTime = forecast_reference_time_value;
        differenceWasFound = true;
      }
    }catch(int e){
    }
  }
  delete store;
  
  if(differenceWasFound == false){
    CDBDebug("No reference times found");
    return NULL;
  }
  
  CDBDebug("SQLITE::getReferenceTime gives %s for %s",foundReferenceTime.c_str(),timeValue);
  
  CDBStore::ColumnModel *columnModel = new CDBStore::ColumnModel(1);
  columnModel->setColumn(0,netcdfReferenceTimeDimName);
  CDBStore::Store *result = new CDBStore::Store(columnModel);
  CDBStore::Record *record = new  CDBStore::Record(columnModel);
  record->push(0,foundReferenceTime.c_str());
  result->push(record);
  return result;
};

CDBStore::Store *CDBAdapterSQLLite::getClosestDataTimeToSystemTime(const char *netcdfReferenceTimeDimName,const char *referenceTimeTableName){
  CSQLLiteDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }
  CT::string query;
  

  //query.print("SELECT %s,abs(EXTRACT(EPOCH FROM (%s - now()))) as t from %s order by t asc limit 1",netcdfReferenceTimeDimName,netcdfReferenceTimeDimName,referenceTimeTableName);
  query.print("SELECT %s from %s",netcdfReferenceTimeDimName,referenceTimeTableName);
  
  CDBStore::Store*store = DB->queryToStore(query.c_str())  ;
  if(store == NULL)return NULL;

  CTime ctime;  
  ctime.init("seconds since 1970");
  
  
  CT::string currentTimeString = CTime::currentDateTime();
  
  double currentTimeAsEpoch ;
  
  try{
    currentTimeAsEpoch = ctime.dateToOffset( ctime.freeDateStringToDate(currentTimeString.c_str()));
    CT::string currentDateConverted = ctime.dateToISOString(ctime.getDate(currentTimeAsEpoch));
    CDBDebug("Epoch time = %f = %s",currentTimeAsEpoch,currentDateConverted.c_str());
  }catch(int e){
    CDBDebug("Unable to convert %s to epoch",currentTimeString.c_str());
    delete store;
    return NULL;
  }
  
  double minDifference = INFINITY;
  bool differenceWasFound = false;
  CT::string foundReferenceTime;
  
  CDBDebug("found %d times",store->getSize());
  
  for(size_t j=0;j<store->getSize();j++){
    try{
      CDBStore::Record*record = store->getRecord(j);
      CT::string forecast_reference_time_value = record->get(0)->c_str();

      double refTimeAsEpoch,difference ;
  
      //Convert reftime to epoch
      try{
        refTimeAsEpoch = ctime.dateToOffset( ctime.ISOStringToDate(forecast_reference_time_value.c_str()));
      }catch(int e){
        CDBDebug("Unable to convert %s to epoch",forecast_reference_time_value.c_str());
        throw(__LINE__);
      }
      
      difference = fabs(refTimeAsEpoch-currentTimeAsEpoch);
      
      if(difference<minDifference){
        minDifference= difference;
        foundReferenceTime = forecast_reference_time_value;
        differenceWasFound = true;
      }
      
      
    }catch(int e){
    }
  }
  delete store;
  
  if(differenceWasFound == false){
    CDBDebug("No reference times found");
    return NULL;
  }
  
  CDBDebug("SQLITE::getClosestReferenceTimeToSystemTime gives %s for %s",foundReferenceTime.c_str(),currentTimeString.c_str());
  
  CDBStore::ColumnModel *columnModel = new CDBStore::ColumnModel(1);
  columnModel->setColumn(0,netcdfReferenceTimeDimName);
  CDBStore::Store *result = new CDBStore::Store(columnModel);
  CDBStore::Record *record = new  CDBStore::Record(columnModel);
  record->push(0,foundReferenceTime.c_str());
  result->push(record);
  return result;
  
};



CDBStore::Store *CDBAdapterSQLLite::getFilesAndIndicesForDimensions(CDataSource *dataSource,int limit){
  CSQLLiteDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }

  
  
  CT::string queryOrderedDESC;
  CT::string query;
  queryOrderedDESC.print("select a0.path");
  for(size_t i=0;i<dataSource->requiredDims.size();i++){
    queryOrderedDESC.printconcat(",%s,dim%s",dataSource->requiredDims[i]->netCDFDimName.c_str(),dataSource->requiredDims[i]->netCDFDimName.c_str());
    
  }
  
  
  
  queryOrderedDESC.concat(" from ");
  bool timeValidationError = false;
  
  #ifdef CDBAdapterSQLLite_DEBUG
  CDBDebug("%s",queryOrderedDESC.c_str());
  #endif
  
  //Compose the query
  for(size_t i=0;i<dataSource->requiredDims.size();i++){
    CT::string netCDFDimName(&dataSource->requiredDims[i]->netCDFDimName);

    CT::string tableName;
    try{
      tableName = getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), netCDFDimName.c_str(),dataSource);
    }catch(int e){
      CDBError("Unable to create tableName from '%s' '%s' '%s'",dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), netCDFDimName.c_str());
      return NULL;
    }

    CT::string subQuery;
    subQuery.print("(select path,dim%s,%s from %s ",netCDFDimName.c_str(),
                netCDFDimName.c_str(),
                tableName.c_str());
    CT::string queryParams(&dataSource->requiredDims[i]->value);
    if(queryParams.equals("*")==false){
      CT::string *cDims =queryParams.splitToArray(",");// Split up by commas (and put into cDims)
      for(size_t k=0;k<cDims->count;k++){
        CT::string *sDims =cDims[k].splitToArray("/");// Split up by slashes (and put into sDims)
        
        if(k==0){
          subQuery.concat("where ");
        }
        if(sDims->count>0&&k>0)subQuery.concat("or ");
        for(size_t  l=0;l<sDims->count&&l<2;l++){
          if(sDims[l].length()>0){
            
            //Determine column type (timestamp, integer, real)
            bool isRealType = false;
            CT::string dataTypeQuery;
#ifdef CDBAdapterSQLLite_DEBUG                
            CDBDebug("Get columntype for %s %s",tableName.c_str(),netCDFDimName.c_str());
#endif            
            dataTypeQuery.print("PRAGMA table_info(%s)",tableName.c_str());//,netCDFDimName.c_str());
            #ifdef CDBAdapterSQLLite_DEBUG
            CDBDebug("%s",dataTypeQuery.c_str());
            #endif
            try{
              
              CDBStore::Store * dataType = DB->queryToStore(dataTypeQuery.c_str(),true);
              
              if(dataType!=NULL){
                if(dataType->getSize()>0){
            
                  for(size_t j=0;j<dataType->getSize();j++){
                    if(dataType->getRecord(j)->get(1)->equals(netCDFDimName.c_str())){
                      if(dataType->getRecord(j)->get(2)->equals("real")){
                        isRealType = true;
                      }
                      break;
                    }
                  }
                  
                }
              }
              
              delete dataType;dataType = NULL;
              
            }catch(int e){
              if((CServerParams::checkDataRestriction()&SHOW_QUERYINFO)==false)query.copy("hidden");
              CDBError("Unable to determine column type: '%s'",dataTypeQuery.c_str());
              return NULL;
            }
            
            
          
            if(l>0)subQuery.concat("and ");
            if(sDims->count==1){
              
              if(!CServerParams::checkTimeFormat(sDims[l]))timeValidationError=true;
              
              if(isRealType == false){
                subQuery.printconcat("%s = '%s' ",netCDFDimName.c_str(),sDims[l].c_str());
              }
              
              //This query gets the closest value from the table.
              if(isRealType){
                subQuery.printconcat("abs(%s - %s) = (select min(abs(%s - %s)) from %s)",
                                    sDims[l].c_str(),
                                    netCDFDimName.c_str(),
                                    sDims[l].c_str(),
                                    netCDFDimName.c_str(),
                                    tableName.c_str());
              }
            }
            
            //TODO Currently only start/stop is supported, start/stop/resolution is not supported yet.
            if(sDims->count>=2){
              if(l==0){
                if(!CServerParams::checkTimeFormat(sDims[l]))timeValidationError=true;
                subQuery.printconcat("%s >= '%s' ",netCDFDimName.c_str(),sDims[l].c_str());
              }
              if(l==1){
                if(!CServerParams::checkTimeFormat(sDims[l]))timeValidationError=true;
                subQuery.printconcat("%s <= '%s' ",netCDFDimName.c_str(),sDims[l].c_str());
              }
            }
          }
        }
        delete[] sDims;
      }
      delete[] cDims;
    }
    if(i==0){
      subQuery.printconcat("ORDER BY %s DESC limit %d)a%d ",netCDFDimName.c_str(),limit,i);
      //subQuery.printconcat("ORDER BY %s DESC )a%d ",netCDFDimName.c_str(),i);
    }else{
      subQuery.printconcat("ORDER BY %s DESC)a%d ",netCDFDimName.c_str(),i);
    }
    //subQuery.printconcat("ORDER BY %s DESC)a%d ",netCDFDimName.c_str(),i);
    if(i<dataSource->requiredDims.size()-1)subQuery.concat(",");
    queryOrderedDESC.concat(&subQuery);
  }
  
  #ifdef CDBAdapterSQLLite_DEBUG
  CDBDebug("%s",queryOrderedDESC.c_str());
  #endif
  //Join by path
  if(dataSource->requiredDims.size()>1){
    queryOrderedDESC.concat(" where a0.path=a1.path");
    for(size_t i=2;i<dataSource->requiredDims.size();i++){
      queryOrderedDESC.printconcat(" and a0.path=a%d.path",i);
    }
  }
  #ifdef CDBAdapterSQLLite_DEBUG
  CDBDebug("%s",queryOrderedDESC.c_str());
  #endif
  //writeLogFile3(queryOrderedDESC.c_str());
  //writeLogFile3("\n");
  //queryOrderedDESC.concat(" limit 40");

  if(timeValidationError==true){
    if((CServerParams::checkDataRestriction()&SHOW_QUERYINFO)==false)queryOrderedDESC.copy("hidden");
    CDBError("queryOrderedDESC fails regular expression: '%s'",queryOrderedDESC.c_str());
    return NULL;
  }
  
  query.print("select distinct * from (%s)T order by ",queryOrderedDESC.c_str());
  query.concat(&dataSource->requiredDims[0]->netCDFDimName);
  for(size_t i=1;i<dataSource->requiredDims.size();i++){
    query.printconcat(",%s",dataSource->requiredDims[i]->netCDFDimName.c_str());
  }
  
  //Execute the query
  
    //writeLogFile3(query.c_str());
    //writeLogFile3("\n");
  //values_path = DB.query_select(query.c_str(),0);
  #ifdef CDBAdapterSQLLite_DEBUG
  CDBDebug("%s",query.c_str());
  #endif
  
  CDBStore::Store *store = NULL;
  try{
    store = DB->queryToStore(query.c_str(),true);
  }catch(int e){
    if((CServerParams::checkDataRestriction()&SHOW_QUERYINFO)==false)query.copy("hidden");
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for layer %s",dataSource->cfgLayer->Name[0]->value.c_str());
    CDBDebug("Query failed with code %d (%s)",e,query.c_str());
    return NULL;
  }
  return store;
}

int  CDBAdapterSQLLite::autoUpdateAndScanDimensionTables(CDataSource *dataSource){
  CServerParams *srvParams = dataSource->srvParams;;
  CServerConfig::XMLE_Layer * cfgLayer = dataSource->cfgLayer;
  CSQLLiteDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CCache::Lock lock;
  CT::string identifier = "checkDimTables";  identifier.concat(cfgLayer->FilePath[0]->value.c_str());  identifier.concat("/");  identifier.concat(cfgLayer->FilePath[0]->attr.filter.c_str());  
  CT::string cacheDirectory = srvParams->cfg->TempDir[0]->attr.value.c_str();
  //srvParams->getCacheDirectory(&cacheDirectory);
  if(cacheDirectory.length()>0){
    lock.claim(cacheDirectory.c_str(),identifier.c_str(),"checkDimTables",srvParams->isAutoResourceEnabled());
  }
  
  #ifdef CDBAdapterSQLLite_DEBUG
  CDBDebug("[checkDimTables]");
  #endif
  bool tableNotFound=false;
  bool fileNeedsUpdate = false;
  CT::string dimName;
  for(size_t i=0;i<cfgLayer->Dimension.size();i++){
    dimName=cfgLayer->Dimension[i]->attr.name.c_str();
    
    CT::string tableName;
    try{
      tableName = getTableNameForPathFilterAndDimension(cfgLayer->FilePath[0]->value.c_str(),cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str(),dataSource);
    }catch(int e){
      CDBError("Unable to create tableName from '%s' '%s' '%s'",cfgLayer->FilePath[0]->value.c_str(),cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str());
      return 1;
    }
    
    CT::string query;
    query.print("select path,filedate,%s from %s limit 1",dimName.c_str(),tableName.c_str());
    CDBStore::Store *store = dataBaseConnection->queryToStore(query.c_str());
    if(store==NULL){
      tableNotFound=true;
      CDBDebug("No table found for dimension %s",dimName.c_str());
    }
    
    if(tableNotFound == false){
      if(srvParams->isAutoLocalFileResourceEnabled()==true){
        try{
          CT::string databaseTime = store->getRecord(0)->get(1);if(databaseTime.length()<20){
            databaseTime.concat("Z");
            
          }databaseTime.setChar(10,'T');
          
          CT::string fileDate = CDirReader::getFileDate(store->getRecord(0)->get(0)->c_str());
          
          
          
          if(databaseTime.equals(fileDate)==false){
            CDBDebug("Table was found, %s ~ %s : %d",fileDate.c_str(),databaseTime.c_str(),databaseTime.equals(fileDate));
            fileNeedsUpdate = true;
          }
          
        }catch(int e){
          CDBDebug("Unable to get filedate from database, error: %s",CDBStore::getErrorMessage(e));
          fileNeedsUpdate = true;
        }
        
          
      }
    }
    
    delete store;
    if(tableNotFound||fileNeedsUpdate)break;
  }
  
  
  
  if(fileNeedsUpdate == true){
    //Recreate table
    if(srvParams->isAutoLocalFileResourceEnabled()==true){
      for(size_t i=0;i<cfgLayer->Dimension.size();i++){
        dimName=cfgLayer->Dimension[i]->attr.name.c_str();
      
        CT::string tableName;
        try{
          tableName = getTableNameForPathFilterAndDimension(cfgLayer->FilePath[0]->value.c_str(),cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str(),dataSource);
        }catch(int e){
          CDBError("Unable to create tableName from '%s' '%s' '%s'",cfgLayer->FilePath[0]->value.c_str(),cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str());
          return 1;
        }
        CDBFileScanner::markTableDirty(&tableName);
        //CDBDebug("Dropping old table (if exists)",tableName.c_str());
        CT::string query ;
        query.print("drop table %s",tableName.c_str());
        CDBDebug("Try to %s for %s",query.c_str(),dimName.c_str());
        dataBaseConnection->query(query.c_str());
      }
      tableNotFound = true;
    }
   
  }
 
  
  
  
  if(tableNotFound){
    if(srvParams->isAutoLocalFileResourceEnabled()==true){

      CDBDebug("Updating database");
      int status = CDBFileScanner::updatedb(srvParams->cfg->DataBase[0]->attr.parameters.c_str(),dataSource,NULL,NULL);
      if(status !=0){CDBError("Could not update db for: %s",cfgLayer->Name[0]->value.c_str());return 2;}
    }else{
      CDBDebug("No table found for dimension %s and autoresource is disabled",dimName.c_str());
      return 1;
    }
  }
  #ifdef CDBAdapterSQLLite_DEBUG
  CDBDebug("[/checkDimTables]");
  #endif
  lock.release();
  return 0;
}




CT::string CDBAdapterSQLLite::getTableNameForPathFilterAndDimension(const char *path,const char *filter, const char * dimension,CDataSource *dataSource){
  if(dataSource->cfgLayer->DataBaseTable.size() == 1){
    CT::string tableName = "";
    
    tableName.concat(dataSource->cfgLayer->DataBaseTable[0]->value.c_str());
    CT::string dimName = "";
    if(dimension!=NULL){
      dimName = dimension;
    }
    
    dataSource->srvParams->makeCorrectTableName(&tableName,&dimName);
    return tableName;
    
  }
  
  CT::string identifier = "lookuptable/";  identifier.concat(path);  identifier.concat("/");  identifier.concat(filter);  
  if(dimension!=NULL){identifier.concat("/");identifier.concat(dimension);}
  CT::string tableName;
  
  std::map<std::string,std::string>::iterator it=lookupTableNameCacheMap.find(identifier.c_str());
  if(it!=lookupTableNameCacheMap.end()){
    tableName = (*it).second.c_str();
    //CDBDebug("Returning tablename %s from map",tableName.c_str());
    return tableName;
  }
  
  CCache::Lock lock;
  CT::string cacheDirectory = dataSource->cfg->TempDir[0]->attr.value.c_str();
  //getCacheDirectory(&cacheDirectory);
  if(cacheDirectory.length()>0){
    lock.claim(cacheDirectory.c_str(),identifier.c_str(),"lookupTableName",dataSource->srvParams->isAutoResourceEnabled());
  }

  
 
 
  // This makes use of a lookup table to find the tablename belonging to the filter and path combinations.
  // Database collumns: path filter tablename
  
  CT::string filterString="F_";filterString.concat(filter);
  CT::string pathString="P_";pathString.concat(path);
  CT::string dimString="";if(dimension != NULL){dimString.concat(dimension);dimString.toLowerCaseSelf();}
  
// CDBDebug("lookupTableName %s",identifier.c_str());
  
  CT::string lookupTableName = "pathfiltertablelookup";
  
  //TODO CRUCIAL setting for fast perfomance on large datasets, add Unique to enable building fast lookup indexes.
  CT::string tableColumns("path varchar (511), filter varchar (511), dimension varchar (511), tablename varchar (63), UNIQUE (path,filter,dimension) ");
 // CT::string tableColumns("path varchar (511), filter varchar (511), dimension varchar (511), tablename varchar (63)");
  CT::string mvRecordQuery;
  int status;
  CSQLLiteDB *DB = getDataBaseConnection();if(DB == NULL){CDBError("Unable to connect to DB");throw(1);}

  

  try{

    status = DB->checkTable(lookupTableName.c_str(),tableColumns.c_str());
#ifdef CDBAdapterSQLLite_DEBUG        
    if(status == 0){CDBDebug("OK: Table %s is available",lookupTableName.c_str());}
#endif    
    if(status == 1){
      CDBError("FAIL: Table %s could not be created: %s",lookupTableName.c_str(),tableColumns.c_str());
      CDBError("Error: %s",DB->getError());    
      throw(1);  
    }
#ifdef CDBAdapterSQLLite_DEBUG        
    if(status == 2){CDBDebug("OK: Table %s is created",lookupTableName.c_str());  }
#endif    
    
    
    //Check wether a records exists with this path and filter combination.
    
    bool lookupTableIsAvailable=false;
    
    
    
    if(dimString.length()>1){
      mvRecordQuery.print("SELECT * FROM %s where path='%s' and filter='%s' and dimension='%s'",
                          lookupTableName.c_str(),pathString.c_str(),filterString.c_str(),dimString.c_str());
    }else{
      mvRecordQuery.print("SELECT * FROM %s where path='%s' and filter='%s'",
                          lookupTableName.c_str(),pathString.c_str(),filterString.c_str());
    }

#ifdef CDBAdapterSQLLite_DEBUG        
    CDBDebug("QueryToStore %s",mvRecordQuery.c_str());
#endif    
    CDBStore::Store *rec = DB->queryToStore(mvRecordQuery.c_str()); 

    if(rec==NULL){CDBError("Unable to select records: \"%s\"",mvRecordQuery.c_str());throw(1);  }
    if(rec->getSize()>0){
      tableName.copy(rec->getRecord(0)->get(3));
      if(tableName.length()>0){
        lookupTableIsAvailable = true;
      }
      
    }
   
    delete rec;
    
    //Add a new lookuptable with an unique id.
    if(lookupTableIsAvailable==false){
    
      CT::string randomTableString = "t";
      randomTableString.concat(CTime::currentDateTime());
      randomTableString.concat("_");
      randomTableString.concat(CServerParams::randomString(20));
      randomTableString.replaceSelf(":","");
      randomTableString.replaceSelf("-","");
      randomTableString.replaceSelf("Z",""); 
      
      tableName.copy(randomTableString.c_str());
      tableName.toLowerCaseSelf();
      mvRecordQuery.print("INSERT INTO %s values ('%s','%s','%s','%s')",lookupTableName.c_str(),pathString.c_str(),filterString.c_str(),dimString.c_str(),tableName.c_str());
      //CDBDebug("%s",mvRecordQuery.c_str());
      status = DB->query(mvRecordQuery.c_str()); if(status!=0){CDBError("Unable to insert records: \"%s\"",mvRecordQuery.c_str());throw(1);  }
    }
    //Close the database
  }catch(int e){

    lock.release();
    throw(e);
  }
  
  if(tableName.length()>0){
    //CDBDebug("Pushing %s with id %s",tableName.c_str(),identifier.c_str());
    lookupTableNameCacheMap.insert(std::pair<std::string,std::string>(identifier.c_str(),tableName.c_str()));
  }
  
  lock.release();
  if(tableName.length()<=0){
    CDBError("Unable to generate lookup table name for %s",identifier.c_str());
    throw(1);
  }
  
  return tableName;
}

CDBStore::Store *CDBAdapterSQLLite::getDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername){
  CSQLLiteDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return NULL;  }
  CT::string query;
  query.print("SELECT * FROM autoconfigure_dimensions where layerid='%s_%s'",layertable,layername);
  CDBStore::Store *store = dataBaseConnection->queryToStore(query.c_str());
  if(store==NULL){
    CDBDebug("No dimension info stored for %s",layername);
  }
  return store;
}


int CDBAdapterSQLLite::storeDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername,const char *netcdfname,const char *ogcname,const char *units){
  CSQLLiteDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }

  CT::string query;
  CT::string tableColumns("layerid varchar (255), ncname varchar (255), ogcname varchar (255), units varchar (255)");
  
  int status = dataBaseConnection->checkTable("autoconfigure_dimensions",tableColumns.c_str());
  if(status == 1){CDBError("\nFAIL: Table autoconfigure_dimensions could not be created: %s",tableColumns.c_str()); throw(__LINE__);  }
  
  query.print("INSERT INTO autoconfigure_dimensions values ('%s_%s','%s','%s','%s')",layertable,layername,netcdfname,ogcname,units);
  status = dataBaseConnection->query(query.c_str()); 
  if(status!=0){
    CDBError("Unable to insert records: \"%s\"",query.c_str());
    throw(__LINE__); 
  }
  return 0;
}

int CDBAdapterSQLLite::dropTable(const char *tablename){
  CSQLLiteDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CT::string query;
  query.print("drop table %s",tablename);
  if(dataBaseConnection->query(query.c_str())!=0){
    CDBError("Query %s failed",query.c_str());
    return 1;
  }
  return 0;
}

int CDBAdapterSQLLite::createDimTableOfType(const char *dimname,const char *tablename,int type){
  CSQLLiteDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CT::string tableColumns("path varchar (511)");
   
  if(type == 3)tableColumns.printconcat(", %s varchar (64), dim%s int",dimname,dimname);
  if(type == 2)tableColumns.printconcat(", %s varchar (64), dim%s int",dimname,dimname);
  if(type == 1)tableColumns.printconcat(", %s real, dim%s int",dimname,dimname);
  if(type == 0)tableColumns.printconcat(", %s int, dim%s int",dimname,dimname);
  
  tableColumns.printconcat(", filedate varchar (64)");
  tableColumns.printconcat(", PRIMARY KEY (path, %s)",dimname);
  
  int status = dataBaseConnection->checkTable(tablename,tableColumns.c_str());
  return status;
  
}


int CDBAdapterSQLLite::createDimTableInt(const char *dimname,const char *tablename){
  return createDimTableOfType(dimname,tablename,0);
}

int CDBAdapterSQLLite::createDimTableReal(const char *dimname,const char *tablename){
  return createDimTableOfType(dimname,tablename,1);
}

int CDBAdapterSQLLite::createDimTableString(const char *dimname,const char *tablename){
  return createDimTableOfType(dimname,tablename,2);
}

int CDBAdapterSQLLite::createDimTableTimeStamp(const char *dimname,const char *tablename){
  return createDimTableOfType(dimname,tablename,3);
}

int CDBAdapterSQLLite::checkIfFileIsInTable(const char *tablename,const char *filename){
  int fileIsOK = 1;
  CSQLLiteDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CT::string query;
  query.print("select path from %s where path = '%s' limit 1",tablename,filename);
  CDBStore::Store *pathValues = dataBaseConnection->queryToStore(query.c_str());
  if(pathValues == NULL){CDBError("Query failed");throw(__LINE__);}
  if(pathValues->getSize()==1){fileIsOK=0;}else{fileIsOK=1;}
  delete pathValues;
  return fileIsOK;
}

int CDBAdapterSQLLite::removeFile(const char *tablename,const char *file){
  CSQLLiteDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CT::string query;
  query.print("delete from %s where path = '%s'",tablename,file);
  int status = dataBaseConnection->query(query.c_str()); if(status!=0)throw(__LINE__);
  return 0;
}

int CDBAdapterSQLLite::removeFilesWithChangedCreationDate(const char *tablename,const char *file,const char *creationDate){
  CSQLLiteDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CT::string query;
  query.print("delete from %s where path = '%s' and (filedate != '%s' or filedate is NULL)",tablename,file,creationDate);
  int status = dataBaseConnection->query(query.c_str()); if(status!=0)throw(__LINE__);
  return 0;
}

int CDBAdapterSQLLite::setFileInt(const char *tablename,const char *file,int dimvalue,int dimindex,const char*filedate){
  CT::string values;
  values.print("('%s',%d,'%d','%s')",file,dimvalue,dimindex,filedate);
  #ifdef CDBAdapterSQLLite_DEBUG
  CDBDebug("Adding INT %s",values.c_str());
  #endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}
int CDBAdapterSQLLite::setFileReal(const char *tablename,const char *file,double dimvalue,int dimindex,const char*filedate){
  CT::string values;
  values.print("('%s',%f,'%d','%s')",file,dimvalue,dimindex,filedate);
  #ifdef CDBAdapterSQLLite_DEBUG
  CDBDebug("Adding REAL %s",values.c_str());
  #endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}
int CDBAdapterSQLLite::setFileString(const char *tablename,const char *file,const char * dimvalue,int dimindex,const char*filedate){
  CT::string values;
  values.print("('%s','%s','%d','%s')",file,dimvalue,dimindex,filedate);
  #ifdef CDBAdapterSQLLite_DEBUG
  CDBDebug("Adding STRING %s",values.c_str());
  #endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}
int CDBAdapterSQLLite::setFileTimeStamp(const char *tablename,const char *file,const char *dimvalue,int dimindex,const char*filedate){
  CT::string values;
  values.print("('%s','%s','%d','%s')",file,dimvalue,dimindex,filedate);
  #ifdef CDBAdapterSQLLite_DEBUG
  CDBDebug("Adding TIMESTAMP %s",values.c_str());
  #endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}
int CDBAdapterSQLLite::addFilesToDataBase(){
  #ifdef CDBAdapterSQLLite_DEBUG
  CDBDebug("Adding files to database");
  #endif
  CSQLLiteDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CT::string multiInsert = "";
  for (std::map<std::string,std::vector<std::string> >::iterator it=fileListPerTable.begin(); it!=fileListPerTable.end(); ++it){
      #ifdef CDBAdapterSQLLite_DEBUG
    CDBDebug("Updating table %s with %d records",it->first.c_str(),(it->second.size()));
#endif
    if(it->second.size()>0){
      
      multiInsert.print("INSERT into %s VALUES ",it->first.c_str());
      for(size_t j=0;j<it->second.size();j++){
        if(j>0)multiInsert.concat(",");
        multiInsert.concat(it->second[j].c_str());
      }
      int status =  dataBaseConnection->query(multiInsert.c_str()); 
      if(status!=0){
        CDBError("Query failed [%s]:",dataBaseConnection->getError());
        throw(__LINE__);
      }
      CDBDebug("/Inserting %d bytes",multiInsert.length());
    }
    it->second.clear();
  }
  CDBDebug("clearing arrays");
  fileListPerTable.clear();
  return 0;
}

#endif

 
