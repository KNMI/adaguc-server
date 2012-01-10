#include "CDBFileScanner.h"
const char *CDBFileScanner::className="CDBFileScanner";

#define MAX_STR_LEN 8191


/*int CDBFileScanner::addIndexToTable(CPGSQLDB *DB,const char *tableName, const char * dimName){
  return 0;
  // Test if the table exists , if not create the table
  CDBDebug("AddIndexToTable table %s ...\t",tableName);
  CT::string query;

  //Create an index on these files:
  query.print("create index %s_idx on %s (%s)",tableName,tableName,dimName);
  CDBDebug("%s",query.c_str());
  if(DB->query(query.c_str())!=0){
    CDBError("Query %s failed",query.c_str());
    DB->close();
    return 1;
  }
  
  //Create an index on path
  query.print("create index %s_pathidx on %s (path)",tableName,tableName);
  CDBDebug("%s",query.c_str());
  if(DB->query(query.c_str())!=0){
    CDBError("Query %s failed",query.c_str());
    DB->close();
    return 1;
  }
  
  return 0;
}*/

int CDBFileScanner::createDBUpdateTables(CPGSQLDB *DB,CDataSource *dataSource,int &removeNonExistingFiles){
  int status = 0;
  CT::string query;
  //First check and create all tables...
  for(size_t d=0;d<dataSource->cfgLayer->Dimension.size();d++){
    bool isTimeDim = false;
    CT::string dimName(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
    dimName.toLowerCase();
    if(dimName.equals("time"))isTimeDim=true;
    //How do we detect correctly wether this is a time dim?
    if(dimName.indexOf("time")!=-1)isTimeDim=true;
    
    //Create database tableNames
    CT::string tableName(dataSource->cfgLayer->DataBaseTable[0]->value.c_str());
    CServerParams::makeCorrectTableName(&tableName,&dimName);
    //Create column names
    CT::string tableColumns("path varchar (255)");
    if(isTimeDim==true){
      tableColumns.printconcat(", %s timestamp, dim%s int",dimName.c_str(),dimName.c_str());
    }else{
      tableColumns.printconcat(", %s varchar (16), dim%s int",dimName.c_str(),dimName.c_str());
    }
    
    // Put the file date in the database, in order to be able to detect whether a file has been changed in a later stage.       
    
    //if(d==0){
      tableColumns.printconcat(", filedate timestamp");
    //}
    
    //Unique constraint / PRIMARY KEY
    tableColumns.printconcat(", PRIMARY KEY (path, %s)",dimName.c_str());
    
    //CDBDebug("Check table %s with columns  %s ...\t",tableName.c_str(),tableColumns.c_str());
    CDBDebug("Check table %s ",tableName.c_str());
    status = DB->checkTable(tableName.c_str(),tableColumns.c_str());
    //if(status == 0){CDBDebug("OK: Table is available");}
    if(status == 1){CDBError("\nFAIL: Table %s could not be created: %s",tableName.c_str(),tableColumns.c_str()); DB->close();return 1;  }
    if(status == 2){
      removeNonExistingFiles=0;
      //removeExisting files can be set back to zero, because there are no files to remove (table is created)
      //note the int &removeNonExistingFiles as parameter of this function!
      //(Setting the value will have effect outside this function)
      //CDBDebug("OK: Table %s created, (check for unavailable files is off);",tableName);
      //if( addIndexToTable(DB,tableName.c_str(),dimName.c_str()) != 0)return 1;
    }
    
    if(removeNonExistingFiles==1){
      //The temporary table should always be dropped before filling.  
      //We will do a complete new update, so store everything in an new table
      //Later we will rename this table
      CT::string tableName_temp(&tableName);
      if(removeNonExistingFiles==1){
        tableName_temp.concat("_temp");
      }
      CDBDebug("Making empty temporary table %s ... ",tableName_temp.c_str());
      CDBDebug("Check table %s ...\t",tableName.c_str());
      status=DB->checkTable(tableName_temp.c_str(),tableColumns.c_str());
      if(status==0){
        //Table already exists....
        query.print("drop table %s",tableName_temp.c_str());
        CDBError("*** Warning! Temporary table already exists!!! IS ANOTHER PROCESS UPDATING THE DB ALREADY? ***");
        CDBDebug("*** DROPPING TEMPORARY TABLE: %s",query.c_str());
        if(DB->query(query.c_str())!=0){
          CDBError("Query %s failed",query.c_str());
          DB->close();
          return 1;
        }
        CDBDebug("Check table %s ... ",tableName_temp.c_str());
        status = DB->checkTable(tableName_temp.c_str(),tableColumns.c_str());
        if(status == 0){CDBDebug("OK: Table is available");}
        if(status == 1){CDBError("\nFAIL: Table %s could not be created: %s",tableName_temp.c_str(),tableColumns.c_str()); DB->close();return 1;  }
        if(status == 2){CDBDebug("OK: Table %s created",tableName_temp.c_str());
          //Create a index on these files:
          //if(addIndexToTable(DB,tableName_temp.c_str(),dimName.c_str())!= 0)return 1;
        }
      }
      
      if(status == 0 || status == 1){CDBError("\nFAIL: Table %s could not be created: %s",tableName_temp.c_str(),tableColumns.c_str()); DB->close();return 1;  }
      if(status == 2){
        //OK, Table did not exist, is created.
        //Create a index on these files:
        //if(addIndexToTable(DB,tableName_temp.c_str(),dimName.c_str()) != 0)return 1;
      }
    }
    
  }
  return 0;
}

int CDBFileScanner::DBLoopFiles(CPGSQLDB *DB,CDataSource *dataSource,int removeNonExistingFiles,CDirReader *dirReader){
  CT::string query;
  CDFObject *cdfObject = NULL;
  int status = 0;

  try{
    //Loop dimensions and files
    //CDBDebug("Checking files that are already in the database...");
    char ISOTime[MAX_STR_LEN+1];
    size_t numberOfFilesAddedFromDB=0;
    
    //Setup variables like tableNames and timedims for each dimension
    size_t numDims=dataSource->cfgLayer->Dimension.size();
    bool isTimeDim[numDims];
    CT::string dimNames[numDims];
    //CT::string tableColumns[numDims];
    CT::string tableNames[numDims];
    CT::string tableNames_temp[numDims];
    CT::string queryString;
    CT::string VALUES;
    CADAGUC_time *ADTime  = NULL;
    for(size_t d=0;d<dataSource->cfgLayer->Dimension.size();d++){
      dimNames[d].copy(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
      dimNames[d].toLowerCase();
      isTimeDim[d]=false;
      if(dimNames[d].equals("time"))isTimeDim[d]=true;
      //TODO: implement use of standardname? How do we detect correctly wether this is a time dim?
      if(dimNames[d].indexOf("time")!=-1)isTimeDim[d]=true;
      //Create column names
      /*tableColumns[d].copy("path varchar (255)");
      if(isTimeDim[d]==true){
        tableColumns[d].printconcat(", %s timestamp, dim%s int",dimNames[d].c_str(),dimNames[d].c_str());
      }else{
        //tableColumns[d].printconcat(", %s real, dim%s int",dimNames[d].c_str(),dimNames[d].c_str());
        tableColumns[d].printconcat(", %s varchar (16), dim%s int",dimNames[d].c_str(),dimNames[d].c_str());
      }*/
      //Create database tableNames
      tableNames[d].copy(dataSource->cfgLayer->DataBaseTable[0]->value.c_str());
      CServerParams::makeCorrectTableName(&(tableNames[d]),&(dimNames[d]));
      //Create temporary tableName
      tableNames_temp[d].copy(&(tableNames[d]));
      if(removeNonExistingFiles==1){
        tableNames_temp[d].concat("_temp");
      }
    }
    
    for(size_t j=0;j<dirReader->fileList.size();j++){
      //Loop through all configured dimensions.
      #ifdef CDATAREADER_DEBUG
      CDBDebug("Loop through all configured dimensions.");
      #endif
      
      
      CT::string fileDate;
      CDirReader::getFileDate(&fileDate,dirReader->fileList[j]->fullName.c_str());
      if(fileDate.length()<10)fileDate.copy("1970-01-01T00:00:00Z");
      CT::string dimensionTextList;
      dimensionTextList.print("(%s",dataSource->cfgLayer->Dimension[0]->attr.name.c_str());
      for(size_t d=1;d<dataSource->cfgLayer->Dimension.size();d++){
        dimensionTextList.printconcat(", %s",dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
      }
      dimensionTextList.concat(")");
      for(size_t d=0;d<dataSource->cfgLayer->Dimension.size();d++){
        int fileExistsInDB=0;
        //If we are messing in the non-temporary table (e.g.removeNonExistingFiles==0)
        //we need to make a transaction to make sure a file is not added twice
        //If removeNonExistingFiles==1, we are using the temporary table
        //Which is already locked by a transaction 
        if(removeNonExistingFiles==0){
          #ifdef USEQUERYTRANSACTIONS                
          status = DB->query("BEGIN"); if(status!=0)throw(__LINE__);
          #endif          
        }
        
        //Delete files with non-matching creation date 
        query.print("delete from %s where path = '%s' and (filedate != '%s' or filedate is NULL)",tableNames[d].c_str(),dirReader->fileList[j]->fullName.c_str(),fileDate.c_str());
        //CDBDebug("Deleting: %s", query.c_str());
        status = DB->query(query.c_str()); if(status!=0)throw(__LINE__);

        //Check if file already resides in the nontemporary database
        query.print("select path from %s where path = '%s' and filedate = '%s' and filedate is not NULL limit 1",tableNames[d].c_str(),dirReader->fileList[j]->fullName.c_str(),fileDate.c_str());
        //CDBDebug("Checking: %s", query.c_str());
        CT::string *pathValues = DB->query_select(query.c_str(),0);
        if(pathValues == NULL){CDBError("Query failed");DB->close();throw(__LINE__);}
        if(pathValues->count==1){fileExistsInDB=1;}else{fileExistsInDB=0;}
        delete[] pathValues;
          
        //Move this record from the nontemporary table into the temporary table
        if(fileExistsInDB == 1&&removeNonExistingFiles==1){
          //The file resides already in the nontemporary table, copy it into the temporary table
          // Now check wether the file date has changed
          
          
          CT::string mvRecordQuery;
         /* mvRecordQuery.print("INSERT INTO %s select path,%s,dim%s from %s where path = '%s'",
            tableNames_temp[d].c_str(),
            dimNames[d].c_str(),
            dimNames[d].c_str(),
            tableNames[d].c_str(),
            dirReader->fileList[j]->fullName.c_str()
          );*/
          mvRecordQuery.print("INSERT INTO %s select * from %s where path = '%s'",
                              tableNames_temp[d].c_str(),                             
                              tableNames[d].c_str(),
                              dirReader->fileList[j]->fullName.c_str()
          );
          //printf("%s\n",mvRecordQuery.c_str());
          if(j%1000==0&&d==0){
            CDBDebug("Re-using record %d/%d %s\t %s",
            (int)j,
            (int)dirReader->fileList.size(),
            dimensionTextList.c_str(),
            dirReader->fileList[j]->baseName.c_str());
          }
          if(DB->query(mvRecordQuery.c_str())!=0){CDBError("Query %s failed",mvRecordQuery.c_str());throw(__LINE__);}
          numberOfFilesAddedFromDB++;
        }
        
        //The file metadata does not already reside in the db.
        //Therefore we need to read information from it
        if(fileExistsInDB == 0){
          try{
            if(d==0){
              CDBDebug("Adding fileNo %d/%d %s\t %s",
              (int)j,
              (int)dirReader->fileList.size(),
              dimensionTextList.c_str(),
              dirReader->fileList[j]->baseName.c_str());
            };
            #ifdef CDATAREADER_DEBUG
            CDBDebug("Creating new CDFObject");
            #endif
            cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObject(dataSource,dirReader->fileList[j]->fullName.c_str());
            if(cdfObject == NULL)throw(__LINE__);
            
            //Open the file
            #ifdef CDATAREADER_DEBUG
            CDBDebug("Opening file %s",dirReader->fileList[j]->fullName.c_str());
            #endif
            
            status = cdfObject->open(dirReader->fileList[j]->fullName.c_str());
            if(status!=0){
              CDBError("Unable to open file '%s'",dirReader->fileList[j]->fullName.c_str());
              throw(__LINE__);
            }
            #ifdef CDATAREADER_DEBUG
            CDBDebug("File opened.");
            #endif
            
            if(status==0){
              CDF::Dimension * dimDim = cdfObject->getDimensionNE(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
              CDF::Variable *  dimVar = cdfObject->getVariableNE(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
              if(dimDim==NULL||dimVar==NULL){
                CDBError("In file %s",dirReader->fileList[j]->fullName.c_str());
                CDBError("For variable '%s' dimension '%s' not found",dataSource->cfgLayer->Variable[0]->value.c_str(),
                  dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
                throw(__LINE__);
              }else{
                CDF::Attribute *dimUnits = dimVar->getAttributeNE("units");
                if(dimUnits==NULL){
                  dimVar->setAttributeText("units","1");
                  dimUnits = dimVar->getAttributeNE("units");
                }
                
              
                //Create adaguctime structure, when this is a time dimension.
                if(isTimeDim[d]){
                  try{ADTime = new CADAGUC_time((char*)dimUnits->data);}catch(int e){delete ADTime;ADTime=NULL;throw(__LINE__);}
                }
                
                //Read the dimension data
                status = dimVar->readData(CDF_DOUBLE);if(status!=0){
                  CDBError("Unable to read variable data for %s",dimVar->name.c_str());
                  throw(__LINE__);
                }
                
                //Check for status flag dimensions
                bool hasStatusFlag = false;
                std::vector<CDataSource::StatusFlag*> statusFlagList;
                CDataSource::readStatusFlags(dimVar,&statusFlagList);
                if(statusFlagList.size()>0)hasStatusFlag=true;
                
                int exceptionAtLineNr=0;
                try{
                  const double *dimValues=(double*)dimVar->data;
                  for(size_t i=0;i<dimDim->length;i++){
                    if(dimValues[i]!=NC_FILL_DOUBLE){
                      if(isTimeDim[d]==false){
                        if(hasStatusFlag==true){
                          VALUES.print("VALUES ('%s','%s','%d','%s')",dirReader->fileList[j]->fullName.c_str(),CDataSource::getFlagMeaning( &statusFlagList,double(dimValues[i])),int(i),fileDate.c_str());
                        }
                        if(hasStatusFlag==false){
                          switch(dimVar->type){
                            case CDF_UINT:
                            case CDF_INT:
                              VALUES.print("VALUES ('%s','%d','%d','%s')",dirReader->fileList[j]->fullName.c_str(),int(dimValues[i]),int(i),fileDate.c_str());break;
                            default:
                              VALUES.print("VALUES ('%s','%f','%d','%s')",dirReader->fileList[j]->fullName.c_str(),double(dimValues[i]),int(i),fileDate.c_str());
                          }
                        }
                      }else{
                        VALUES.copy("");
                        ADTime->PrintISOTime(ISOTime,MAX_STR_LEN,dimValues[i]);status = 0;//TODO make PrintISOTime return a 0 if succeeded
                        if(status == 0){
                          ISOTime[19]='Z';ISOTime[20]='\0';
                          VALUES.print("VALUES ('%s','%s','%d','%s')",dirReader->fileList[j]->fullName.c_str(),ISOTime,int(i),fileDate.c_str());
                        }
                      }
                      
                      //Insert record into DB.
                      if(VALUES.length()>0){
                        //Add the record to the temporary table.
                        queryString.print("INSERT into %s %s",tableNames_temp[d].c_str(),VALUES.c_str());
                        status = DB->query(queryString.c_str()); 
                        if(status!=0){
                          CDBError("Query failed: %s",queryString.c_str());
                          throw(__LINE__);
                        }
                        //CDBDebug("queryString= %s",queryString.c_str());
                        if(removeNonExistingFiles==1){
                          //We are adding the query above to the temporary table if removeNonExistingFiles==1;
                          //Lets add it also to the non temporary table for convenience
                          //Later this table will be dropped, but it will remain more up to date during scanning this way.
                          queryString.print("INSERT into %s %s",tableNames[d].c_str(),VALUES.c_str());
                          DB->query(queryString.c_str()); 
                        }
                      }
                    }
                  }
                }catch(int linenr){
                  exceptionAtLineNr=linenr;
                }
                //Cleanup statusflags
                for(size_t i=0;i<statusFlagList.size();i++)delete statusFlagList[i];
                statusFlagList.clear();
                
                //Cleanup adaguctime structure
                if(isTimeDim[d]){delete ADTime;ADTime=NULL;}
                
                if(exceptionAtLineNr!=0)throw(exceptionAtLineNr);
              }
            }
            //delete cdfObject;cdfObject=NULL;
            cdfObject=CDFObjectStore::getCDFObjectStore()->deleteCDFObject(&cdfObject);
          }catch(int linenr){
            CDBError("Exception in DBLoopFiles at line %d, msg: '%s'",linenr,DB->getError());
            CDBError(" *** SKIPPING FILE %s ***",dirReader->fileList[j]->baseName.c_str());
            //Close cdfObject. this is only needed if an exception occurs, otherwise it does nothing...
            //delete cdfObject;cdfObject=NULL;
            delete ADTime;ADTime=NULL;
            cdfObject=CDFObjectStore::getCDFObjectStore()->deleteCDFObject(&cdfObject);
          }
        }
        //If we are messing in the non-temporary table (e.g.removeNonExistingFiles==0)
        //we need to make a transaction to make sure a file is not added twice
        //If removeNonExistingFiles==1, we are using the temporary table
        //Which is already locked by a transaction 
        if(removeNonExistingFiles==0){
          //CDBDebug("COMMIT");
          #ifdef USEQUERYTRANSACTIONS                
          status = DB->query("COMMIT"); if(status!=0)throw(__LINE__);
          #endif          
        }
      }
    }
    if(status != 0){CDBError(DB->getError());throw(__LINE__);}
    if(numberOfFilesAddedFromDB!=0){CDBDebug("%d files not scanned, they were already in the database",numberOfFilesAddedFromDB);}
    
    
    bool checkForDuplicateEntries=false;
    //If this layer has only one dimension, we are able to detect whether there are multiple entries in our database.
    if(dataSource->cfgLayer->Dimension.size()==1&&checkForDuplicateEntries==true){
      //delete from e_obs_tg_time using (select time,count(time),min(filedate) as oldestfiledate from e_obs_tg_time group by time)C where C.count != 1 and e_obs_tg_time.time=C.time and filedate=C.oldestfiledate ;
      
      //select * from e_obs_tg_time,(select time,count(time),min(filedate) as oldestfiledate from e_obs_tg_time group by time)C where C.count != 1 and e_obs_tg_time.time=C.time and filedate=C.oldestfiledate ;
      const char *tname=tableNames_temp[0].c_str();
      const char *dname=dimNames[0].c_str();
      queryString.print("DELETE FROM %s using (",tname);
      queryString.printconcat("SELECT %s,count(%s),max(filedate) AS mostrecentfiledate FROM %s group by %s",dname,dname,tname,dname);
      queryString.printconcat(")C where C.count!=1 and %s.%s=C.%s and filedate!=C.mostrecentfiledate",tname,dname,dname);
      CDBDebug("Checking for duplicate entries and selecting most recent files:\n%s",queryString.c_str());
      status = DB->query(queryString.c_str()); 
      if(status!=0){
        CDBError("Query failed: %s",queryString.c_str());
        throw(__LINE__);
      }
    }
    
    
  }
  catch(int linenr){
    #ifdef USEQUERYTRANSACTIONS                    
    DB->query("COMMIT"); 
    #endif    
    CDBError("Exception in DBLoopFiles at line %d",linenr);
    cdfObject=CDFObjectStore::getCDFObjectStore()->deleteCDFObject(&cdfObject);
    return 1;
  }
  
  
  //delete cdfObject;cdfObject=NULL;
  cdfObject=CDFObjectStore::getCDFObjectStore()->deleteCDFObject(&cdfObject);
  return 0;
}



int CDBFileScanner::updatedb(const char *pszDBParams, CDataSource *dataSource,CT::string *_tailPath,CT::string *_layerPathToScan){
  
  if(dataSource->dLayerType!=CConfigReaderLayerTypeDataBase)return 0;
  
  //We only need to update the provided path in layerPathToScan. We will simply ignore the other directories
  if(_layerPathToScan!=NULL){
    if(_layerPathToScan->length()!=0){
      CT::string layerPath,layerPathToScan;
      layerPath.copy(dataSource->cfgLayer->FilePath[0]->value.c_str());
      layerPathToScan.copy(_layerPathToScan);
      
      CDirReader::makeCleanPath(&layerPath);
      CDirReader::makeCleanPath(&layerPathToScan);
      
      //If this is another directory we will simply ignore it.
      if(layerPath.equals(&layerPathToScan)==false){
        CDBError("Skipping %s==%s\n",layerPath.c_str(),layerPathToScan.c_str());
        return 0;
      }
    }
  }
  // This variable enables the query to remove files that no longer exist in the filesystem
  int removeNonExistingFiles=1;
  
  int status;
  CPGSQLDB DB;
  
  //Copy tailpath (can be provided to scan only certain subdirs)
  
  CT::string tailPath(_tailPath);
  
  CDirReader::makeCleanPath(&tailPath);
  
  //if tailpath is defined than removeNonExistingFiles must be zero
  if(tailPath.length()>0)removeNonExistingFiles=0;
  
  
  //temp = new CDirReader();
  //temp->makeCleanPath(&FilePath);
  //delete temp;
  
  CDirReader dirReader;
  
  CDBDebug("*** Starting update layer [\"%s\"] ***",dataSource->cfgLayer->Name[0]->value.c_str());
  
  if(searchFileNames(&dirReader,dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(),tailPath.c_str())!=0)return 0;
  
  if(dirReader.fileList.size()==0)return 0;
  
  /*----------- Connect to DB --------------*/
  //CDBDebug("Connecting to DB ...\t");
  status = DB.connect(pszDBParams);if(status!=0){
    CDBError("FAILED: Unable to connect to the database with parameters: [%s]",pszDBParams);
    return 1;
  }

  try{ 
    
    status = DB.query("SET client_min_messages TO WARNING");
    if(status != 0 )throw(__LINE__);
    
    //First check and create all tables...
    status = createDBUpdateTables(&DB,dataSource,removeNonExistingFiles);
    if(status != 0 )throw(__LINE__);
           
           //CDBDebug("removeNonExistingFiles = %d\n",removeNonExistingFiles);
    //We need to do a transaction if we want to remove files from the existing table
    if(removeNonExistingFiles==1){
      //CDBDebug("BEGIN");
      #ifdef USEQUERYTRANSACTIONS      
      status = DB.query("BEGIN"); if(status!=0)throw(__LINE__);
      #endif      
    }
    
    
    //Loop Through all files
    status = DBLoopFiles(&DB,dataSource,removeNonExistingFiles,&dirReader);
    if(status != 0 )throw(__LINE__);
           
           //In case of a complete update, the data is written in a temporary table
    //Rename the table to the correct one (remove _temp)
    if(removeNonExistingFiles==1){
      for(size_t d=0;d<dataSource->cfgLayer->Dimension.size();d++){
        CT::string dimName(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
        CT::string tableName(dataSource->cfgLayer->DataBaseTable[0]->value.c_str());
        CServerParams::makeCorrectTableName(&tableName,&dimName);
        CDBDebug("Renaming temporary table... %s",tableName.c_str());
        CT::string query;
        //Drop old table
        query.print("drop table %s",tableName.c_str());
        if(DB.query(query.c_str())!=0){CDBError("Query %s failed",query.c_str());DB.close();throw(__LINE__);}
        //Rename new table to old table name
        query.print("alter table %s_temp rename to %s",tableName.c_str(),tableName.c_str());
        if(DB.query(query.c_str())!=0){CDBError("Query %s failed",query.c_str());DB.close();throw(__LINE__);}
        if(status!=0){throw(__LINE__);}
      }
    }
    
  }
  catch(int linenr){
    CDBError("Exception in updatedb at line %d",linenr);
    #ifdef USEQUERYTRANSACTIONS                    
    if(removeNonExistingFiles==1)status = DB.query("COMMIT");
           #endif    
    status = DB.close();return 1;
  }
  // Close DB
  //CDBDebug("COMMIT");
  #ifdef USEQUERYTRANSACTIONS                  
  if(removeNonExistingFiles==1)status = DB.query("COMMIT");
           #endif  
  status = DB.close();if(status!=0)return 1;

  CDBDebug("*** Finished update layer [\"%s\"] ***\n",dataSource->cfgLayer->Name[0]->value.c_str());
  //printStatus("OK","HOi %s","Maarten");
  return 0;
}


int CDBFileScanner::searchFileNames(CDirReader *dirReader,const char * path,const char * expr,const char *tailPath){
  if(path==NULL){
    CDBError("No path defined");
    return 1;
  }
  CT::string filePath=path;//dataSource->cfgLayer->FilePath[0]->value.c_str();
  if(tailPath!=NULL)filePath.concat(tailPath);
  if(filePath.lastIndexOf(".nc")>0||filePath.indexOf("http://")>=0||filePath.indexOf("https://")>=0||filePath.indexOf("dodsc://")>=0){
    //Add single file or opendap URL.
    CFileObject * fileObject = new CFileObject();
    fileObject->fullName.copy(&filePath);
    fileObject->baseName.copy(&filePath);
    fileObject->isDir=false;
    dirReader->fileList.push_back(fileObject);
  }else{
    //Read directory
    dirReader->makeCleanPath(&filePath);
    try{
      CT::string fileFilterExpr("\\.nc");
      if(expr!=NULL){//dataSource->cfgLayer->FilePath[0]->attr.filter.c_str()
        fileFilterExpr.copy(expr);
      }
      CDBDebug("Reading directory %s with filter %s",filePath.c_str(),fileFilterExpr.c_str()); 
      dirReader->listDirRecursive(filePath.c_str(),fileFilterExpr.c_str());
      
      //Delete all files that start with a "." from the filelist.
      for(size_t j=0;j<dirReader->fileList.size();j++){
        if(dirReader->fileList[j]->baseName.c_str()[0]=='.'){
          delete dirReader->fileList[j];
          dirReader->fileList.erase(dirReader->fileList.begin()+j);
          j--;
        }
      }
      
    }catch(const char *msg){
      CDBDebug("Directory %s does not exist, silently skipping...",filePath.c_str());
      return 1;
    }
  }
  CDBDebug("Found %d file(s) in directory",int(dirReader->fileList.size()));
  return 0;
}

