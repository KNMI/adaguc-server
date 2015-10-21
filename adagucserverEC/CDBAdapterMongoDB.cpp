/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  MongoDB driver ADAGUC Server
 * Author:   Rob Tjalma, tjalma "at" knmi.nl
 * Date:     2015-09-18
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
#include "mongo/client/dbclient.h"
#include "mongo/bson/bson.h"
#include "CDBAdapterMongoDB.h"
#include <set>
#include "CDebugger.h"

/*
 * What to do next?
 * - Memory leak in getDatabaseConnection. Line 50.
 * 
 * 
 * Make MongoDriver work with DATASET key value pair.
 * 
 * Create config via:
 * 
 * ./adagucserver --getlayers --file /nobackup/users/plieger/projects/data/sdpkdc/RADNL_DATA/RADNL_OPER_R___25PCPRR_L3__20110617T152000_20110617T152500_0001.nc --datasetpath /nobackup/users/plieger/projects/data/sdpkdc/RADNL_DATA/ > /nobackup/users/plieger/datasetconfigs/urn_xkdc_dg_nl.knmi__default_testset_ADAGUC_NONE_1_.xml
 * 
 * Reference via URL with 
 * server.cgi?dataset=urn:xkdc:dg:nl.knmi::default_testset_ADAGUC_NONE/1/&&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=dnb&QUERY_LAYERS=dnb&CRS=EPSG%3A3857&BBOX=-4017299.426632504,429296.1946664017,5144326.288111853,6643017.105297432&WIDTH=1585&HEIGHT=1075&I=500&J=706&FORMAT=image/gif&INFO_FORMAT=text/html&STYLES=&&time=2015-10-21T02%3A57%3A45Z
 */

const char *CDBAdapterMongoDB::className="CDBAdapterMongoDB";

#define CDBAdapterMongoDB_DEBUG

CServerConfig::XMLE_Configuration *configurationObject;

/*
 * Getting the database connection. 
 * This includes connecting and authenticating, 
 * with the params from the config file.
 * 
 * @return the DBClientConnection object, containing the database connection. 
 *         If no connection can be established, return NULL.
 */
mongo::DBClientConnection *dataBaseConnection;

DEF_ERRORMAIN();


mongo::DBClientConnection *getDataBaseConnection(){
  //if(dataBaseConnection->isFailed()){
    std::string errorMessage;
    /* Connecting to the database. Only needed is host + port. */
    CDBDebug("Using MongoDB settings: %s",configurationObject->DataBase[0]->attr.parameters.c_str());
    dataBaseConnection = new mongo::DBClientConnection();
    dataBaseConnection->connect(configurationObject->DataBase[0]->attr.parameters.c_str(),errorMessage);
    
    /* Authenticating with dbname, username and password. */
    /*
    dataBaseConnection->auth(configurationObject->DataBase[0]->attr.parameters.c_str(),
      configurationObject->DataBase[1]->attr.parameters.c_str(),
      configurationObject->DataBase[2]->attr.parameters.c_str(),errorMessage); */
    if(!errorMessage.empty()){
      /* Something with className error. Commented for now. TODO */
      CDBError("Unable to connect to the MongoDB database: %s",errorMessage.c_str());
      return NULL;
    }
  //}
  return dataBaseConnection;
}

CDBAdapterMongoDB::CDBAdapterMongoDB(){
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("CDBAdapterMongoDB()");
  #endif  
    dataBaseConnection = NULL;
}

CDBAdapterMongoDB::~CDBAdapterMongoDB() {
  mongo::BSONObj info_logging_out;
  #ifdef CDBAdapterMongoDB_DEBUG  
    CDBDebug("~CDBAdapterMongoDB()");
  #endif  
  if(dataBaseConnection!=NULL){
    /* Don't know how to log out yet. TODO */
    //dataBaseConnection->logout(configurationObject->DataBase[0]->attr.parameters.c_str(),&info_logging_out);
  }
  dataBaseConnection = NULL;
}

CDBStore::Store *ptrToStore(auto_ptr<mongo::DBClientCursor> cursor) {
   /* Get the first one. So the max value. */
  mongo::BSONObj firstValue = cursor->next();
  
  /* Number of columns. */
  size_t numCols=firstValue.nFields();
  
  CDBStore::ColumnModel *colModel = new CDBStore::ColumnModel(numCols);
  
  /* Getting all the column names. */
  std::set<std::string> fieldNames;
  firstValue.getFieldNames(fieldNames);
  
  /* Filling all column names. */
  size_t colNumber = 0;
  for (std::set<std::string>::iterator i = fieldNames.begin(); i != fieldNames.end(); i++) {
    colModel->setColumn(colNumber,(*i).c_str());
    colNumber++;
  }
  
  /* Creating a store */
  CDBStore::Store *store=new CDBStore::Store(colModel);

  /* Reset the colNumber. */
  colNumber = 0;
  
  /* For the first record (which is already taken), push it directly first. */
  CDBStore::Record *record = new CDBStore::Record(colModel);
  for (std::set<std::string>::iterator i = fieldNames.begin(); i != fieldNames.end(); i++) {
    mongo::BSONElement fieldValue = firstValue.getField((*i));
    record->push(colNumber,fieldValue.String().c_str());
  }
  colNumber++; 
  store->push(record);
  
  /* Then the next values. */
  while(cursor->more()) {
    mongo::BSONObj nextValue = cursor->next();
    for (std::set<std::string>::iterator i = fieldNames.begin(); i != fieldNames.end(); i++) {
      mongo::BSONElement fieldValue = nextValue.getField((*i));
      record->push(colNumber,fieldValue.String().c_str());
      colNumber++;
    }
    store->push(record);
  }
  
  return store;
}

int CDBAdapterMongoDB::setConfig(CServerConfig::XMLE_Configuration *cfg) {
  configurationObject = cfg;
  return 0;
}

CDBStore::Store *CDBAdapterMongoDB::getReferenceTime(const char *netcdfDimName,const char *netcdfTimeDimName,const char *timeValue,const char *timeTableName,const char *tableName) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }
  
  /*
  mongo::BSONObjBuilder query;
  
  query << netcdfDimName << 1 << "ncname" << netcdfname << "ogcname" << ogcname << "units" << units;
  mongo::BSONObj objBSON = query.obj();
  */
  CT::string query;
  /*
  query.print(
          "select * from (select %s,(EXTRACT(EPOCH FROM (%s-%s))) as age from ( select * from %s)a0 ,( select * from %s where %s = '%s')a1 where a0.path = a1.path order by age asc)a0 where age >= 0 limit 1",
                    netcdfDimName,
                    netcdfTimeDimName,
                    netcdfDimName,
                    tableName,
                    timeTableName,
                    netcdfTimeDimName,
                    timeValue
                    );
  return DB->queryToStore(query.c_str())  ; */
  return NULL;
}

CDBStore::Store *CDBAdapterMongoDB::getClosestDataTimeToSystemTime(const char *netcdfDimName,const char *tableName) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }
  
  mongo::BSONObjBuilder querySelect;
  querySelect << netcdfDimName << 1;
  mongo::BSONObj objBSON = querySelect.obj();
  
  /* To build: EXTRACT EPOCH FROM. */
  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  cursorFromMongoDB = DB->query(tableName,mongo::Query(objBSON).sort("",1), 1);
  
  //query.print("SELECT %s,abs(EXTRACT(EPOCH FROM (%s - now()))) as t from %s order by t asc limit 1",netcdfDimName,netcdfDimName,tableName);
  return ptrToStore(cursorFromMongoDB);
}

CT::string CDBAdapterMongoDB::getTableNameForPathFilterAndDimension(const char *path,const char *filter, const char * dimension,CDataSource *dataSource) {
  if(dataSource->cfgLayer->DataBaseTable.size() == 1) {
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
  mongo::BSONObjBuilder mvRecordQuery;
  int status = 0;
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    throw(1);
  }
  
  try {
    //status = checkTableMongo(lookupTableName.c_str(),tableColumns.c_str());
    #ifdef CDBAdapterMongoDB_DEBUG        
      if(status == 0) {
		CDBDebug("OK: Table %s is available",lookupTableName.c_str());
	  }
    #endif    
    if(status == 1){
      CDBError("FAIL: Table %s could not be created",lookupTableName.c_str());
      CDBError("Error: %s", DB->getLastError().c_str());    
      throw(1);  
    }
    #ifdef CDBAdapterMongoDB_DEBUG        
      if(status == 2) {
		CDBDebug("OK: Table %s is created",lookupTableName.c_str());
	  }
    #endif    
    
    //Check wether a records exists with this path and filter combination.
    bool lookupTableIsAvailable = false;
    
    mvRecordQuery << "path" << pathString.c_str() << "filter" << filterString.c_str();
    if(dimString.length() > 1){
      mvRecordQuery << "dimension" << dimString.c_str();
    }   
  
    /* MongoDB uses auto_ptr for getting all records. */
    auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
    mongo::BSONObj recordQuery = mvRecordQuery.obj();
    cursorFromMongoDB = DB->query(lookupTableName.c_str(),recordQuery);

    CDBStore::Store *rec = ptrToStore(cursorFromMongoDB); 

    if(rec==NULL) {
		CDBError("Unable to select records");
		throw(1);
	}
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
      mongo::BSONObjBuilder queryToInsert;
      queryToInsert << "path" << pathString.c_str() << "filter" << filterString.c_str() << "dimension" << dimString.c_str() << "tablename" << tableName.c_str();
      mongo::BSONObj objBSONInsert = queryToInsert.obj();
      DB->insert(lookupTableName.c_str(), objBSONInsert);
      
      /* Check if insert succeeded. */
      auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
      mongo::BSONObjBuilder queryCheck;
      queryCheck << "path" << pathString.c_str() << "filter" << filterString.c_str();
      mongo::BSONObj objBSONCheck = queryCheck.obj();
  
      cursorFromMongoDB = DB->query(lookupTableName.c_str(),objBSONCheck);
  
      /* If no records are found.. */
      if(!cursorFromMongoDB->more()){
		mongo::BSONObj recordBSON = mvRecordQuery.obj();
		CDBError("Unable to insert records: \"%s\"",recordBSON.toString().c_str());
		throw(1);
	  }
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

int CDBAdapterMongoDB::autoUpdateAndScanDimensionTables(CDataSource *dataSource) {
  CServerParams *srvParams = dataSource->srvParams;;
  CServerConfig::XMLE_Layer * cfgLayer = dataSource->cfgLayer;
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return -1;
  }
  
  CCache::Lock lock;
  CT::string identifier = "checkDimTables";  identifier.concat(cfgLayer->FilePath[0]->value.c_str());  identifier.concat("/");  identifier.concat(cfgLayer->FilePath[0]->attr.filter.c_str());  
  CT::string cacheDirectory = srvParams->cfg->TempDir[0]->attr.value.c_str();
  //srvParams->getCacheDirectory(&cacheDirectory);
  if(cacheDirectory.length() > 0){
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
    
    
    mongo::BSONObjBuilder queryForSelecting;
    queryForSelecting << "path" << 1 << "filedate" << 1 << dimName.c_str() << 1;
    mongo::BSONObj objBSON = queryForSelecting.obj();
    
    auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
    cursorFromMongoDB = DB->query(tableName.c_str(),mongo::Query(objBSON),1);
    CDBStore::Store *store = ptrToStore(cursorFromMongoDB);
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
      int status = CDBFileScanner::updatedb(srvParams->cfg->DataBase[0]->attr.parameters.c_str(),dataSource,NULL,NULL);
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
  return 0;
}

CDBStore::Store *CDBAdapterMongoDB::getMin(const char *name,const char *table) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }
  /* Executing the query and sort by name, in ascending order. */
  mongo::Query query = mongo::Query().sort(name,1);
  
  /* MongoDB uses auto_ptr for getting all records. */
  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  cursorFromMongoDB = DB->query(table,query);

  CDBStore::Store *minStore = ptrToStore(cursorFromMongoDB);
  
  if(minStore == NULL){
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for  %s",name);
    CDBError("query failed"); 
    return NULL;
  }
  return minStore; 
}

CDBStore::Store *CDBAdapterMongoDB::getMax(const char *name,const char *table) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }
  /* Executing the query and sort by name, in ascending order. */
  mongo::Query query = mongo::Query().sort(name,-1);
  
  /* MongoDB uses auto_ptr for getting all records. */
  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  cursorFromMongoDB = DB->query(table,query);
  
  CDBStore::Store *maxStore = ptrToStore(cursorFromMongoDB);
  
  if(maxStore == NULL){
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for  %s",name);
    CDBError("query failed"); 
    return NULL;
  }
  return maxStore; 
}

CDBStore::Store *CDBAdapterMongoDB::getUniqueValuesOrderedByValue(const char *name, int limit, bool orderDescOrAsc,const char *table) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }
  /* TODO Not distinct yet! */
  mongo::Query query = mongo::Query().sort(name, orderDescOrAsc ? 1 : -1);

  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  
  if(limit>0) {
    cursorFromMongoDB = DB->query(table, query, limit);
  } else {
    cursorFromMongoDB = DB->query(table, query);
  }
  
  CDBStore::Store *store = ptrToStore(cursorFromMongoDB);
  
  if(store == NULL){
    CDBDebug("Query %s failed",query.toString().c_str());
  }
  return store;
}

CDBStore::Store *CDBAdapterMongoDB::getUniqueValuesOrderedByIndex(const char *name, int limit, bool orderDescOrAsc,const char *table) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }
  std::string buff("dim");
  buff.append(name);
  
  mongo::BSONObjBuilder querySelect;
  querySelect << buff << 1 << name << 1;
  mongo::BSONObj objBSON = querySelect.obj();
  
  /* Sorting by two fields. */
  mongo::BSONObjBuilder sortQuery;
  sortQuery << buff << 1 << name << 1;
  mongo::BSONObj sortBSON = sortQuery.obj();
  
  mongo::Query query = mongo::Query(objBSON).sort(sortBSON);

  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  
  if(limit>0) {
    cursorFromMongoDB = DB->query(table, query, limit);
  } else {
    cursorFromMongoDB = DB->query(table, query);
  }
 
  CDBStore::Store *store = ptrToStore(cursorFromMongoDB);
  if(store==NULL){
    CDBDebug("Query failed!");
  }
  
  return store;
}

CDBStore::Store *CDBAdapterMongoDB::getFilesAndIndicesForDimensions(CDataSource *dataSource,int limit) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }

  return NULL;
}

CDBStore::Store *CDBAdapterMongoDB::getFilesForIndices(CDataSource *dataSource,size_t *start,size_t *count,ptrdiff_t *stride,int limit) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }
  return NULL; 
}
    
CDBStore::Store *CDBAdapterMongoDB::getDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }
  
  std::string buff("E'");
  buff.append(layertable);
  buff.append("_");
  buff.append(layername);
  
  mongo::BSONObjBuilder query;
  query << "layerid" << buff;
  mongo::BSONObj objBSON = query.obj();
  
  /* MongoDB uses auto_ptr for getting all records. */
  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  cursorFromMongoDB = DB->query("database.autoconfigure_dimensions",mongo::Query(objBSON));
  
  CDBStore::Store *store = ptrToStore(cursorFromMongoDB);
  if(store==NULL){
    CDBDebug("No dimension info stored for %s",layername);
  }
  return store;
}

int CDBAdapterMongoDB::storeDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername,const char *netcdfname,const char *ogcname,const char *units) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return -1;
  }
  /* Check if collection already exists. */
  if(DB->exists("autoconfigure_dimensions")) {
	/* Returns true if succesful. */
	bool status = DB->createCollection("autoconfigure_dimensions");
  
	if(!status) {
	  CDBError("\nFAIL: Table autoconfigure_dimensions could not be created"); 
	  throw(__LINE__);  
	}
  } 
  /* layerid becomes layertable + _ + layername. */
  mongo::BSONObjBuilder query;
  std::string buff(layertable);
  buff.append("_");
  buff.append(layername);
  
  query << "layerid" << buff << "ncname" << netcdfname << "ogcname" << ogcname << "units" << units;
  mongo::BSONObj objBSON = query.obj();
  
  DB->insert("autoconfigure_dimensions", objBSON);
  
  /* Query to see if record was succesfuly added. */
  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  
  mongo::BSONObjBuilder queryCheck;
  queryCheck << "layerid" << buff;
  mongo::BSONObj objBSONCheck = queryCheck.obj();
  
  cursorFromMongoDB = DB->query("autoconfigure_dimensions",objBSONCheck);
  
  /* If no records are found.. */
  if(!cursorFromMongoDB->more()){
    CDBError("Unable to insert records");
    throw(__LINE__); 
  }
  return 0;
}
    
int CDBAdapterMongoDB::dropTable(const char *tablename) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return -1;
  }
  bool status = false;
  status = DB->dropCollection(tablename);
  if(!status) {
    CDBError("Dropping the table failed");
    return 1;
  }
  return 0; 
}

int CDBAdapterMongoDB::createDimTableOfType(const char *dimname,const char *tablename,int type) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return -1;
  }
  
  CT::string tableColumns("path varchar (511)");
   
  if(type == 3)tableColumns.printconcat(", %s timestamp, dim%s int",dimname,dimname);
  if(type == 2)tableColumns.printconcat(", %s varchar (64), dim%s int",dimname,dimname);
  if(type == 1)tableColumns.printconcat(", %s real, dim%s int",dimname,dimname);
  if(type == 0)tableColumns.printconcat(", %s int, dim%s int",dimname,dimname);
  
  tableColumns.printconcat(", filedate timestamp");
  tableColumns.printconcat(", PRIMARY KEY (path, %s)",dimname);
  
  int status = 0;
  
  //int status = checkTable(tablename,tableColumns.c_str());
  return status;
}

int CDBAdapterMongoDB::createDimTableInt(const char *dimname,const char *tablename) {
  return createDimTableOfType(dimname,tablename,0);
}

int CDBAdapterMongoDB::createDimTableReal(const char *dimname,const char *tablename) {
  return createDimTableOfType(dimname,tablename,1);
}

int CDBAdapterMongoDB::createDimTableString(const char *dimname,const char *tablename) {
  return createDimTableOfType(dimname,tablename,2);
}

int CDBAdapterMongoDB::createDimTableTimeStamp(const char *dimname,const char *tablename) {
  return createDimTableOfType(dimname,tablename,3);
}

int CDBAdapterMongoDB::checkIfFileIsInTable(const char *tablename,const char *filename) {
  int fileIsOK = 1;
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return -1;
  }
  
  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  
  mongo::BSONObjBuilder query;
  query << "fileName" << filename;
  mongo::BSONObj objBSON = query.obj();
  
  /* Third parameter is number of results. */
  cursorFromMongoDB = DB->query(tablename,objBSON,1);
  
  /* Trying to get a store with a record. */
  CDBStore::Store *store = ptrToStore(cursorFromMongoDB);
  if(store == NULL) {
    CDBError("Query failed");
    throw(__LINE__);
  }
  if(store->getSize()==1) {
    fileIsOK=0;
  }else {
    fileIsOK=1;
  }
  
  delete store;
  return fileIsOK; 
}
    
int CDBAdapterMongoDB::removeFile(const char *tablename,const char *file) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return -1;
  }
  /* Status will never change. Remove function is a void. 
   * Also, no param pointers are being used, so no feedback. */
  int status = 0;
  mongo::BSONObjBuilder query;
  query << "fileName" << file;
  mongo::BSONObj objBSON = query.obj();
  /* Third param is true, this means after one record has been found, it stops. */
  DB->remove(tablename,mongo::Query(objBSON),true);
  
  if(status != 0) {
    throw(__LINE__);
  }
  return status;
}

int CDBAdapterMongoDB::removeFilesWithChangedCreationDate(const char *tablename,const char *file,const char *creationDate) {
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return -1;
  }
  
  mongo::BSONObjBuilder query;
  query << "fileName" << file << "fileDate" << mongo::NE << creationDate;
  mongo::BSONObj objBSON = query.obj();
  
  DB->remove(tablename,mongo::Query(objBSON),true); 
  if(DB->getLastError() == "") {
    throw(__LINE__);
  }
  return 0; 
}

int CDBAdapterMongoDB::setFileInt(const char *tablename,const char *file,int dimvalue,int dimindex,const char*filedate) {
  CT::string values;
  values.print("('%s',%d,'%d','%s')",file,dimvalue,dimindex,filedate);
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("Adding INT %s",values.c_str());
  #endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}

int CDBAdapterMongoDB::setFileReal(const char *tablename,const char *file,double dimvalue,int dimindex,const char*filedate) {
  CT::string values;
  values.print("('%s',%f,'%d','%s')",file,dimvalue,dimindex,filedate);
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("Adding REAL %s",values.c_str());
  #endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}

int CDBAdapterMongoDB::setFileString(const char *tablename,const char *file,const char * dimvalue,int dimindex,const char*filedate) {
  CT::string values;
  values.print("('%s','%s','%d','%s')",file,dimvalue,dimindex,filedate);
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("Adding STRING %s",values.c_str());
  #endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}

int CDBAdapterMongoDB::setFileTimeStamp(const char *tablename,const char *file,const char *dimvalue,int dimindex,const char*filedate) {
  CT::string values;
  values.print("('%s','%s','%d','%s')",file,dimvalue,dimindex,filedate);
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug(" Adding TIMESTAMP %s",values.c_str());
  #endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}

int CDBAdapterMongoDB::addFilesToDataBase() {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("Adding files to database");
  #endif
    
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return -1;
  }
  
  mongo::BSONObjBuilder multiInsert;
  for (std::map<std::string,std::vector<std::string> >::iterator it=fileListPerTable.begin(); it!=fileListPerTable.end(); ++it){
    CDBDebug("Updating table %s with %d records",it->first.c_str(),(it->second.size()));
    if(it->second.size()>0){
      CT::string tableToUpdate = it->first.c_str();
      
      /*
      multiInsert.print("INSERT into %s VALUES ",it->first.c_str());
      for(size_t j=0;j<it->second.size();j++){
        if(j>0)multiInsert.concat(",");
        multiInsert.concat(it->second[j].c_str());
      } */
      
      mongo::BSONObj objBSONInsert = multiInsert.obj();
      DB->insert(tableToUpdate.c_str(), objBSONInsert);
      
      //if(status!=0){
        //CDBError("Query failed [%s]:",dataBaseConnection->getError());
        //throw(__LINE__);
      //}
      CDBDebug("Inserting complete!");
    }
    it->second.clear();
  }
  
  CDBDebug("clearing arrays");
  fileListPerTable.clear(); 
  return 0;
}
