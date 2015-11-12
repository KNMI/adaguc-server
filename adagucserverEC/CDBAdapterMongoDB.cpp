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
#include <boost/algorithm/string.hpp>
#include "CDBAdapterMongoDB.h"
#include <set>
#include "CDebugger.h"

const char *CDBAdapterMongoDB::className="CDBAdapterMongoDB";

#define CDBAdapterMongoDB_DEBUG

CServerConfig::XMLE_Configuration *configurationObject;

/* Used to discover the sorting of the MongoDB queries. */
std::map<std::string,std::string> table_combi;

/* To remember the current used dimension, define a global variable. */
const char* current_used_dimension;

DEF_ERRORMAIN();

/*
 * Getting the database connection. 
 * This includes connecting and authenticating, 
 * with the params from the config file.
 * 
 * @return the DBClientConnection object, containing the database connection. 
 *         If no connection can be established, return NULL.
 */
mongo::DBClientConnection *dataBaseConnection;

mongo::DBClientConnection *getDataBaseConnection(){
  if(dataBaseConnection == NULL) {
    std::string errorMessage;
    /* Connecting to the database. Only needed is host + port. */
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
  }
  return dataBaseConnection;
}

int checkTableMongo(const char * pszTableName,const char *pszColumns){
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: checkTableMongo]");
  #endif
  // returncodes:
  // 0 = no change
  // 1 = error
  // 2 = table created
  
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    CDBError("checkTable: Not connected to DB");
    return 1;
  }
  /* Collection name needs <database-name>. in front of it. */
  std::string collection_in_mongo = "database.datagranules";
  
  /* Getting the record of the given granule. */
  mongo::BSONObjBuilder selecting_query;
  selecting_query << "fileName" << pszTableName;
  mongo::BSONObj the_query = selecting_query.obj();
  
  mongo::BSONObjBuilder specific_fields;
  specific_fields << "adaguc" << 1 << "_id" << 0;
  mongo::BSONObj fields_specifically = specific_fields.obj();
  
  /* Getting the result. */
  auto_ptr<mongo::DBClientCursor> ptr_to_mongodb = DB->query(collection_in_mongo, mongo::Query(the_query), 0, 0, &fields_specifically);
  
  /* Getting the BSON object of the record. */
  mongo::BSONObj record = ptr_to_mongodb->next().getObjectField("adaguc");
  
  /* There must be returned exactly 1 field ( path ), otherwise they don't exist. */
  if(record.nFields() != 1) {
    
    CDBDebug("The granule with fileName %s contains no adaguc field.", pszTableName);
    CDBError("Error: No adaguc field in granule with fileName %s.", pszTableName);
  
    /*
     * If there is no adaguc field in MongoDB, create the whole package.
     * Just with empty values, these will be added in addFilesToDataBase.
     */
    /* If not existing, create them! */
    //mongo::BSONObjBuilder update_query;
    //update_query << "$set" << BSON("adaguc" << BSON("path" << "" << "dimension" << BSON("time" << BSON("time" << "" << "dimtime" << BSON_ARRAY(0 << 1) << "filedate" << ""))));
    //mongo::BSONObj update_query_bson = update_query.obj();
  
    /* Update the right granule. */
    //DB->update("database.datagranules", mongo::Query(the_query), update_query_bson);
  

    //auto_ptr<mongo::DBClientCursor> ptr_for_check = DB->query(collection_in_mongo, mongo::Query(the_query), 0, 0, &fields_specifically);
    //if(ptr_for_check->more()){
      //if(ptr_for_check->next().getObjectField("adaguc").nFields() !=2) {
        /* If query not succeeded. */
  	    //return 1;
      //} else {
	    /* If it did succeed.. */
        //return 2;
      //}
    //}
    return 1;
  }
  
  return 0;
}

CDBAdapterMongoDB::CDBAdapterMongoDB(){
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("CDBAdapterMongoDB()");
  #endif  
  table_combi.insert(std::make_pair("pathfiltertablelookup","path,filter,dimension,tablename"));
  table_combi.insert(std::make_pair("autoconfigure_dimensions","layer,ncname,ogcname,units"));
  table_combi.insert(std::make_pair("all_other","path,time,dimtime,filedate"));
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
  delete dataBaseConnection;
}

const char* CDBAdapterMongoDB::getCorrectedColumnName(const char* column_name) {
	std::string prefix;
	if(strcmp(column_name,"time") == 0) {
	  prefix = "adaguc.";
	} else if(strcmp(column_name,"dimtime") == 0) {
	  prefix = "adaguc.dimension.time.";
	}else {
	  prefix = "adaguc.";
    }
    
	prefix.append(column_name);
	return prefix.c_str();
}


bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

/*
 *  Converting the query to a CDBStore, compatible with ADAGUC.
 * 
 *  @param 		DBClientCursor 		the cursor with a pointer to the query result.
 *  @param 		const char* 		Used for having the field names in the correct order.
 * 	@return		CDBStore::Store		The store containing the results.
 */
CDBStore::Store *ptrToStore(auto_ptr<mongo::DBClientCursor> cursor, const char* table) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: ptrToStore]");
  #endif
  
  /* Prework. */
  std::string delimiter = ",";
  /* Fieldsnames cannot be collected from the query unfortunately. So we use 
   * static fields. */
  std::string usedColumns = table;
  std::string the_file_name_of_the_granule;
  std::string object_field_return;
  
  /* Variable used for determine if <filename>_<layername> is being used. */
  bool using_extended_layerid = false;
  
  /* Number of columns that are being used. */
  size_t colNumber = 0;
  
  /* If the pointer returns no records, return an empty store. */
  if(!cursor->more()) {
	CDBDebug("There are no records, so returning an empty store!");
	CDBStore::ColumnModel *colModel = new CDBStore::ColumnModel(4);
	
	for(size_t j=0; j<colNumber;j++) {
	  std::string cName = usedColumns.substr(0,usedColumns.find(delimiter));
	  usedColumns.erase(0,usedColumns.find(delimiter) + 1);
      colModel->setColumn(colNumber,cName.c_str());
      colNumber++;
    }
    
	CDBStore::Store *store=new CDBStore::Store(colModel);
    return store;
  }
    
   /* Get the first one. Get directly the adaguc object. */
  mongo::BSONObj firstValue = cursor->next();
  
  /* Only applicable if table columns are equal to these. */
  if(strcmp(table,"layer,ncname,ogcname,units") == 0) {
    using_extended_layerid = true;
    /* Instead of tablelayer + layername, for MongoDB we use fileName + _ + layername. */
    the_file_name_of_the_granule = firstValue.getStringField("fileName");
    the_file_name_of_the_granule.append("_");
    }else if(strcmp(table,"time,") == 0) {
	object_field_return = "dimension.time";
    }
  
  /* Number of columns. */
  size_t numCols = 0;
  if(strcmp(object_field_return.c_str(),"dimension.time") == 0) {
    numCols = firstValue.nFields();
  } else if(strcmp(table, "path,time,dimtime") == 0) {
	numCols = 3;
  } else if(strcmp(table, "layer,ncname,ogcname,units") == 0) {
	numCols = 4;
  } else if(strcmp(table, "time,") == 0) {
	numCols = 1;
  } else {
    numCols = firstValue.nFields();
  } 
    
  CDBStore::ColumnModel *colModel = new CDBStore::ColumnModel(numCols);
  
  /* Making a copy of the used columns. */
  usedColumns = table;
  
  /* Filling all column names. */
  colNumber = 0;
  for(size_t j = 0; j < numCols; j++) {
	std::string cName = usedColumns.substr(0,usedColumns.find(delimiter));
	usedColumns.erase(0,usedColumns.find(delimiter) + 1);
	/* If true, it means the first column is layer. Make it layerid, so adaguc knows the column. */
	if(j == 0 && true == using_extended_layerid) { cName.append("id"); }
    colModel->setColumn(colNumber,cName.c_str());
    colNumber++;
  }
  
  /* Creating a store */
  CDBStore::Store *store=new CDBStore::Store(colModel);
  
  usedColumns = table;
  
  /* Reset the colNumber. */
  colNumber = 0;
  
  /* Using a vector for future purposes ( aggregations ). */
  std::vector<mongo::BSONElement> vector_with_dimension_values;
  std::vector<mongo::BSONElement>::iterator it;
    
  /* For the first record (which is already taken), push it directly first. */
  CDBStore::Record *record = new CDBStore::Record(colModel);
  for(size_t j = 0; j < numCols; j++) {
      std::string cName = usedColumns.substr(0,usedColumns.find(delimiter));
      usedColumns.erase(0,usedColumns.find(delimiter) + 1);
      std::string fieldValue;
      // If time or dimtime is being used:
      if(strcmp(cName.c_str(),"time") == 0) {
	vector_with_dimension_values = firstValue.getObjectField("adaguc").getField(cName.c_str()).Array();
	it = vector_with_dimension_values.begin();
	fieldValue = (*it).String(); ++it;
      } else if(strcmp(cName.c_str(),"dimtime") == 0) {
	fieldValue = "0";
      } else if(strcmp(cName.c_str(), "ncname") == 0) {
	fieldValue = current_used_dimension;
      } else if(strcmp(cName.c_str(), "ogcname") == 0) {
	fieldValue = firstValue.getObjectField("adaguc").getObjectField("layer").getObjectField(configurationObject->Layer[0]->Variable[0]->value.c_str()).getObjectField("dimension").getObjectField(current_used_dimension).getStringField("ogcname");
      } else if(strcmp(cName.c_str(), "layer") == 0) {
	fieldValue = configurationObject->Layer[0]->Variable[0]->value.c_str();
      }else {
	fieldValue = firstValue.getObjectField("adaguc").getStringField(cName.c_str());
      }

      if(j == 0 && true == using_extended_layerid) {
	fieldValue = the_file_name_of_the_granule.append(fieldValue); 
      }
      CDBDebug("What goes in: %s", fieldValue.c_str());
      record->push(colNumber,fieldValue.c_str());
      colNumber++;
    } 
  /* And push it! */
  store->push(record);
  
  /* Reset the column number. */
  colNumber = 0;
  
  /* Then the next values. */
  while(cursor->more()) {
    usedColumns = table;
    mongo::BSONObj nextValue = cursor->next();
    CDBStore::Record *record = new CDBStore::Record(colModel);
    for(size_t j = 0; j < numCols; j++) {
      std::string cName = usedColumns.substr(0,usedColumns.find(delimiter));
      usedColumns.erase(0,usedColumns.find(delimiter) + 1);
      std::string fieldValue;
      // If time or dimtime is being used:
      if(strcmp(cName.c_str(),"time") == 0) {
	vector_with_dimension_values = nextValue.getObjectField("adaguc").getField(cName.c_str()).Array();
	it = vector_with_dimension_values.begin();
	fieldValue = (*it).String(); ++it;
      } else if(strcmp(cName.c_str(),"dimtime") == 0) {
	fieldValue = "0";
      } else if(strcmp(cName.c_str(), "ncname") == 0) {
	fieldValue = current_used_dimension;
      } else if(strcmp(cName.c_str(), "ogcname") == 0) {
	fieldValue = nextValue.getObjectField("adaguc").getObjectField("layer").getObjectField(configurationObject->Layer[0]->Variable[0]->value.c_str()).getObjectField("dimension").getObjectField(current_used_dimension).getStringField("ogcname");
      } else if(strcmp(cName.c_str(), "layer") == 0) {
	fieldValue = configurationObject->Layer[0]->Variable[0]->value.c_str();
      }else {
	fieldValue = nextValue.getObjectField("adaguc").getStringField(cName.c_str());
      }

      if(j == 0 && true == using_extended_layerid) {
	fieldValue = the_file_name_of_the_granule.append(fieldValue); 
      }
      CDBDebug("What goes in: %s", fieldValue.c_str());
      record->push(colNumber,fieldValue.c_str());
      colNumber++;
    }
    store->push(record);
    colNumber = 0;
  }
  
  return store;
}

int CDBAdapterMongoDB::setConfig(CServerConfig::XMLE_Configuration *cfg) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: setConfig]");
  #endif
  configurationObject = cfg;
  return 0;
}

/* Query needs to be defined. */
CDBStore::Store *CDBAdapterMongoDB::getReferenceTime(const char *netcdfDimName,const char *netcdfTimeDimName,const char *timeValue,const char *timeTableName,const char *tableName) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: getReferenceTime]");
  #endif
  return NULL;
}

/* Query needs to be defined. */
CDBStore::Store *CDBAdapterMongoDB::getClosestDataTimeToSystemTime(const char *netcdfDimName,const char *tableName) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: getClosestDataTimeToSystemTime]");
  #endif
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }
  
  /*
  mongo::BSONObjBuilder querySelect;
  querySelect << netcdfDimName << 1;
  mongo::BSONObj objBSON = querySelect.obj();
  
  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  cursorFromMongoDB = DB->query(tableName,mongo::Query(objBSON).sort("",1), 1, 0);
  
  DATENOW
  //query.print("SELECT %s,abs(EXTRACT(EPOCH FROM (%s - now()))) as t from %s order by t asc limit 1",netcdfDimName,netcdfDimName,tableName);
  return ptrToStore(cursorFromMongoDB, tableName.c_str());
  */
  
  return NULL;
}

CT::string CDBAdapterMongoDB::getTableNameForPathFilterAndDimension(const char *path,const char *filter, const char * dimension,CDataSource *dataSource) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: getTableNameForPathFilterAndDimension]");
    CDBDebug("path = %s", path);
  #endif
  
  CT::string the_path = path;
  
  /* Formatting the path, so the last part is only used. */
  if(!hasEnding(path, "/")) {
    CT::string delimiter = "/";
    the_path.substringSelf(&the_path,(size_t)the_path.lastIndexOf("/") + 1,the_path.length());
  }
  
  /* The path is the fileName of the granule in the MongoDB database. */
  return the_path;
}

int CDBAdapterMongoDB::autoUpdateAndScanDimensionTables(CDataSource *dataSource) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: autoUpdateAndScanDimensionTables]");
  #endif
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
  
  #ifdef CDBAdapterMongoDB_DEBUG
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
    
    mongo::BSONObjBuilder queryForSelecting;
    queryForSelecting << "adaguc.path" << 1 << "adaguc.filedate" << 1 << dimName.c_str() << 1 << "_id" << 0;
    mongo::BSONObj objBSON = queryForSelecting.obj();
    
    mongo::BSONObjBuilder query_builder;
    if(!hasEnding(tableName.c_str(), "/")) {
      query_builder << "fileName" << tableName.c_str();
    } else {
      query_builder << "adaguc.dataSetPath" << tableName.c_str(); 
    }
    mongo::BSONObj the_query = query_builder.obj();
    
    auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
    
    cursorFromMongoDB = DB->query("database.datagranules",mongo::Query(the_query),1, 0, &objBSON);
    CDBStore::Store *store = ptrToStore(cursorFromMongoDB, table_combi.find("pathfiltertablelookup")->second.c_str());
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
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[/checkDimTables]");
  #endif
  lock.release();
  return 0;
}

CDBStore::Store *CDBAdapterMongoDB::getMin(const char *name,const char *table) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: getMin]");
  #endif
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }
  
  /* Get the correct MongoDB path. */
  std::string used_name = getCorrectedColumnName(name);
  
  mongo::BSONObjBuilder query_builder;
  if(!hasEnding(table, "/")) {
    query_builder << "fileName" << table;
  } else {
    query_builder << "adaguc.dataSetPath" << table; 
  }
  mongo::BSONObj query_object = query_builder.obj();
  
  /* Executing the query and sort by name, in ascending order. */
  mongo::Query query = mongo::Query(query_object).sort(used_name,1);
  
  mongo::BSONObjBuilder selecting_builder;
  selecting_builder << used_name.c_str() << 1 << "_id" << 0;
  mongo::BSONObj selecting_query = selecting_builder.obj();
  
  /* MongoDB uses auto_ptr for getting all records. */
  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  cursorFromMongoDB = DB->query("database.datagranules",query, 1, 0, &selecting_query);
  
  std::string buff = name;
  buff.append(",");
  CDBStore::Store *minStore = ptrToStore(cursorFromMongoDB, buff.c_str());
  
  if(minStore == NULL){
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for  %s",name);
    CDBError("query failed"); 
    return NULL;
  }
  return minStore;
}

CDBStore::Store *CDBAdapterMongoDB::getMax(const char *name,const char *table) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: getMax]");
    CDBDebug("Name is %s, fileName is %s", name, table);
  #endif
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }
  /* Get the correct MongoDB path. */
  std::string used_name = getCorrectedColumnName(name);
  
  mongo::BSONObjBuilder query_builder;
  if(!hasEnding(table, "/")) {
    query_builder << "fileName" << table;
  } else {
    query_builder << "adaguc.dataSetPath" << table; 
  }
  mongo::BSONObj query_object = query_builder.obj();
  
  /* Executing the query and sort by name, in ascending order. */
  mongo::Query query = mongo::Query(query_object).sort(used_name,-1);
  
  mongo::BSONObjBuilder selecting_builder;
  selecting_builder << used_name.c_str() << 1 << "_id" << 0;
  mongo::BSONObj selecting_query = selecting_builder.obj();
  
  /* MongoDB uses auto_ptr for getting all records. */
  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  cursorFromMongoDB = DB->query("database.datagranules",query, 1, 0, &selecting_query);
  
  std::string buff = name;
  buff.append(",");
  CDBStore::Store *maxStore = ptrToStore(cursorFromMongoDB, buff.c_str());
  
  if(maxStore == NULL){
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for  %s",name);
    CDBError("query failed"); 
    return NULL;
  }
  return maxStore; 
}

CDBStore::Store *CDBAdapterMongoDB::getUniqueValuesOrderedByValue(const char *name, int limit, bool orderDescOrAsc,const char *table) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: getUniqueValuesOrderedByValue]");
    CDBDebug("name = %s, table = %s", name, table);
  #endif
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }

  /* The corrected name. Because of MongoDB, it can be that  */
  const char* corrected_name = getCorrectedColumnName(name);
  
  /* What do we want to select? Only the name variable. */
  mongo::BSONObjBuilder queryBSON;
  queryBSON << corrected_name << 1 << "_id" << 0;
  mongo::BSONObj queryObj = queryBSON.obj();
  
  /* The query itself. */
  
  mongo::BSONObjBuilder query_itself;
  if(!hasEnding(table, "/")) {
    query_itself << "fileName" << table;
  } else {
    query_itself << "adaguc.dataSetPath" << table; 
  }
  mongo::BSONObj the_query = query_itself.obj();
  
  mongo::Query query = mongo::Query(the_query).sort(corrected_name, orderDescOrAsc ? 1 : -1);

  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  
  cursorFromMongoDB = DB->query("database.datagranules", query, limit, 0, &queryObj);
  
  std::string columns = name;
  columns.append(",");
  
  CDBStore::Store *store = ptrToStore(cursorFromMongoDB, columns.c_str());
  
  if(store == NULL){
    CDBDebug("Query %s failed",query.toString().c_str());
  }
  return store;
}

CDBStore::Store *CDBAdapterMongoDB::getUniqueValuesOrderedByIndex(const char *name, int limit, bool orderDescOrAsc,const char *table) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: getUniqueValuesOrderedByIndex]");
  #endif
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }

  /* The corrected name. Because of MongoDB, it can be that  */
  const char* corrected_name = getCorrectedColumnName(name);
  
  std::string dimension_name = "dim";
  dimension_name.append(name);
  const char * corrected_dim_name = getCorrectedColumnName(dimension_name.c_str());
  
  /* What do we want to select? Only the name variable. */
  mongo::BSONObjBuilder queryBSON;
  queryBSON << corrected_name << 1 << corrected_dim_name << 1 << "_id" << 0;
  mongo::BSONObj queryObj = queryBSON.obj();
  
  /* The query itself. */
  mongo::BSONObjBuilder query_itself;
  if(!hasEnding(table, "/")) {
    query_itself << "fileName" << table;
  } else {
    query_itself << "adaguc.dataSetPath" << table; 
  }
  mongo::BSONObj the_query = query_itself.obj();
  
  /* Sorting on multiple fields. So creating a BSON */
  mongo::BSONObjBuilder sorting_fields_builder;
  sorting_fields_builder << corrected_name << 1 << corrected_dim_name << 1;
  mongo::BSONObj sorting_fields = sorting_fields_builder.obj();
  
  mongo::Query query = mongo::Query(the_query).sort(sorting_fields);

  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  
  cursorFromMongoDB = DB->query("database.datagranules", query, limit, 0, &queryObj);
  
  std::string columns = name;
  columns.append(",");
  
  CDBStore::Store *store = ptrToStore(cursorFromMongoDB, columns.c_str());
  
  CT::string *r1 = store->getRecord(0)->get(0);
  CDBDebug("%s", r1->c_str());
  
  if(store == NULL){
    CDBDebug("Query %s failed",query.toString().c_str());
  }
  return store;
}

CDBStore::Store *CDBAdapterMongoDB::getFilesAndIndicesForDimensions(CDataSource *dataSource,int limit) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: getFilesAndIndicesForDimensions]");
    CDBDebug("limit is %d", limit);
  #endif
  mongo::DBClientConnection * DB = getDataBaseConnection();
  
  CT::string tableName;
    try{
      tableName = getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), "",dataSource);
    }catch(int e){
      CDBError("Unable to create tableName");
      return NULL;
    }
   
  mongo::BSONObjBuilder query_builder;
  if(!hasEnding(tableName.c_str(), "/")) {
    query_builder << "fileName" << tableName.c_str();
  } else { 
    query_builder << "adaguc.dataSetPath" << tableName.c_str();
  }
  mongo::BSONObj the_query = query_builder.obj();
  
  mongo::BSONObjBuilder selecting_builder;
  selecting_builder << "adaguc.path" << 1 << getCorrectedColumnName(dataSource->requiredDims[0]->netCDFDimName.c_str()) << 1 << "_id" << 0;
  
  mongo::BSONObj selecting_query = selecting_builder.obj();
  
  auto_ptr<mongo::DBClientCursor> ptrToMongo = DB->query("database.datagranules", mongo::Query(the_query).sort(getCorrectedColumnName(dataSource->requiredDims[0]->netCDFDimName.c_str()),1), limit, 0, &selecting_query);
  
  CDBStore::Store *store = ptrToStore(ptrToMongo, "path,time,dimtime");
  
  if(store != NULL) {
    return store;
  }
  return NULL;
}

/* Query needs to be defined. */
CDBStore::Store *CDBAdapterMongoDB::getFilesForIndices(CDataSource *dataSource,size_t *start,size_t *count,ptrdiff_t *stride,int limit) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: getFilesForIndices]");
    CDBDebug("Dimension %s FilePath %s", dataSource->requiredDims[0]->netCDFDimName.c_str(), dataSource->cfgLayer->FilePath[0]->value.c_str());
    CDBDebug("%d %d %d %d %d Limit %d", count[0], count[1], count[2], count[3], count[4], limit);
  #endif
  mongo::DBClientConnection * DB = getDataBaseConnection();
  
  /* Selecting the granule with the specific fileName. */
  mongo::BSONObjBuilder query;
  
  const char* string_for_name = dataSource->cfgLayer->FilePath[0]->value.c_str();
  
  if(!hasEnding(string_for_name, "/")) {
    query << "adaguc.path" << string_for_name;
  } else { 
    query << "adaguc.dataSetPath" << string_for_name;
  }
  mongo::BSONObj objBSON = query.obj();
  
  std::string time_field = "adaguc.";
  time_field.append(dataSource->requiredDims[0]->netCDFDimName.c_str());
  
  /* Figure out what dimension is being used. 
   * This is stored in the dataSets collection. */
  mongo::BSONObjBuilder data_set_name_builder;
  data_set_name_builder << "adaguc.path" <<  1 << time_field.c_str() << 1 << "_id" << 0;
  mongo::BSONObj data_set_name_obj = data_set_name_builder.obj();
  
  int nToSkip = 0;
  int limit_in_documents = 0; 
  
  if(count[2] == 0) {
    limit_in_documents = 0;
  } else {
    limit_in_documents = 1;
    nToSkip = count[4];
  }
  
  auto_ptr<mongo::DBClientCursor> ptrToMongo = DB->query("database.datagranules", mongo::Query(objBSON).sort(time_field,1) , limit_in_documents, nToSkip, &data_set_name_obj); 
  
  CDBStore::Store *store = ptrToStore(ptrToMongo, "path,time,dimtime");
  
  return store;
}

CDBStore::Store *CDBAdapterMongoDB::getDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: getDimensionInfoForLayerTableAndLayerName]");
    CDBDebug("layertable is %s, layername is %s", layertable, layername);
  #endif
  
  /* First getting the database connection. */
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return NULL;
  }
  /* Old code. Must be deleted, but for reference it will be kept. */
  //std::string buff(layertable);
  //buff.append("_");
  //buff.append(layername);
  
  /* Selecting the granule with the specific fileName. */
  mongo::BSONObjBuilder query;
  if(!hasEnding(layertable, "/")) {
    query << "fileName" << layertable;
  } else { 
    query << "adaguc.dataSetPath" << layertable;
  }
  mongo::BSONObj objBSON = query.obj();
  
  /* Figure out what dimension is being used. 
   * This is stored in the dataSets collection. */
  mongo::BSONObjBuilder data_set_name_builder;
  data_set_name_builder << "dataSetName" << 1 << "_id" << 0;
  mongo::BSONObj data_set_name_obj = data_set_name_builder.obj();
  
  auto_ptr<mongo::DBClientCursor> ptrForName = DB->query("database.datagranules", mongo::Query(objBSON), 1, 0, &data_set_name_obj);
  const char* data_set_name;
  if(ptrForName->more()) {
    data_set_name = ptrForName->next().getStringField("dataSetName"); 
  } else {
    data_set_name = ""; 
    #ifdef CDBAdapterMongoDB_DEBUG
      CDBDebug("No dataSetName specified in granule");
    #endif
  }
  
  /* Getting the dimension. */
  mongo::BSONObjBuilder data_set_builder;
  data_set_builder << "dataSetName" << data_set_name;
  mongo::BSONObj data_set_query = data_set_builder.obj();
  
  mongo::BSONObjBuilder data_set_selecting;
  data_set_selecting << "dimension" << 1 << "_id" << 0;
  mongo::BSONObj data_set_selecting_obj = data_set_selecting.obj();
  
  auto_ptr<mongo::DBClientCursor> ptrForDimension = DB->query("database.dataSets", mongo::Query(data_set_query), 1, 0, &data_set_selecting_obj);
  const char* dimension_name;
  if(ptrForDimension->more()) {
    dimension_name = ptrForDimension->next().getStringField("dimension"); 
  } else {
    dimension_name = ""; 
    #ifdef CDBAdapterMongoDB_DEBUG
      CDBDebug("No dimension specified in the dataset");
    #endif
  }
  
  /* Selecting the fields that must be returned. Named:
   * 												layer, ncname, ogcname and units. */
  // Easy, just the fileName field of the granule document.
  const char* filename_name = "fileName";
  // The layer name, so getting it from the config file.
  const char* layer_name = configurationObject->Layer[0]->Variable[0]->value.c_str();
  // The ncname is in fact the dimension_name.
  const char* ncname_name = dimension_name;
  // The ogcname can be different compared to the ncname, so getting it out of the db.
  std::string ogcname_name_as_string;
  ogcname_name_as_string.append("adaguc.layer.");
  ogcname_name_as_string.append(layername);
  ogcname_name_as_string.append(".dimension.");
  ogcname_name_as_string.append(ncname_name);
  const char* ogcname_name = ogcname_name_as_string.c_str();
  // Units is just declared in the 
  const char* units_name = "adaguc.units";
  
  current_used_dimension = ncname_name;
  
  mongo::BSONObjBuilder selectingColumns;
  selectingColumns << filename_name << 1 << layer_name << 1 << ncname_name << 1 << ogcname_name << 1 << units_name << 1 << "_id" << 0;
  mongo::BSONObj selectedColumns = selectingColumns.obj();
  
  /* Collecting the results. */
  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  cursorFromMongoDB = DB->query("database.datagranules",mongo::Query(objBSON), 0, 0, &selectedColumns);

  /* Making a store of it. */
  CDBStore::Store *store = ptrToStore(cursorFromMongoDB, table_combi.find("autoconfigure_dimensions")->second.c_str());
  if(store==NULL){
    CDBDebug("No dimension info stored for %s",layername);
  }
  return store;
}

int CDBAdapterMongoDB::storeDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername,const char *netcdfname,const char *ogcname,const char *units) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: storeDImensionInfoForLayerTableAndLayerName]");
  #endif
  /* First getting the database connection. */
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return -1;
  }
  /* Old code. Kept for references. */
  //std::string buff(layertable);
  //buff.append("_");
  //buff.append(layername);
  
  /* Selecting the right granule. */
  mongo::BSONObjBuilder query;
  query << "fileName" << layertable;
  mongo::BSONObj selectedGranule = query.obj();
  
  /* The data to be stored. */
  query << "adaguc.layer" << layername << "adaguc.ncname" << netcdfname << "adaguc.ogcname" << ogcname << "adaguc.units" << units;
  mongo::BSONObj objBSON = query.obj();
  
  /* Update the right granule. */
  DB->update("database.datagranules", mongo::Query(selectedGranule), objBSON);
  
  /* Query to see if record was succesfuly added. */
  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  
  mongo::BSONObjBuilder queryCheck;
  queryCheck << "fileName" << layertable << "adaguc.layer" << layername;
  mongo::BSONObj objBSONCheck = queryCheck.obj();
  
  cursorFromMongoDB = DB->query("database.datagranules",objBSONCheck);
  
  /* If no records are found.. */
  if(!cursorFromMongoDB->more()){
    CDBError("Unable to insert record");
    throw(__LINE__); 
  }
  return 0;
}
    
/* Not used anymore? */
int CDBAdapterMongoDB::dropTable(const char *tablename) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: dropTable]");
  #endif
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
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: createDimTableOfType]");
    CDBDebug("Dimtype is: %zu", type);
  #endif
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
  
  status = checkTableMongo(tablename,tableColumns.c_str());
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
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: checkIfFileIsInTable]");
    CDBDebug("The fileName is %s, and the path is %s",tablename, filename);
  #endif
  int fileIsOK = 1;
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return -1;
  }
  auto_ptr<mongo::DBClientCursor> cursorFromMongoDB;
  
  /* Granule needs to have the correct fileName and the correct path. */
  mongo::BSONObjBuilder query;
  query << "fileName" << tablename << "adaguc.path" << filename;
  mongo::BSONObj objBSON = query.obj();
  
  std::string table_name = "database.datagranules";
  
  /* Third parameter is number of results. */
  cursorFromMongoDB = DB->query(table_name, mongo::Query(objBSON));
  
  /* If there is result, and the next one is valid ( so no $err.. ) */
  if(cursorFromMongoDB->more()) {
    if(cursorFromMongoDB->next().isValid()) {
      fileIsOK=0;
	} else {
      fileIsOK=1;
	}
  } else {
    fileIsOK=1;
  }
  
  return fileIsOK; 
}
    
/* Not used anymore? */
int CDBAdapterMongoDB::removeFile(const char *tablename,const char *file) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: removeFile]");
    CDBDebug("Deleting %s from %s", file, tablename);
  #endif
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return -1;
  }
  
  /* Status will never change. Remove function is a void. 
   * Also, no param pointers are being used, so no feedback. */
  int status = 0;
  mongo::BSONObjBuilder query;
  query << "path" << file;
  mongo::BSONObj objBSON = query.obj();
  /* Third param is true, this means after one record has been found, it stops. */
  DB->remove(tablename,mongo::Query(objBSON),true);
  
  if(status != 0) {
    throw(__LINE__);
  }
  return status;
}

/* Not used anymore? */
int CDBAdapterMongoDB::removeFilesWithChangedCreationDate(const char *tablename,const char *file,const char *creationDate) {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: removeFilesWithChangedCreationDate]");
  #endif
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return -1;
  }
  
  mongo::BSONObjBuilder query;
  query << "path" << file << "filedate" << mongo::NE << creationDate;
  mongo::BSONObj objBSON = query.obj();
  
  std::string buff("database.");
  buff.append(tablename);  
  
  DB->remove(buff,mongo::Query(objBSON)); 
  if(DB->getPrevError().isEmpty()) {
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

/* For next story! */
int CDBAdapterMongoDB::addFilesToDataBase() {
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("[Function: addFilesToDateBase]");
    CDBDebug("Adding files to database");
  #endif
  mongo::DBClientConnection * DB = getDataBaseConnection();
  if(DB == NULL) {
    return -1;
  }

  std::string usedColumns;
  for (std::map<std::string,std::vector<std::string> >::iterator it=fileListPerTable.begin(); it!=fileListPerTable.end(); ++it){
	#ifdef CDBAdapterMongoDB_DEBUG
      CDBDebug("Updating table %s with %d records",it->first.c_str(),(it->second.size()));
    #endif
    if(it->second.size()>0){
      
      for(size_t j=0;j<it->second.size();j++){
        mongo::BSONObjBuilder insertQuery;
	    // If table is known, use those columns.
	    usedColumns = table_combi.find(it->first)->second;
	    if(usedColumns.empty()) {
	      // Otherwse use the third one.
          usedColumns = table_combi.find("all_other")->second;
	    }
	    #ifdef CDBAdapterMongoDB_DEBUG
          CDBDebug("used columns: %s",usedColumns.c_str());
        #endif
		std::string delimiter = ",";
		
		// Correctly format the values to insert
		std::string valuesToInsert = it->second[j].c_str();
		valuesToInsert.erase(0,2);
		valuesToInsert.erase(valuesToInsert.size()-2);
		
		boost::erase_all(valuesToInsert, "'");
		
		for(size_t j=0; j<4;j++) {
		  std::string cName = usedColumns.substr(0,usedColumns.find(delimiter));
		  usedColumns.erase(0,usedColumns.find(delimiter) + 1);
		  std::string cValue = valuesToInsert.substr(0,valuesToInsert.find(delimiter));
		  valuesToInsert.erase(0,valuesToInsert.find(delimiter) + 1);
		  
		  if(strcmp(cName.c_str(),"time") || strcmp(cName.c_str(),"filedate")) {
		    boost::replace_all(valuesToInsert, "T", " ");
		    boost::erase_all(valuesToInsert, "Z");
		  }
		  insertQuery << cName << cValue;
	    }
		mongo::BSONObj insertQueryObj = insertQuery.obj();
		std::string buff = "database.";
		buff.append(it->first.c_str());
        DB->insert(buff, insertQueryObj);
      }
      //int status =  dataBaseConnection->query(multiInsert.c_str()); 
      //if(status!=0){
        //CDBError("Query failed [%s]:",dataBaseConnection->getError());
        //throw(__LINE__);
      //}
      //CDBDebug("/Inserting %d bytes",multiInsert.length());
    }
    it->second.clear();
    
  }
  #ifdef CDBAdapterMongoDB_DEBUG
    CDBDebug("clearing arrays");
  #endif
  
  fileListPerTable.clear();
  return 0;
}
