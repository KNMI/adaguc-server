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
#ifdef ADAGUC_USE_POSTGRESQL
#include "CDBAdapterPostgreSQL.h"
#include <set>
#include "CDebugger.h"

const char *CDBAdapterPostgreSQL::className="CDBAdapterPostgreSQL";
#define CDBAdapterPostgreSQL_PATHFILTERTABLELOOKUP "pathfiltertablelookup_v2_0_23"
// #define CDBAdapterPostgreSQL_DEBUG
// 
// #define MEASURETIME

CDBAdapterPostgreSQL::CDBAdapterPostgreSQL(){
#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("CDBAdapterPostgreSQL()");
#endif  
  dataBaseConnection = NULL;
}

CDBAdapterPostgreSQL::~CDBAdapterPostgreSQL(){
#ifdef CDBAdapterPostgreSQL_DEBUG  
  CDBDebug("~CDBAdapterPostgreSQL()");
#endif  
  if(dataBaseConnection!=NULL){
    dataBaseConnection->close2();
  }
  delete dataBaseConnection ;
  dataBaseConnection = NULL;
}

CPGSQLDB *CDBAdapterPostgreSQL::getDataBaseConnection(){
  if(dataBaseConnection == NULL){
    #ifdef MEASURETIME
    StopWatch_Stop(">CDBAdapterPostgreSQL::getDataBaseConnection");
    #endif
    dataBaseConnection = new CPGSQLDB();
    int status = dataBaseConnection->connect(configurationObject->DataBase[0]->attr.parameters.c_str());if(status!=0){CDBError("Unable to connect to DB");return NULL;}
    #ifdef MEASURETIME
    StopWatch_Stop("<CDBAdapterPostgreSQL::getDataBaseConnection");
    #endif
  }
  return dataBaseConnection;
}
  
int CDBAdapterPostgreSQL::setConfig(CServerConfig::XMLE_Configuration *cfg){
  configurationObject = cfg;
  return 0;
}

CDBStore::Store *CDBAdapterPostgreSQL::getMax(const char *name,const char *table){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getMax");
  #endif
  CPGSQLDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }
  
  CT::string query;
  // CREATE INDEX MAXINDEX ON t20171017t085357097_iwna17kzcgyvrvr183as (none)
  query.print("select max(%s) from %s",name,table);
  // query.print("SELECT %s FROM %s ORDER BY %s DESC LIMIT 1",name, table,name);
  CDBStore::Store *maxStore = DB->queryToStore(query.c_str());
  if(maxStore == NULL){
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for  %s",name);
    CDBError("query failed"); return NULL;
  }
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getMax");
  #endif
  return maxStore;
};

CDBStore::Store *CDBAdapterPostgreSQL::getMin(const char *name,const char *table){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getMin");
  #endif
  CPGSQLDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }
  CT::string query;
  
  
  query.print("select min(%s) from %s",name,table);
  CDBStore::Store *maxStore = DB->queryToStore(query.c_str());
  if(maxStore == NULL){
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for  %s",name);
    CDBError("query failed"); return NULL;
  }
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getMin");
  #endif
  return maxStore;
};


CDBStore::Store *CDBAdapterPostgreSQL::getUniqueValuesOrderedByValue(const char *name, int limit, bool orderDescOrAsc,const char *table){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getUniqueValuesOrderedByValue");
  #endif
  CPGSQLDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }
  
  CT::string query;
  query.print("select %s from %s group by %s order by %s %s",name,table,name,name,orderDescOrAsc?"asc":"desc");
  if(limit>0){
    query.printconcat(" limit %d",limit);
  }
  CDBStore::Store* store = DB->queryToStore(query.c_str());
  if(store == NULL){
    CDBDebug("Query %s failed",query.c_str());
  }
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getUniqueValuesOrderedByValue");
  #endif
  return store;
}

CDBStore::Store *CDBAdapterPostgreSQL::getUniqueValuesOrderedByIndex(const char *name, int limit, bool orderDescOrAsc,const char *table){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getUniqueValuesOrderedByIndex");
  #endif
  CPGSQLDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }
  
  CT::string query;
   query.print("select distinct %s,dim%s from %s order by dim%s,%s",name,name,table,name,name);
  if(limit>0){
    query.printconcat(" limit %d",limit);
  }
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getUniqueValuesOrderedByIndex");
  #endif
  return DB->queryToStore(query.c_str());
}

CDBStore::Store *CDBAdapterPostgreSQL::getReferenceTime(const char *netcdfReferenceTimeDimName,const char *netcdfTimeDimName,const char *timeValue,const char *timeTableName,const char *referenceTimeTableName){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getReferenceTime");
  #endif
  CPGSQLDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }
  CT::string query;
 
  query.print(
         "select max(forecast_reference_time) from (select %s,%s as age from ( select * from %s)a0 ,( select * from %s where %s = '%s')a1 where a0.path = a1.path )a0",
                    netcdfReferenceTimeDimName,
                    netcdfTimeDimName,
                    referenceTimeTableName,
                    timeTableName,
                    netcdfTimeDimName,
                    timeValue
                    );

  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getReferenceTime");
  #endif
  return DB->queryToStore(query.c_str())  ;
};

CDBStore::Store *CDBAdapterPostgreSQL::getClosestDataTimeToSystemTime(const char *netcdfDimName,const char *tableName){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getClosestDataTimeToSystemTime");
  #endif
  CPGSQLDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }
  CT::string query;
  

  query.print("SELECT %s,abs(EXTRACT(EPOCH FROM (to_timestamp(%s) - now()))) as t from %s order by t asc limit 1",netcdfDimName,netcdfDimName,tableName);
  
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getClosestDataTimeToSystemTime");
  #endif
  return DB->queryToStore(query.c_str());
};

CDBStore::Store *CDBAdapterPostgreSQL::getFilesForIndices(CDataSource *dataSource,size_t *start,size_t *count,ptrdiff_t *stride,int limit){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getFilesForIndices");
  #endif
  #ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("getFilesForIndices");
#endif
   CPGSQLDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }

  
  
  CT::string queryOrderedDESC;
  CT::string query;
  queryOrderedDESC.print("select a0.path");
  for(size_t i=0;i<dataSource->requiredDims.size();i++){
    queryOrderedDESC.printconcat(",%s,dim%s",dataSource->requiredDims[i]->netCDFDimName.c_str(),dataSource->requiredDims[i]->netCDFDimName.c_str());
    
  }
  
  
  
  queryOrderedDESC.concat(" from ");

  
  #ifdef CDBAdapterPostgreSQL_DEBUG
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
    
   
      
    //subQuery.printconcat("where dim%s = %d ",netCDFDimName.c_str(),start[i]);
    subQuery.printconcat("ORDER BY %s ASC limit %d offset %d)a%d ",netCDFDimName.c_str(),count[i],start[i],i);
    if(i<dataSource->requiredDims.size()-1)subQuery.concat(",");
    queryOrderedDESC.concat(&subQuery);
  }
  
  #ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("%s",queryOrderedDESC.c_str());
  #endif
  //Join by path
  if(dataSource->requiredDims.size()>1){
    queryOrderedDESC.concat(" where a0.path=a1.path");
    for(size_t i=2;i<dataSource->requiredDims.size();i++){
      queryOrderedDESC.printconcat(" and a0.path=a%d.path",i);
    }
  }
  #ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("%s",queryOrderedDESC.c_str());
  #endif
    
  query.print("select distinct * from (%s)T order by ",queryOrderedDESC.c_str());
  query.concat(&dataSource->requiredDims[0]->netCDFDimName);
  for(size_t i=1;i<dataSource->requiredDims.size();i++){
    query.printconcat(",%s",dataSource->requiredDims[i]->netCDFDimName.c_str());
  }
  
  //Execute the query
  
    //writeLogFile3(query.c_str());
    //writeLogFile3("\n");
  //values_path = DB.query_select(query.c_str(),0);
  #ifdef CDBAdapterPostgreSQL_DEBUG
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
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getFilesForIndices");
  #endif
  return store;
}

CDBStore::Store *CDBAdapterPostgreSQL::getFilesAndIndicesForDimensions(CDataSource *dataSource,int limit){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getFilesAndIndicesForDimensions");
  #endif
  CPGSQLDB * DB = getDataBaseConnection(); if(DB == NULL){return NULL;  }

  if(dataSource->requiredDims.size()==0){
    CDBError("Unable to do getFilesAndIndicesForDimensions, this datasource has no dimensions: dataSource->requiredDims.size()==0");
    return NULL;
  }
  
//   CDBDebug("dataSource->requiredDims.size() = %d",dataSource->requiredDims.size());
  
  CT::string queryOrderedDESC;
  CT::string query;
  queryOrderedDESC.print("select a0.path");
  for(size_t i=0;i<dataSource->requiredDims.size();i++){
    queryOrderedDESC.printconcat(",%s,dim%s",dataSource->requiredDims[i]->netCDFDimName.c_str(),dataSource->requiredDims[i]->netCDFDimName.c_str());
    
  }
  
  
  
  queryOrderedDESC.concat(" from ");
  bool timeValidationError = false;
  
  #ifdef CDBAdapterPostgreSQL_DEBUG
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
    int numQueriesAdded = 0;
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
            numQueriesAdded++;
            //Determine column type (timestamp, integer, real)
            bool isRealType = false;
            CT::string dataTypeQuery;
            dataTypeQuery.print("select data_type from information_schema.columns where table_name = '%s' and column_name = '%s'",tableName.c_str(),netCDFDimName.c_str());
            #ifdef CDBAdapterPostgreSQL_DEBUG
            CDBDebug("%s",dataTypeQuery.c_str());
            #endif
            try{
              
              CDBStore::Store * dataType = DB->queryToStore(dataTypeQuery.c_str(),true);
              
              if(dataType!=NULL){
                if(dataType->getSize()==1){
                  #ifdef CDBAdapterPostgreSQL_DEBUG
                  CDBDebug("%s",dataType->getRecord(0)->get(0)->c_str());
                  #endif
                  if(dataType->getRecord(0)->get(0)->equals("real")){
                    isRealType = true;
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
                // Get closest lowest value to this requested one, or if request value is way below get earliest value:
                subQuery.printconcat("%s >= (select max(%s) from %s where %s <= '%s' or %s = (select min(%s) from %s)) ",
                                    netCDFDimName.c_str(),
                                    netCDFDimName.c_str(),
                                    tableName.c_str(),
                                    netCDFDimName.c_str(),
                                    sDims[l].c_str(),
                                    netCDFDimName.c_str(),
                                    netCDFDimName.c_str(),
                                    tableName.c_str()
                                    );
                // subQuery.printconcat("%s >= '%s' ",netCDFDimName.c_str(),sDims[l].c_str());
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
      if(numQueriesAdded==0){
        subQuery.printconcat("where ");
      }else{
        subQuery.printconcat("and ");
      }
      if(dataSource->queryBBOX){
        
        subQuery.printconcat("adaguctilinglevel = %d and minx >= %f and maxx <= %f and miny >= %f and maxy <= %f ",dataSource->queryLevel,dataSource->nativeViewPortBBOX[0],dataSource->nativeViewPortBBOX[2],dataSource->nativeViewPortBBOX[1],dataSource->nativeViewPortBBOX[3]);
       
      }
      else{
         subQuery.printconcat("adaguctilinglevel != %d ",-1);
      }
      subQuery.printconcat("ORDER BY %s DESC limit %d)a%d ",netCDFDimName.c_str(),limit,i);
      //subQuery.printconcat("ORDER BY %s DESC )a%d ",netCDFDimName.c_str(),i);
    }else{
      subQuery.printconcat("ORDER BY %s DESC)a%d ",netCDFDimName.c_str(),i);
    }
    //subQuery.printconcat("ORDER BY %s DESC)a%d ",netCDFDimName.c_str(),i);
    if(i<dataSource->requiredDims.size()-1)subQuery.concat(",");
    queryOrderedDESC.concat(&subQuery);
  }
//  CDBDebug("%s",queryOrderedDESC.c_str());
  #ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("%s",queryOrderedDESC.c_str());
  #endif
  //Join by path
  if(dataSource->requiredDims.size()>1){
    queryOrderedDESC.concat(" where a0.path=a1.path");
    for(size_t i=2;i<dataSource->requiredDims.size();i++){
      queryOrderedDESC.printconcat(" and a0.path=a%d.path",i);
    }
  }
  #ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("%s",queryOrderedDESC.c_str());
  #endif

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
  #ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("%s",query.c_str());
  #endif
  
  CDBStore::Store *store = NULL;
  try{
    store = DB->queryToStore(query.c_str(),true);
  }catch(int e){
    if((CServerParams::checkDataRestriction()&SHOW_QUERYINFO)==false)query.copy("hidden");
    setExceptionType(InvalidDimensionValue);
    CDBDebug("Query failed with code %d (%s)",e,query.c_str());
    return NULL;
  }
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getFilesAndIndicesForDimensions");
  #endif
  return store;
}

int  CDBAdapterPostgreSQL::autoUpdateAndScanDimensionTables(CDataSource *dataSource){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::autoUpdateAndScanDimensionTables");
  #endif
  CServerParams *srvParams = dataSource->srvParams;;
  CServerConfig::XMLE_Layer * cfgLayer = dataSource->cfgLayer;
  CPGSQLDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CCache::Lock lock;
  CT::string identifier = "checkDimTables";  identifier.concat(cfgLayer->FilePath[0]->value.c_str());  identifier.concat("/");  identifier.concat(cfgLayer->FilePath[0]->attr.filter.c_str());  
  CT::string cacheDirectory = srvParams->cfg->TempDir[0]->attr.value.c_str();
  //srvParams->getCacheDirectory(&cacheDirectory);
  if(cacheDirectory.length()>0){
    lock.claim(cacheDirectory.c_str(),identifier.c_str(),"checkDimTables",srvParams->isAutoResourceEnabled());
  }
  
  #ifdef CDBAdapterPostgreSQL_DEBUG
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
          CT::string databaseTime = store->getRecord(0)->get(1);if(databaseTime.length()<20){databaseTime.concat("Z");}databaseTime.setChar(10,'T');
          
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
      int status = CDBFileScanner::updatedb(dataSource,NULL,NULL,0);
      if(status !=0){CDBError("Could not update db for: %s",cfgLayer->Name[0]->value.c_str());return 2;}
    }else{
      CDBDebug("No table found for dimension %s and autoresource is disabled",dimName.c_str());
      return 1;
    }
  }
  #ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("[/checkDimTables]");
  #endif
  lock.release();
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::autoUpdateAndScanDimensionTables");
  #endif
  return 0;
}




CT::string CDBAdapterPostgreSQL::getTableNameForPathFilterAndDimension(const char *path,const char *filter, const char * dimension,CDataSource *dataSource){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getTableNameForPathFilterAndDimension");
  #endif
  if(dataSource->cfgLayer->DataBaseTable.size() == 1){
    CT::string tableName = "";
    
    tableName.concat(dataSource->cfgLayer->DataBaseTable[0]->value.c_str());
    CT::string dimName = "";
    if(dimension!=NULL){
      dimName = dimension;
    }
    
    dataSource->srvParams->makeCorrectTableName(&tableName,&dimName);
    #ifdef MEASURETIME
    StopWatch_Stop("<CDBAdapterPostgreSQL::getTableNameForPathFilterAndDimension");
    #endif
    return tableName;
    
  }
  
  CT::string identifier = "lookuptable/";  identifier.concat(path);  identifier.concat("/");  identifier.concat(filter);  
  if(dimension!=NULL){identifier.concat("/");identifier.concat(dimension);}
  CT::string tableName;
  
  std::map<std::string,std::string>::iterator it=lookupTableNameCacheMap.find(identifier.c_str());
  if(it!=lookupTableNameCacheMap.end()){
    tableName = (*it).second.c_str();
    //CDBDebug("Returning tablename %s from map",tableName.c_str());
    #ifdef MEASURETIME
    StopWatch_Stop("<CDBAdapterPostgreSQL::getTableNameForPathFilterAndDimension");
    #endif
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
  
  CT::string lookupTableName = CDBAdapterPostgreSQL_PATHFILTERTABLELOOKUP;
  
  //TODO CRUCIAL setting for fast perfomance on large datasets, add Unique to enable building fast lookup indexes.
  CT::string tableColumns("path varchar (511), filter varchar (511), dimension varchar (511), tablename varchar (63), UNIQUE (path,filter,dimension) ");
 // CT::string tableColumns("path varchar (511), filter varchar (511), dimension varchar (511), tablename varchar (63)");
  CT::string mvRecordQuery;
  int status;
  CPGSQLDB *DB = getDataBaseConnection();if(DB == NULL){CDBError("Unable to connect to DB");throw(1);}

  

  try{

    status = DB->checkTable(lookupTableName.c_str(),tableColumns.c_str());
    //if(status == 0){CDBDebug("OK: Table %s is available",lookupTableName.c_str());}
    if(status == 1){
      CDBError("FAIL: Table %s could not be created: %s",lookupTableName.c_str(),tableColumns.c_str());
      CDBError("Error: %s",DB->getError());    
      throw(1);  
    }
    //if(status == 2){CDBDebug("OK: Table %s is created",lookupTableName.c_str());  }

    
    //Check wether a records exists with this path and filter combination.
    
    bool lookupTableIsAvailable=false;
    
    
    
    if(dimString.length()>1){
      mvRecordQuery.print("SELECT * FROM %s where path=E'%s' and filter=E'%s' and dimension=E'%s'",
                          lookupTableName.c_str(),pathString.c_str(),filterString.c_str(),dimString.c_str());
    }else{
      mvRecordQuery.print("SELECT * FROM %s where path=E'%s' and filter=E'%s'",
                          lookupTableName.c_str(),pathString.c_str(),filterString.c_str());
    }
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
      mvRecordQuery.print("INSERT INTO %s values (E'%s',E'%s',E'%s',E'%s')",lookupTableName.c_str(),pathString.c_str(),filterString.c_str(),dimString.c_str(),tableName.c_str());
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
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getTableNameForPathFilterAndDimension");
  #endif
  return tableName;
}

CDBStore::Store *CDBAdapterPostgreSQL::getDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getDimensionInfoForLayerTableAndLayerName");
  #endif
  CPGSQLDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return NULL;  }

  CT::string query;
  query.print("SELECT * FROM autoconfigure_dimensions where layerid=E'%s_%s'",layertable,layername);
  CDBStore::Store *store = dataBaseConnection->queryToStore(query.c_str());
  if(store==NULL){
    CDBDebug("No dimension info stored for %s",layername);
  }
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getDimensionInfoForLayerTableAndLayerName");
  #endif
  return store;
}


int CDBAdapterPostgreSQL::storeDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername,const char *netcdfname,const char *ogcname,const char *units){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::storeDimensionInfoForLayerTableAndLayerName");
  #endif
  CPGSQLDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }

  CT::string query;
  CT::string tableColumns("layerid varchar (255), ncname varchar (255), ogcname varchar (255), units varchar (255)");
  
  int status = dataBaseConnection->checkTable("autoconfigure_dimensions",tableColumns.c_str());
  if(status == 1){CDBError("\nFAIL: Table autoconfigure_dimensions could not be created: %s",tableColumns.c_str()); throw(__LINE__);  }
  
  query.print("INSERT INTO autoconfigure_dimensions values (E'%s_%s',E'%s',E'%s',E'%s')",layertable,layername,netcdfname,ogcname,units);
  status = dataBaseConnection->query(query.c_str()); 
  if(status!=0){
    CDBError("Unable to insert records: \"%s\"",query.c_str());
    throw(__LINE__); 
  }
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::storeDimensionInfoForLayerTableAndLayerName");
  #endif
  return 0;
}


int CDBAdapterPostgreSQL::removeDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::removeDimensionInfoForLayerTableAndLayerName");
  #endif
  CPGSQLDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  CT::string query;
  query.print("delete FROM autoconfigure_dimensions where layerid like E'%s%'",layertable);
  int status = dataBaseConnection->query(query.c_str()); 
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::removeDimensionInfoForLayerTableAndLayerName");
  #endif
  return status;
}


int CDBAdapterPostgreSQL::dropTable(const char *tablename){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::dropTable");
  #endif

  CPGSQLDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CT::string query;
  query.print("drop table %s",tablename);
  if(dataBaseConnection->query(query.c_str())!=0){
    CDBError("Query %s failed",query.c_str());
    return 1;
  }
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::dropTable");
  #endif
  return 0;
}

int CDBAdapterPostgreSQL::createDimTableOfType(const char *dimname,const char *tablename,int type){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::createDimTableOfType");
  #endif

  CPGSQLDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CT::string tableColumns("path varchar (511)");
  //12345678901234567890
  //0000-00-00T00:00:00Z
  if(type == 3)tableColumns.printconcat(", %s varchar (20), dim%s int",dimname,dimname);
  if(type == 2)tableColumns.printconcat(", %s varchar (64), dim%s int",dimname,dimname);
  if(type == 1)tableColumns.printconcat(", %s real, dim%s int",dimname,dimname);
  if(type == 0)tableColumns.printconcat(", %s int, dim%s int",dimname,dimname);
  tableColumns.printconcat(", filedate timestamp");
  
  // New since 2016-02-15 projection information and level
   tableColumns.printconcat(", adaguctilinglevel int");
   //tableColumns.printconcat(", crs varchar (511)");
   tableColumns.printconcat(", minx real, miny real, maxx real, maxy real");
   tableColumns.printconcat(", startx int, starty int, countx int, county int");
   
  
  tableColumns.printconcat(", PRIMARY KEY (path, %s)",dimname);
  
  //CDBDebug("tableColumns = %s",tableColumns.c_str());
  int status = dataBaseConnection->checkTable(tablename,tableColumns.c_str());
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::createDimTableOfType");
  #endif
  if (status == 2) {
    CDBDebug("New table created: Set indexes");
    CT::string query;
    int status = 0;
    /* Create index on dimension */
    query.print("CREATE INDEX idxdim%s on %s (%s)", tablename, tablename, dimname); 
    status = dataBaseConnection->query(query.c_str()); if(status!=0) { 
      CDBDebug("Warning: Unable to create index [%s]", query.c_str());
    }
    
//     /* Create index on filepath */
//     query.print("CREATE INDEX INDEXFOR%s on %s (%s)", "path", tablename, "path"); status = dataBaseConnection->query(query.c_str()); if(status!=0) { CDBError("Unable to create index [%s]", query.c_str());throw(__LINE__); }
//     
//     /* Create index on minx etc */
//     query.print("CREATE INDEX INDEXFOR%s on %s (%s)", "minx", tablename, "minx"); status = dataBaseConnection->query(query.c_str()); if(status!=0) { CDBError("Unable to create index [%s]", query.c_str());throw(__LINE__); }
//     query.print("CREATE INDEX INDEXFOR%s on %s (%s)", "miny", tablename, "miny"); status = dataBaseConnection->query(query.c_str()); if(status!=0) { CDBError("Unable to create index [%s]", query.c_str());throw(__LINE__); }
//     query.print("CREATE INDEX INDEXFOR%s on %s (%s)", "maxx", tablename, "maxx"); status = dataBaseConnection->query(query.c_str()); if(status!=0) { CDBError("Unable to create index [%s]", query.c_str());throw(__LINE__); }
//     query.print("CREATE INDEX INDEXFOR%s on %s (%s)", "maxy", tablename, "maxy"); status = dataBaseConnection->query(query.c_str()); if(status!=0) { CDBError("Unable to create index [%s]", query.c_str());throw(__LINE__); }
//     query.print("CREATE INDEX INDEXFOR%s on %s (%s)", "adaguctilinglevel", tablename, "adaguctilinglevel"); status = dataBaseConnection->query(query.c_str()); if(status!=0) { CDBError("Unable to create index [%s]", query.c_str());throw(__LINE__); }
    
  }
  return status;
  
}


int CDBAdapterPostgreSQL::createDimTableInt(const char *dimname,const char *tablename){
  return createDimTableOfType(dimname,tablename,0);
}

int CDBAdapterPostgreSQL::createDimTableReal(const char *dimname,const char *tablename){
  return createDimTableOfType(dimname,tablename,1);
}

int CDBAdapterPostgreSQL::createDimTableString(const char *dimname,const char *tablename){
  return createDimTableOfType(dimname,tablename,2);
}

int CDBAdapterPostgreSQL::createDimTableTimeStamp(const char *dimname,const char *tablename){
  return createDimTableOfType(dimname,tablename,3);
}

int CDBAdapterPostgreSQL::checkIfFileIsInTable(const char *tablename,const char *filename){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::checkIfFileIsInTable");
  #endif
  int fileIsOK = 1;
  CPGSQLDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CT::string query;
  query.print("select path from %s where path = '%s' limit 1",tablename,filename);
  CDBStore::Store *pathValues = dataBaseConnection->queryToStore(query.c_str());
  if(pathValues == NULL){CDBError("Query failed");throw(__LINE__);}
  if(pathValues->getSize()==1){fileIsOK=0;}else{fileIsOK=1;}
  delete pathValues;
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::checkIfFileIsInTable");
  #endif
  return fileIsOK;
}


int CDBAdapterPostgreSQL::removeFile(const char *tablename,const char *file){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::removeFile");
  #endif
  CPGSQLDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CT::string query;
  query.print("delete from %s where path = '%s'",tablename,file);
  int status = dataBaseConnection->query(query.c_str()); if(status!=0)throw(__LINE__);
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::removeFile");
  #endif
  return 0;
}

int CDBAdapterPostgreSQL::removeFilesWithChangedCreationDate(const char *tablename,const char *file,const char *creationDate){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::removeFilesWithChangedCreationDate");
  #endif
  CPGSQLDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CT::string query;
  query.print("delete from %s where path = '%s' and (filedate != '%s' or filedate is NULL)",tablename,file,creationDate);
  int status = dataBaseConnection->query(query.c_str()); if(status!=0){
    //CDBError("removeFilesWithChangedCreationDate exception");
    throw(__LINE__);
  }
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::removeFilesWithChangedCreationDate");
  #endif
  return 0;
}

int CDBAdapterPostgreSQL::setFileInt(const char *tablename,const char *file,int dimvalue,int dimindex,const char*filedate, GeoOptions *geoOptions){
  CT::string values;
  values.print("('%s',%d,'%d','%s','%d','%f','%f','%f','%f','%d','%d','%d','%d')",file,dimvalue,dimindex,filedate,
               geoOptions->level,geoOptions->bbox[0],geoOptions->bbox[1],geoOptions->bbox[2],geoOptions->bbox[3],
               geoOptions->indices[0],geoOptions->indices[1],geoOptions->indices[2],geoOptions->indices[3]);
  #ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("Adding INT %s",values.c_str());
  #endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}
int CDBAdapterPostgreSQL::setFileReal(const char *tablename,const char *file,double dimvalue,int dimindex,const char*filedate, GeoOptions *geoOptions){
  CT::string values;
  values.print("('%s',%f,'%d','%s','%d','%f','%f','%f','%f','%d','%d','%d','%d')",file,dimvalue,dimindex,filedate,
               geoOptions->level,geoOptions->bbox[0],geoOptions->bbox[1],geoOptions->bbox[2],geoOptions->bbox[3],
               geoOptions->indices[0],geoOptions->indices[1],geoOptions->indices[2],geoOptions->indices[3]);
  #ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("Adding REAL %s",values.c_str());
  #endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}
int CDBAdapterPostgreSQL::setFileString(const char *tablename,const char *file,const char * dimvalue,int dimindex,const char*filedate, GeoOptions *geoOptions){
  CT::string values;
  values.print("('%s','%s','%d','%s','%d','%f','%f','%f','%f','%d','%d','%d','%d')",file,dimvalue,dimindex,filedate,
               geoOptions->level,geoOptions->bbox[0],geoOptions->bbox[1],geoOptions->bbox[2],geoOptions->bbox[3],
               geoOptions->indices[0],geoOptions->indices[1],geoOptions->indices[2],geoOptions->indices[3]);
  #ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("Adding STRING %s",values.c_str());
  #endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}
int CDBAdapterPostgreSQL::setFileTimeStamp(const char *tablename,const char *file,const char *dimvalue,int dimindex,const char*filedate, GeoOptions *geoOptions){
  CT::string values;
  values.print("('%s','%s','%d','%s','%d','%f','%f','%f','%f','%d','%d','%d','%d')",file,dimvalue,dimindex,filedate,
               geoOptions->level,geoOptions->bbox[0],geoOptions->bbox[1],geoOptions->bbox[2],geoOptions->bbox[3],
               geoOptions->indices[0],geoOptions->indices[1],geoOptions->indices[2],geoOptions->indices[3]);
  #ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("Adding TIMESTAMP %s",values.c_str());
  #endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}
int CDBAdapterPostgreSQL::addFilesToDataBase(){
  #ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::addFilesToDataBase");
  #endif
  #ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("Adding files to database");
  #endif
  CPGSQLDB * dataBaseConnection = getDataBaseConnection(); if(dataBaseConnection == NULL){return -1;  }
  
  CT::string multiInsert = "";
  for (std::map<std::string,std::vector<std::string> >::iterator it=fileListPerTable.begin(); it!=fileListPerTable.end(); ++it){
      #ifdef CDBAdapterPostgreSQL_DEBUG
    CDBDebug("Updating table %s with %d records",it->first.c_str(),(it->second.size()));
#endif
    size_t maxIters = 50;
    if(it->second.size()>0){
      size_t rowNumber = 0;
      do{
        multiInsert.print("INSERT into %s VALUES ",it->first.c_str());
        for(size_t j=0;j<maxIters;j++){
          if(j>0)multiInsert.concat(",");
          multiInsert.concat(it->second[rowNumber].c_str());
          rowNumber++;
          if(rowNumber>=it->second.size())break;
        }
        // CDBDebug("Inserting %d bytes ",multiInsert.length());
        int status =  dataBaseConnection->query(multiInsert.c_str()); 
        if(status!=0){
          CDBError("Query failed [%s]:",dataBaseConnection->getError());
          throw(__LINE__);
        }
      }while(rowNumber<it->second.size());
    }
    it->second.clear();
  }
  #ifdef CDBAdapterPostgreSQL_DEBUG  
  CDBDebug("clearing arrays");
#endif
  fileListPerTable.clear();
  #ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::addFilesToDataBase");
  #endif
  return 0;
}

#endif
