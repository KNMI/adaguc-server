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
#include "adagucserver.h"
#include "CNetCDFDataWriter.h"
#include <set>
const char *CDBFileScanner::className="CDBFileScanner";
std::vector <CT::string> CDBFileScanner::tableNamesDone;
//#define CDBFILESCANNER_DEBUG
#define ISO8601TIME_LEN 32


bool CDBFileScanner::isTableAlreadyScanned(CT::string *tableName){
  for(size_t t=0;t<tableNamesDone.size();t++){
    if(tableNamesDone[t].equals(tableName->c_str())){
      return true;
    }
  }
  return false;
}
void CDBFileScanner::markTableDirty(CT::string *tableName){
  CDBDebug("Marking table dirty: %d %s",tableNamesDone.size(),tableName->c_str());
  for(size_t t=0;t<tableNamesDone.size();t++){
    if(tableNamesDone[t].equals(tableName->c_str())){
      tableNamesDone.erase (tableNamesDone.begin()+t);
      CDBDebug("Table marked dirty %d %s",tableNamesDone.size(),tableName->c_str());
      return;
    }
  }
}

/**
 * 
 * @return Positive on error, zero on succes, negative on skip.
 */
int CDBFileScanner::createDBUpdateTables(CDataSource *dataSource,int &removeNonExistingFiles,CDirReader *dirReader){
 ;
  int status = 0;
  CT::string query;
  
  
  CDBAdapter * dbAdapter = CDBFactory::getDBAdapter(dataSource->srvParams->cfg);
  
  
  
  if(dataSource->cfgLayer->Dimension.size()==0){
    if(CDataReader::autoConfigureDimensions(dataSource)!=0){
      CDBError("Unable to configure dimensions automatically");
      return 1;
    }
  }
  
  CDFObject *cdfObject = NULL;
  try{
    cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObject(dataSource,dirReader->fileList[0]->fullName.c_str());
  }catch(int e){
    CDBError("Unable to get CDFObject for file %s",dirReader->fileList[0]->fullName.c_str());
    return 1;
  }

  
  //Check and create all tables...
  for(size_t d=0;d<dataSource->cfgLayer->Dimension.size();d++){
  
    
    
    CDataReader::DimensionType dtype = CDataReader::getDimensionType(cdfObject,dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
    if(dtype==CDataReader::dtype_none){
      CDBWarning("dtype_none for %s",dtype,dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
    }

    
    bool isTimeDim = false;
    if(dtype == CDataReader::dtype_time || dtype == CDataReader::dtype_reference_time){
      isTimeDim = true;
    }
    
    CT::string dimName(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());

    
    //Create database tableNames
    CT::string tableName;
    try{
      tableName = dbAdapter->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str(),dataSource);
    }catch(int e){
      CDBError("Unable to create tableName from '%s' '%s' '%s'",dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str());
      return 1;

    }

    
    int TABLETYPE_TIMESTAMP = 1;
    int TABLETYPE_INT       = 2;
    int TABLETYPE_REAL      = 3;
    int TABLETYPE_STRING    = 4;

    int tableType = 0;
    
    //Check whether we already did this table in this scan
    bool skip = isTableAlreadyScanned(&tableName);
    if(skip==false){
      CDBDebug("CreateDBUpdateTables: Updating dimension '%s' with table '%s'",dimName.c_str(),tableName.c_str());
      
      //Create column names
      

          
      if(isTimeDim==true){
        tableType = TABLETYPE_TIMESTAMP;
      }else{
        try{
        
          CDF::Variable *dimVar = cdfObject->getVariableNE(dimName.c_str());
          if(dimVar==NULL){
            CDBError("Dimension '%s' not found.",dimName.c_str());
            throw(__LINE__);
          }
          
          bool hasStatusFlag=false;
          std::vector<CDataSource::StatusFlag*> statusFlagList;
          CDataSource::readStatusFlags(dimVar,&statusFlagList);
          if(statusFlagList.size()>0)hasStatusFlag=true;
          for(size_t i=0;i<statusFlagList.size();i++)delete statusFlagList[i];
          statusFlagList.clear();
          if(hasStatusFlag){
            tableType = TABLETYPE_STRING;
            status = dbAdapter->createDimTableString(dimName.c_str(),tableName.c_str());
          }else{
            switch(dimVar->getType()){
              case CDF_FLOAT:
              case CDF_DOUBLE:
                tableType = TABLETYPE_REAL;break;
              case CDF_STRING:
                tableType = TABLETYPE_STRING;break;
              default:
                tableType = TABLETYPE_INT;break;
              
            }          
          }
        }catch(int e){
          CDBError("Exception defining table structure at line %d",e);
          return 1;
        }
        
      }
      if(tableType == 0){
        CDBError("Unknown tabletype");
        return 1;
      }
    
      if(tableType == TABLETYPE_TIMESTAMP)status = dbAdapter->createDimTableTimeStamp(dimName.c_str(),tableName.c_str());
      if(tableType == TABLETYPE_INT      )status = dbAdapter->createDimTableInt      (dimName.c_str(),tableName.c_str());
      if(tableType == TABLETYPE_REAL     )status = dbAdapter->createDimTableReal     (dimName.c_str(),tableName.c_str());
      if(tableType == TABLETYPE_STRING   )status = dbAdapter->createDimTableString   (dimName.c_str(),tableName.c_str());
      
      CDBDebug("Checking filetable %s",tableName.c_str());
      
      //if(status == 0){CDBDebug("OK: Table is available");}
      if(status == 1){
        CDBError("\nFAIL: Table %s could not be created",tableName.c_str()); 
        return 1;  }
      if(status == 2){
        removeNonExistingFiles=0;
        //removeExisting files can be set back to zero, because there are no files to remove (table is created)
        //note the int &removeNonExistingFiles as parameter of this function!
        //(Setting the value will have effect outside this function)
        //CDBDebug("OK: Table %s created, (check for unavailable files is off);",tableName);
        //if( addIndexToTable(DB,tableName.c_str(),dimName.c_str()) != 0)return 1;
      }
      //TODO set removeNonExistingFiles =0 when no records are in table
      
      
      if(removeNonExistingFiles==1){
        //The temporary table should always be dropped before filling.  
        //We will do a complete new update, so store everything in an new table
        //Later we will rename this table
//         CT::string tableName_temp(&tableName);
//         if(removeNonExistingFiles==1){
//           tableName_temp.concat("_temp");
//         }
        //CDBDebug("Making empty temporary table %s ... ",tableName_temp.c_str());
        //CDBDebug("Check table %s ...\t",tableName.c_str());
/*
        if(status==0){
          //Table already exists....
          CDBError("*** WARNING: Temporary table %s already exists. Is another process updating the database? ***",tableName_temp.c_str());
          
          
          CDBDebug("*** DROPPING TEMPORARY TABLE: %s",query.c_str());
          if(dbAdapter->dropTable(tableName_temp.c_str())!=0){
            CDBError("Dropping table %s failed",tableName_temp.c_str());
            return 1;
          }

          CDBDebug("Check table %s ... ",tableName_temp.c_str());
          if(tableType == TABLETYPE_TIMESTAMP)status = dbAdapter->createDimTableTimeStamp(dimName.c_str(),tableName_temp.c_str());
          if(tableType == TABLETYPE_INT      )status = dbAdapter->createDimTableInt      (dimName.c_str(),tableName_temp.c_str());
          if(tableType == TABLETYPE_REAL     )status = dbAdapter->createDimTableReal     (dimName.c_str(),tableName_temp.c_str());
          if(tableType == TABLETYPE_STRING   )status = dbAdapter->createDimTableString   (dimName.c_str(),tableName_temp.c_str());

          if(status == 0){CDBDebug("OK: Table is available");}
          if(status == 1){CDBError("\nFAIL: Table %s could not be created",tableName_temp.c_str()); return 1;  }
          if(status == 2){CDBDebug("OK: Table %s created",tableName_temp.c_str());
            //Create a index on these files:
            //if(addIndexToTable(DB,tableName_temp.c_str(),dimName.c_str())!= 0)return 1;
          }
        }
        
        if(status == 0 || status == 1){CDBError("\nFAIL: Table %s could not be created",tableName_temp.c_str()); return 1;  }
        if(status == 2){
          //OK, Table did not exist, is created.
          //Create a index on these files:
          //if(addIndexToTable(DB,tableName_temp.c_str(),dimName.c_str()) != 0)return 1;
        }*/
      }
    }
      
  }

  return 0;
}

int CDBFileScanner::DBLoopFiles(CDataSource *dataSource,int removeNonExistingFiles,CDirReader *dirReader,int scanFlags){
//  CDBDebug("DBLoopFiles");
  CT::string query;
  CDFObject *cdfObject = NULL;
  int status = 0;
  CT::string multiInsertCache;

  CDBAdapter * dbAdapter = CDBFactory::getDBAdapter(dataSource->srvParams->cfg);
  try{
    //Loop dimensions and files
    //CDBDebug("Checking files that are already in the database...");
    //char ISOTime[ISO8601TIME_LEN+1];
    CT::string isoString;
    size_t numberOfFilesAddedFromDB=0;
    
    //Setup variables like tableNames and timedims for each dimension
    size_t numDims=dataSource->cfgLayer->Dimension.size();
    
    bool isTimeDim[numDims];
    CT::string dimNames[numDims];
    //CT::string tableColumns[numDims];
    CT::string tableNames[numDims];
    //CT::string tableNames_temp[numDims];
    bool skipDim[numDims];
    CT::string queryString;
    //CT::string VALUES;
    //CADAGUC_time *ADTime  = NULL;
    CTime adagucTime;
    
     
    CDFObject *cdfObjectOfFirstFile = NULL;
    try{
      cdfObjectOfFirstFile = CDFObjectStore::getCDFObjectStore()->getCDFObject(dataSource,dirReader->fileList[0]->fullName.c_str());
    }catch(int e){
      CDBError("Unable to get CDFObject for file %s",dirReader->fileList[0]->fullName.c_str());
      return 1;
    }


    
    for(size_t d=0;d<dataSource->cfgLayer->Dimension.size();d++){
      
     
      dimNames[d].copy(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
     
      isTimeDim[d]=false;
      skipDim[d]=false;
      
      CDataReader::DimensionType dtype = CDataReader::getDimensionType(cdfObjectOfFirstFile,dimNames[d].c_str());
      
      if(dtype==CDataReader::dtype_none){
        CDBWarning("dtype_none for %s",dtype,dimNames[d].c_str());
      }
      dimNames[d].toLowerCaseSelf();
      if(dtype == CDataReader::dtype_time || dtype == CDataReader::dtype_reference_time){
        isTimeDim[d] = true;
      }
      CDBDebug("Found dimension %d with name %s of type %d, istimedim: [%d]",d,dimNames[d].c_str(),dtype,isTimeDim[d] );
      
      try{
        tableNames[d] = dbAdapter->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dimNames[d].c_str(),dataSource);
      }catch(int e){
        CDBError("Unable to create tableName from '%s' '%s' '%s'",dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dimNames[d].c_str());
        return 1;
      }
      
//       //Create temporary tableName
//       tableNames_temp[d].copy(&(tableNames[d]));
//       if(removeNonExistingFiles==1){
//         tableNames_temp[d].concat("_temp");
//       }
//       
      skipDim[d] = isTableAlreadyScanned(&tableNames[d]);
      if(skipDim[d]){
        CDBDebug("Skipping dimension '%s' with table '%s': already scanned.",dimNames[d].c_str(),tableNames[d].c_str());
      }
    }
    
    CDBDebug("Found %d files",dirReader->fileList.size());
    
    for(size_t j=0;j<dirReader->fileList.size();j++){
      //Loop through all configured dimensions.
      #ifdef CDBFILESCANNER_DEBUG
      CDBDebug("Loop through all configured dimensions.");
      #endif
      
      
      CT::string fileDate = CDirReader::getFileDate(dirReader->fileList[j]->fullName.c_str());
      CT::string fileDateToCompareWith = fileDate;

      if(scanFlags&CDBFILESCANNER_RESCAN){
        fileDateToCompareWith = "";
        CDBDebug("--rescan set: fileDate is ignored.");
      }
      
      CT::string dimensionTextList="none";
      if(dataSource->cfgLayer->Dimension.size()>0){
        dimensionTextList.print("(%s",dataSource->cfgLayer->Dimension[0]->attr.name.c_str());
        for(size_t d=1;d<dataSource->cfgLayer->Dimension.size();d++){
          dimensionTextList.printconcat(", %s",dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
        }
        dimensionTextList.concat(")");
      }
      for(size_t d=0;d<dataSource->cfgLayer->Dimension.size();d++){
        multiInsertCache = "";
        if(skipDim[d] == false){
          
          numberOfFilesAddedFromDB=0;
          int fileExistsInDB=0;

          
          //Delete files with non-matching creation date 
          dbAdapter->removeFilesWithChangedCreationDate(tableNames[d].c_str(),dirReader->fileList[j]->fullName.c_str(),fileDateToCompareWith.c_str());
          
          //Check if file is already there
          status = dbAdapter->checkIfFileIsInTable(tableNames[d].c_str(),dirReader->fileList[j]->fullName.c_str());
          if(status == 0){
            //The file is there!
            fileExistsInDB = 1;
          }else{
            //The file is not there. If isAutoResourceEnabled and there is no file, force cleaning of autoConfigureDimensions table.
            if(removeNonExistingFiles == 1){   
              if(dirReader->fileList.size() == 1){
                CDBDebug("Removing autoConfigureDimensions [%s_%s]",tableNames[d].c_str(),dataSource->getLayerName());
                CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->removeDimensionInfoForLayerTableAndLayerName(tableNames[d].c_str(),dataSource->getLayerName());
                
                CDBDebug("Creating autoConfigureDimensions");
                status = createDBUpdateTables(dataSource,removeNonExistingFiles,dirReader);
                if(status > 0 ){
                  throw(__LINE__);
                }
              }
            }
              
          }
          
         if(removeNonExistingFiles == 0){

          if(fileExistsInDB == 1){
            //We dont remove non existing files and this files seems to be OK.
              CDBDebug("Done:   %d/%d %s\t %s",
              (int)j,
              (int)dirReader->fileList.size(),
              dimensionTextList.c_str(),
              dirReader->fileList[j]->baseName.c_str());
          }
         }
          
          //The file metadata does not already reside in the db.
          //Therefore we need to read information from it
          if(fileExistsInDB == 0){
            try{
              if(d==0){
                CDBDebug("Adding: %d/%d %s\t %s",
                (int)j,
                (int)dirReader->fileList.size(),
                dimensionTextList.c_str(),
                dirReader->fileList[j]->baseName.c_str());
              };
              #ifdef CDBFILESCANNER_DEBUG
              CDBDebug("Creating new CDFObject");
              #endif
              cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObject(dataSource,dirReader->fileList[j]->fullName.c_str());
              if(cdfObject == NULL)throw(__LINE__);
              
              //Open the file
              #ifdef CDBFILESCANNER_DEBUG
              CDBDebug("Opening file %s",dirReader->fileList[j]->fullName.c_str());
              #endif
             
              
             
                #ifdef CDBFILESCANNER_DEBUG
                CDBDebug("Looking for %s",dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
                #endif
                //Check for the configured dimensions or scalar variables
                //1 )Is this a scalar?
                CDF::Variable *  dimVar = cdfObject->getVariableNE(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
                CDF::Dimension * dimDim = cdfObject->getDimensionNE(dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
                
                
                if(dimVar!=NULL&&dimDim==NULL){
                  //Check for scalar variable
                  if(dimVar->dimensionlinks.size() == 0){
                    CDBDebug("Found scalar variable %s with no dimension. Creating dim",dimVar->name.c_str());
                    dimDim = new CDF::Dimension();
                    dimDim->name = dimVar->name;
                    dimDim->setSize(1);
                    cdfObject->addDimension(dimDim);
                    dimVar->dimensionlinks.push_back(dimDim);
                  }
                  //Check if this variable has another dim attached
                  if(dimVar->dimensionlinks.size() == 1){
                    dimDim = dimVar->dimensionlinks[0];
                    CDBDebug("Using dimension %s for dimension variable %s",dimVar->dimensionlinks[0]->name.c_str(),dimVar->name.c_str());
                  }
                }
                
                
                
                
                if(dimDim==NULL||dimVar==NULL){
                  CDBError("In file %s",dirReader->fileList[j]->fullName.c_str());
                  if(dimVar == NULL){
                    CDBError("Variable '%s' for dimension '%s' not found",dataSource->cfgLayer->Variable[0]->value.c_str(),dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
                  }
                  if(dimDim == NULL){
                    CDBError("For variable '%s' dimension '%s' not found",dataSource->cfgLayer->Variable[0]->value.c_str(),dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
                  }
                  
                  throw(__LINE__);
                }else{
                  CDF::Attribute *dimUnits = dimVar->getAttributeNE("units");
                  if(dimUnits==NULL){
                    if(isTimeDim[d]){
                      CDBError("No time units found for variable %s",dimVar->name.c_str());
                      throw(__LINE__);
                    }
                    dimVar->setAttributeText("units","1");
                    dimUnits = dimVar->getAttributeNE("units");
                  }
                  
                
                  //Create adaguctime structure, when this is a time dimension.
                  if(isTimeDim[d]){
                    try{
                      adagucTime.reset();
                      adagucTime.init((char*)dimUnits->toString().c_str());
                    }catch(int e){
                      CDBDebug("Exception occurred during time initialization: %d",e);
                      throw(__LINE__);
                    }
                  }
                  
                  #ifdef CDBFILESCANNER_DEBUG
                  CDBDebug("Dimension type = %s",CDF::getCDFDataTypeName(dimVar->getType()).c_str());
                  #endif
                  
                  #ifdef CDBFILESCANNER_DEBUG
                  CDBDebug("Reading dimension %s of length %d",dimVar->name.c_str(),dimDim->getSize());
                  #endif
                  
                  //Strings do never fit in a double.
                  if(dimVar->getType()!=CDF_STRING){
                    //Read the dimension data
                    status = dimVar->readData(CDF_DOUBLE);
                  }else{
                    //Read the dimension data
                    status = dimVar->readData(CDF_STRING);
                  }
                  //#ifdef CDBFILESCANNER_DEBUG
//                   CDBDebug("Reading dimension %s of length %d",dimVar->name.c_str(),dimDim->getSize());
                  //#endif
                  if(status!=0){
                    CDBError("Unable to read variable data for %s",dimVar->name.c_str());
                    throw(__LINE__);
                  }
                  
                  //Check for status flag dimensions
                  bool hasStatusFlag = false;
                  std::vector<CDataSource::StatusFlag*> statusFlagList;
                  CDataSource::readStatusFlags(dimVar,&statusFlagList);
                  if(statusFlagList.size()>0)hasStatusFlag=true;
                  
                  int exceptionAtLineNr=0;
                  
                  bool requiresProjectionInfo = true;

                  CDBAdapter::GeoOptions geoOptions;
                  geoOptions.level=-1;
                  geoOptions.crs="EPSG:4236";
                  geoOptions.bbox[0]=-1000;
                  geoOptions.bbox[1]=-1000;
                  geoOptions.bbox[2]=1000;
                  geoOptions.bbox[3]=1000;
                  
                  
                  if(requiresProjectionInfo){
                    CDataReader reader;
                    dataSource->addStep(dirReader->fileList[j]->fullName.c_str(),NULL);
                    reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
//                     CDBDebug("CRS:  [%s]",dataSource->nativeProj4.c_str());
//                     CDBDebug("BBOX: [%f %f %f %f]",dataSource->dfBBOX[0],dataSource->dfBBOX[1],dataSource->dfBBOX[2],dataSource->dfBBOX[3]);
                   /* crs = dataSource->nativeProj4.c_str();
                    minx = dataSource->dfBBOX[0];
                    miny = dataSource->dfBBOX[1];
                    maxx = dataSource->dfBBOX[2];
                    maxy = dataSource->dfBBOX[3];
                    level = 1 ; //Highest detail, highest resolution, most files.
                   */ 
                    geoOptions.level=1;
                    geoOptions.crs=dataSource->nativeProj4.c_str();
                    geoOptions.bbox[0]=dataSource->dfBBOX[0];
                    geoOptions.bbox[1]=dataSource->dfBBOX[1];
                    geoOptions.bbox[2]=dataSource->dfBBOX[2];
                    geoOptions.bbox[3]=dataSource->dfBBOX[3];
                    geoOptions.indices[0]=0;
                    geoOptions.indices[1]=1;
                    geoOptions.indices[2]=2;
                    geoOptions.indices[3]=3;
                    
                    
                    CDF::Attribute *adagucTileLevelAttr=dataSource->getDataObject(0)->cdfObject->getAttributeNE("adaguctilelevel");
                    
                    if(adagucTileLevelAttr != NULL){
                      geoOptions.level = adagucTileLevelAttr->toString().toInt();
                      //CDBDebug( "Found adaguctilelevel %d in NetCDF header",geoOptions.level);
                    }
                  }
                  
                  
                  try{
                    const double *dimValues=(double*)dimVar->data;
                    
                    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    //Start looping over every netcdf dimension element
                    std::set<std::string> uniqueDimensionValueSet;
                    std::pair<std::set<std::string>::iterator,bool> uniqueDimensionValueRet;
                    
                    bool dimIsUnique = true;
                    
                    CT::string uniqueKey;
                    for(size_t i=0;i<dimDim->length;i++){
                      
                      
                      CT::string uniqueKey = "";
                      //Insert individual values of type char, short, int, float, double
                      if(dimVar->getType()!=CDF_STRING){
                        if(dimValues[i]!=NC_FILL_DOUBLE){
                          if(isTimeDim[d]==false){
                            if(hasStatusFlag==true){
                              uniqueKey.print("%s",CDataSource::getFlagMeaning( &statusFlagList,double(dimValues[i])));
                              uniqueDimensionValueRet = uniqueDimensionValueSet.insert(uniqueKey.c_str());
                              if(uniqueDimensionValueRet.second == true){
                                dbAdapter->setFileString(tableNames[d].c_str(),dirReader->fileList[j]->fullName.c_str(),uniqueKey.c_str(),int(i),fileDate.c_str(),&geoOptions) ;
                              }else{
                                dimIsUnique = false;
                              }
                            }
                            if(hasStatusFlag==false){
                              switch(dimVar->getType()){
                                case CDF_FLOAT:
                                case CDF_DOUBLE:
                                  uniqueKey.print("%f",double(dimValues[i]));
                                  uniqueDimensionValueRet = uniqueDimensionValueSet.insert(uniqueKey.c_str());
                                  if(uniqueDimensionValueRet.second == true){
                                    dbAdapter->setFileReal(tableNames[d].c_str(),dirReader->fileList[j]->fullName.c_str(),double(dimValues[i]),int(i),fileDate.c_str(),&geoOptions);
                                  }else{
                                    dimIsUnique = false;
                                  }
                                  break;
                                default:
                                  uniqueKey.print("%d",int(dimValues[i]));
                                  uniqueDimensionValueRet = uniqueDimensionValueSet.insert(uniqueKey.c_str());
                                  if(uniqueDimensionValueRet.second == true){
                                    dbAdapter->setFileInt(tableNames[d].c_str(),dirReader->fileList[j]->fullName.c_str(),int(dimValues[i]),int(i),fileDate.c_str(),&geoOptions) ;
                                  }else{
                                    dimIsUnique = false;
                                  }
                                  break;
                              }
                            }
                          }else{
                            
                            //ADTime->PrintISOTime(ISOTime,ISO8601TIME_LEN,dimValues[i]);status = 0;//TODO make PrintISOTime return a 0 if succeeded
                            
                            try{
                              uniqueKey = adagucTime.dateToISOString(adagucTime.getDate(dimValues[i]));
                              uniqueKey.setSize(19);
                              uniqueKey.concat("Z");
                              dbAdapter->setFileTimeStamp(tableNames[d].c_str(),dirReader->fileList[j]->fullName.c_str(),uniqueKey.c_str(),int(i),fileDate.c_str(),&geoOptions) ;
                              
                         
                            }catch(int e){
                              CDBDebug("Exception occurred during time conversion: %d",e);
                            }
                            
                          }
                        }
                      }
                      
                      if(dimVar->getType()==CDF_STRING){
                        const char *str=((char**)dimVar->data)[i];
                        uniqueKey.print("%s",str);
                        uniqueDimensionValueRet = uniqueDimensionValueSet.insert(uniqueKey.c_str());
                        if(uniqueDimensionValueRet.second == true){
                          dbAdapter->setFileString(tableNames[d].c_str(),dirReader->fileList[j]->fullName.c_str(),uniqueKey.c_str(),int(i),fileDate.c_str(),&geoOptions) ;
                        }else{
                          dimIsUnique = false;
                        }
                      }
                      
                      
                      
                      //Check if this insert is unique
                      if(dimIsUnique == false){
                        CDBError("In file %s dimension value [%s] not unique in dimension [%s]",dirReader->fileList[j]->fullName.c_str(),uniqueKey.c_str(),dimVar->name.c_str());
                      }
                    }
                  }catch(int linenr){
                    CDBError("Exception at linenr %d",linenr);
                    exceptionAtLineNr=linenr;
                  }
                  //Cleanup statusflags
                  for(size_t i=0;i<statusFlagList.size();i++)delete statusFlagList[i];
                  statusFlagList.clear();
                  
                  //Cleanup adaguctime structure
                  
                  
                  if(exceptionAtLineNr!=0)throw(exceptionAtLineNr);
                }
              
              //delete cdfObject;cdfObject=NULL;
              //cdfObject=CDFObjectStore::getCDFObjectStore()->deleteCDFObject(&cdfObject);
            }catch(int linenr){
              CDBError("Exception in DBLoopFiles at line %d");
              CDBError(" *** SKIPPING FILE %s ***",dirReader->fileList[j]->baseName.c_str());
              //Close cdfObject. this is only needed if an exception occurs, otherwise it does nothing...
              //delete cdfObject;cdfObject=NULL;

              //TODO CHECK cdfObject=CDFObjectStore::getCDFObjectStore()->deleteCDFObject(&cdfObject);
            }
          }
        }else{
          CDBDebug("Assuming [%s] done",dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
        }
        
        
       
        
       
        
        
      }
    }
    
    //End of dimloop, start inserting our collected records in one statement
    CDBDebug("Adding files to database");
    dbAdapter->addFilesToDataBase();
    
    if(removeNonExistingFiles == 1){
      //Now delete files in the database a which are not on file system
      for(size_t d=0;d<dataSource->cfgLayer->Dimension.size();d++){
        if(skipDim[d] == false){
          CDBStore::Store *values = CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->getUniqueValuesOrderedByValue("path",0,false,tableNames[d].c_str());
          if(values==NULL){
            CDBError("No files found for %s ",dataSource->layerName.c_str());
          }else{
            CDBDebug("The database contains %d files",values->getSize());
            for(size_t j=0;j<values->getSize();j++){
              bool found = false;
              for(size_t i=0;i<dirReader->fileList.size();i++){
                if(dirReader->fileList[i]->fullName.equals(values->getRecord(j)->get(0))){
                  found = true;
                  break;
                }

              }
              if(found == false){
                CDBDebug("Deleting file %s from db",values->getRecord(j)->get(0)->c_str());
                CDBFactory::getDBAdapter(dataSource->srvParams->cfg)->removeFile(tableNames[d].c_str(),values->getRecord(j)->get(0)->c_str());
              }
            }
          }
          
          
          
          delete values;
        }
      }
    }
  

    if(numberOfFilesAddedFromDB!=0){CDBDebug("%d file(s) were already in the database",numberOfFilesAddedFromDB);}
    
    

  }
  catch(int linenr){
    #ifdef USEQUERYTRANSACTIONS                    
    DB->query("COMMIT"); 
    #endif    
    CDBError("Exception in DBLoopFiles at line %d",linenr);
    
    //TODO CHECK    cdfObject=CDFObjectStore::getCDFObjectStore()->deleteCDFObject(&cdfObject);
    return 1;
  }
  
  
  //delete cdfObject;cdfObject=NULL;
  //cdfObject=CDFObjectStore::getCDFObjectStore()->deleteCDFObject(&cdfObject);
  return 0;
}



int CDBFileScanner::updatedb( CDataSource *dataSource,CT::string *_tailPath,CT::string *_layerPathToScan,int scanFlags){
 
  if(dataSource->dLayerType!=CConfigReaderLayerTypeDataBase)return 0;
 
  CCache::Lock lock;
  CT::string identifier = "updatedb";  identifier.concat(dataSource->cfgLayer->FilePath[0]->value.c_str());  identifier.concat("/");  identifier.concat(dataSource->cfgLayer->FilePath[0]->attr.filter.c_str());  
  //CT::string cacheDirectory = "";
  CT::string cacheDirectory = dataSource->srvParams->cfg->TempDir[0]->attr.value.c_str();
  //dataSource->srvParams->getCacheDirectory(&cacheDirectory);
  if(cacheDirectory.length()>0){
    lock.claim(cacheDirectory.c_str(),identifier.c_str(),"updatedb",dataSource->srvParams->isAutoResourceEnabled());
  }

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
        //CDBDebug ("Skipping %s==%s\n",layerPath.c_str(),layerPathToScan.c_str());
        return 0;
      }
    }
  }
  // This variable enables the query to remove files that no longer exist in the filesystem
  int removeNonExistingFiles=1;
  
  int status;
 
  
  //Copy tailpath (can be provided to scan only certain subdirs)
  CT::string tailPath(_tailPath);
  
  CDirReader::makeCleanPath(&tailPath);
  
  //if tailpath is defined than removeNonExistingFiles must be zero
  if(tailPath.length()>0)removeNonExistingFiles=0;
  
  if(scanFlags&CDBFILESCANNER_DONTREMOVEDATAFROMDB){
    removeNonExistingFiles = 0;
  }
  //temp = new CDirReader();
  //temp->makeCleanPath(&FilePath);
  //delete temp;
  
  CDirReader dirReader;
  
  CDBDebug("*** Starting update layer '%s' ***",dataSource->cfgLayer->Name[0]->value.c_str());
  
  if(searchFileNames(&dirReader,dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(),tailPath.c_str())!=0)return 0;
  
  //Include tiles
  
  if(dataSource->cfgLayer->TileSettings.size() == 1){
    CDBDebug("Start including TileSettings path [%s]. (Already found %d non tiled files)",dataSource->cfgLayer->TileSettings[0]->attr.tilepath.c_str(),dirReader.fileList.size());
    if(searchFileNames(&dirReader,dataSource->cfgLayer->TileSettings[0]->attr.tilepath.c_str(),"^.*\\.nc$",tailPath.c_str())!=0)return 0;
  }
  

  
  if(dirReader.fileList.size()==0){
    CDBWarning("No files found for layer %s",dataSource->cfgLayer->Name[0]->value.c_str());
    return 0;
  }
  

  try{ 
    //First check and create all tables... returns zero on success, positive on error, negative on already done.
    status = createDBUpdateTables(dataSource,removeNonExistingFiles,&dirReader);
    if(status > 0 ){

      throw(__LINE__);
    }
    
    if(status == 0){
      
      //Loop Through all files
      status = DBLoopFiles(dataSource,removeNonExistingFiles,&dirReader,scanFlags);
      if(status != 0 )throw(__LINE__);
    }
  }
  catch(int linenr){
    CDBError("Exception in updatedb at line %d",linenr);
    #ifdef USEQUERYTRANSACTIONS                    
    if(removeNonExistingFiles==1)status = DB->query("COMMIT");
           #endif    
    return 1;
  }
  
  
 
  // Close DB
  //CDBDebug("COMMIT");
  #ifdef USEQUERYTRANSACTIONS                  
  if(removeNonExistingFiles==1)status = DB->query("COMMIT");
           #endif  
  

  CDBDebug("*** Finished update layer '%s' ***\n",dataSource->cfgLayer->Name[0]->value.c_str());
  lock.release();
  return 0;
}

//TODO READ FILE FROM DB!
int CDBFileScanner::searchFileNames(CDirReader *dirReader,const char * path,CT::string expr,const char *tailPath){
  if(path==NULL){
    CDBError("No path defined");
    return 1;
  }
  CT::string filePath=path;//dataSource->cfgLayer->FilePath[0]->value.c_str();
  
  if(tailPath!=NULL){
    if(tailPath[0]=='/'){
      filePath.copy(tailPath);
    }else{
      filePath.concat(tailPath);
    }
  }
  if(filePath.lastIndexOf(".nc")==int(filePath.length()-3)||filePath.lastIndexOf(".h5")==int(filePath.length()-3)||filePath.indexOf("http://")==0||filePath.indexOf("https://")==0||filePath.indexOf("dodsc://")==0){
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
      CT::string fileFilterExpr(".*\\.nc$");
      if(expr.empty()==false){//dataSource->cfgLayer->FilePath[0]->attr.filter.c_str()
        fileFilterExpr.copy(&expr);
      }
      //CDBDebug("Reading directory %s with filter %s",filePath.c_str(),fileFilterExpr.c_str()); 
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
  //CDBDebug("%s",dirReader->fileList[0]->fullName.c_str());
  #ifdef CDBFILESCANNER_DEBUG
  CDBDebug("Found %d file(s)",int(dirReader->fileList.size()));
  #endif
  return 0;
}


int CDBFileScanner::createTiles( CDataSource *dataSource,int scanFlags){
  CDBDebug("createTiles");
              
  if(dataSource->cfgLayer->TileSettings.size() == 0){
    CDBError("TileSettings is not set");
    return 1;
  }
  CDBAdapter * dbAdapter = CDBFactory::getDBAdapter(dataSource->srvParams->cfg);
  if(dataSource->cfgLayer->Dimension.size()==0){
    if(CDataReader::autoConfigureDimensions(dataSource)!=0){
      CDBError("Unable to configure dimensions automatically");
      return 1;
    }
  }
  CDirReader dirReader;
  
  CDBDebug("*** Starting update layer '%s' ***",dataSource->cfgLayer->Name[0]->value.c_str());
  
  if(searchFileNames(&dirReader,dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(),NULL)!=0)return 0;
  
  if(dirReader.fileList.size()==0){
    CDBWarning("No files found for layer %s",dataSource->cfgLayer->Name[0]->value.c_str());
    return 1;
  }

  CDataReader *reader = new CDataReader();;
  dataSource->addStep(dirReader.fileList[0]->fullName.c_str(),NULL);
  reader->open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
  delete reader;
  CDBDebug("CRS:  [%s]",dataSource->nativeProj4.c_str());
  CDBDebug("BBOX: [%f %f %f %f]",dataSource->dfBBOX[0],dataSource->dfBBOX[1],dataSource->dfBBOX[2],dataSource->dfBBOX[3]);
  //const char *crs = dataSource->nativeProj4.c_str();
  double minx = dataSource->dfBBOX[0];
  double miny = dataSource->dfBBOX[1];
  double maxx = dataSource->dfBBOX[2];
  double maxy = dataSource->dfBBOX[3];    
  
  double tileBBOXWidth = fabs(maxx-minx);
  double tileBBOXHeight = fabs(maxy-miny);
      
  CT::string tableName ;
  for(size_t d=0;d<dataSource->cfgLayer->Dimension.size();d++){
    try{

      
      tableName = dbAdapter->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(),dataSource->cfgLayer->Dimension[d]->attr.name.c_str(),dataSource);
      CDBDebug("Create tiles  [%s]",tableName.c_str());
      
      double globalBBOX[4];
      
      CDBStore::Store *store;
      store = dbAdapter->getMin("minx",tableName.c_str());globalBBOX[0] = store->getRecord(0)->get(0)->toDouble();delete store;
      store = dbAdapter->getMin("miny",tableName.c_str());globalBBOX[1] = store->getRecord(0)->get(0)->toDouble();delete store;
      store = dbAdapter->getMax("maxx",tableName.c_str());globalBBOX[2] = store->getRecord(0)->get(0)->toDouble();delete store;
      store = dbAdapter->getMax("maxy",tableName.c_str());globalBBOX[3] = store->getRecord(0)->get(0)->toDouble();delete store;
      
      CDBDebug("globalBBOX: [%f,%f,%f,%f]",globalBBOX[0],globalBBOX[1],globalBBOX[2],globalBBOX[3]);
      CDBDebug("cellSizeX,cellSizeY: [%f,%f]",dataSource->dfCellSizeX,dataSource->dfCellSizeY);
      
      double tilesetWidth = globalBBOX[2] - globalBBOX[0];
      double tilesetHeight = globalBBOX[3] - globalBBOX[1];
      
      CDBDebug("tilesetWidth,tilesetHeight: [%f,%f]",tilesetWidth,tilesetHeight);
      double nrTilesX = tilesetWidth/tileBBOXWidth, nrTilesY = tilesetHeight/tileBBOXHeight;
      CDBDebug("nrTilesX,nrTilesY: [%f,%f]",nrTilesX,nrTilesY);
      int isClosed = true;
      int maxlevel            = dataSource->cfgLayer->TileSettings[0]->attr.maxlevel.toInt();
      for(int level = 2;level<maxlevel+1;level++){
        int numFound = 0;
        int numCreated=0;
        CDBDebug("Tiling level %d",level);
        int levelInc = pow(2,level-1);
        //CDBDebug("Tiling levelInc %d",levelInc);
        for(int y=0;y<nrTilesY;y=y+levelInc){
          CDBDebug("**** Scanning tile: Level %d, Percentage done for this level: %.2f ***",level,(float(y)/float(nrTilesY))*100);
          for(int x=0;x<nrTilesX;x=x+levelInc){
            if(isClosed){
              CDataReader *reader = new CDataReader();;
              dataSource->addStep(dirReader.fileList[0]->fullName.c_str(),NULL);
              reader->open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
              isClosed = false;
              delete reader;
            }
            double dfMinX = globalBBOX[0]+tileBBOXWidth*x;
            double dfMinY = globalBBOX[1]+tileBBOXHeight*y;
            double dfMaxX = globalBBOX[0]+tileBBOXWidth*(x+levelInc);
            double dfMaxY = globalBBOX[1]+tileBBOXHeight*(y+levelInc);

            CT::string fileNameToWrite = dataSource->cfgLayer->TileSettings[0]->attr.tilepath.c_str(); 
            fileNameToWrite.printconcat("/l%dleft%ftop%f.nc",level,dfMinX,dfMinY);
            CDirReader::makeCleanPath(&fileNameToWrite);
            //CDBDebug("Checking [%s][%s]" ,tableName.c_str(),fileNameToWrite.c_str());
            int status = dbAdapter->checkIfFileIsInTable(tableName.c_str(),fileNameToWrite.c_str());
            if(status == 0){
              //CDBDebug("Tile %s already done.",fileNameToWrite.c_str());
            }else{
              dataSource->srvParams->Geo->CRS=dataSource->nativeProj4;
              dataSource->srvParams->Geo->dfBBOX[0]=dfMinX;
              dataSource->srvParams->Geo->dfBBOX[1]=dfMinY;
              dataSource->srvParams->Geo->dfBBOX[2]=dfMaxX;
              dataSource->srvParams->Geo->dfBBOX[3]=dfMaxY;
              
              dataSource->nativeViewPortBBOX[0]=dfMinX;
              dataSource->nativeViewPortBBOX[1]=dfMinY;
              dataSource->nativeViewPortBBOX[2]=dfMaxX;
              dataSource->nativeViewPortBBOX[3]=dfMaxY;
              
             
              //CDBDebug("New globalBBOX: [%f,%f,%f,%f]",dataSource->srvParams->Geo->dfBBOX[0],dataSource->srvParams->Geo->dfBBOX[1],dataSource->srvParams->Geo->dfBBOX[2],dataSource->srvParams->Geo->dfBBOX[3]);
              try{
                try{
                  CRequest::fillDimValuesForDataSource(dataSource,dataSource->srvParams);
                }catch(ServiceExceptionCode e){
                  CDBError("Exception in setDimValuesForDataSource");
                }

                
              // CDBDebug("setDimValuesForDataSource done");
                //CDBDebug("START");
                dataSource->queryLevel=level-1;
                dataSource->queryBBOX=1;
                store = dbAdapter->getFilesAndIndicesForDimensions(dataSource,3000);
                //CDBDebug("OK");
                if(store!=NULL){
                  //CDBDebug("*** Found %d %d:%d = %f %f",store->getSize(),x,y,dfMinX,dfMinY);
                  CDataSource ds;
                  for(size_t d=0;d<dataSource->requiredDims.size();d++){
                    COGCDims *ogcDim = new COGCDims();
                    ds.requiredDims.push_back(ogcDim);
                    ogcDim->name.copy(dataSource->requiredDims[d]->name.c_str());
                    ogcDim->value.copy(dataSource->requiredDims[d]->value.c_str());
                    ogcDim->netCDFDimName.copy(dataSource->requiredDims[d]->netCDFDimName.c_str());
                  }
                  
                  if(store->getSize()>0&&1==1){
                    int status =0;
                    
                    CNetCDFDataWriter *wcsWriter = new CNetCDFDataWriter();
                    try{
                      
                      CServerParams newSrvParams;
                      
                      
                      newSrvParams.cfg=dataSource->srvParams->cfg;
                      newSrvParams.Geo->dWidth=dataSource->dWidth;
                      newSrvParams.Geo->dHeight=dataSource->dHeight;
                      newSrvParams.Geo->dfBBOX[0]=dfMinX;
                      newSrvParams.Geo->dfBBOX[1]=dfMinY;
                      newSrvParams.Geo->dfBBOX[2]=dfMaxX;
                      newSrvParams.Geo->dfBBOX[3]=dfMaxY;
                      newSrvParams.Format="adagucnetcdf";
                      newSrvParams.WCS_GoNative=0;
                      newSrvParams.Geo->CRS.copy(&dataSource->nativeProj4);
                      int ErrorAtLine =0;
                      try{
                        CDBDebug("wcswriter init");
                        status = wcsWriter->init(&newSrvParams,dataSource,dataSource->getNumTimeSteps());if(status!=0){throw(__LINE__);};
                        
                        std::vector <CDataSource*> dataSources;
                        dataSources.push_back(&ds);
                        int layerNo = dataSource->datasourceIndex;
                        if(ds.setCFGLayer(dataSource->srvParams,dataSource->srvParams->configObj->Configuration[0],dataSource->srvParams->cfg->Layer[layerNo],NULL,layerNo)!=0){
                          return 1;
                        }
  //                       try{
  //                       CRequest::setDimValuesForDataSource(&ds,&newSrvParams);
  //                       }catch(int e){
  //                         CDBError("CRequest::setDimValuesForDataSource");
  //                       }
                        
  
                        
                        {//Check if really necessary.
//                           CDBDebug("ds.requiredDims.size() = %d",ds.requiredDims.size());
                          for(size_t k=0;k<store->getSize();k++){
                            CDBStore::Record *record = store->getRecord(k);
                            ds.addStep(record->get(0)->c_str(),NULL);
                            for(size_t i=0;i<ds.requiredDims.size();i++){
                              CT::string value = record->get(1+i*2)->c_str();
                              ds.getCDFDims()->addDimension(ds.requiredDims[i]->netCDFDimName.c_str(),value.c_str(),atoi(record->get(2+i*2)->c_str()));
                              ds.requiredDims[i]->addValue(value.c_str());
                            }
                            
                          }
                        }
                        
                        for(size_t k=0;k<(size_t)dataSources[0]->getNumTimeSteps();k++){
                          for(size_t d=0;d<dataSources.size();d++){
                            dataSources[d]->setTimeStep(k);
                          }

                          status = wcsWriter->addData(dataSources);if(status!=0){throw(__LINE__);};
                          //Clean up.
                          for(size_t d=0;d<dataSources.size();d++){
                            CDFObjectStore::getCDFObjectStore()->deleteCDFObject(&dataSources[d]->getDataObject(0)->cdfObject);
                          }
                          isClosed = true;
                        }
                        
                        //TODO get cache location
                        //TODO add files to db.
                        CDBDebug("wcswriter writefile");
                        
                        status = wcsWriter->writeFile(fileNameToWrite.c_str(),level);if(status!=0){throw(__LINE__);};

  //                       CDataReader *reader = new CDataReader();;
  //                       reader->open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
  //                       delete reader;
                        
                        //CDBDebug("Starting Updating DB");
                        updatedb(dataSources[0],&fileNameToWrite,NULL,CDBFILESCANNER_DONTREMOVEDATAFROMDB|CDBFILESCANNER_UPDATEDB);
                        //CDBDebug("Done Updating DB");
                      }catch(int e){
                        ErrorAtLine=e;
                      }
                      newSrvParams.cfg = NULL;
                      if(ErrorAtLine!=0)throw(ErrorAtLine);
                    }catch(int e){
                      CDBError("Exception at line %d",e);
                      status  =1;
                    }  
                    delete wcsWriter;
                    
                    if(status!=0)return 1;
                  }
                  numFound+=store->getSize();
                  if(store->getSize()>0){
                    numCreated++;
                  }
                }
                delete store;store=NULL;
                
              }catch(ServiceExceptionCode e){
                CDBError("Catched ServiceExceptionCode %d at line %d",e,__LINE__);
              }catch(int e){
                CDBError("Catched integerException %d at line %d",e,__LINE__);
              }
              
              delete store;store=NULL;
            }
          }
        }
        CDBDebug( "Found %d file, created %d tiles",numFound,numCreated);
      }
     
      
    }catch(int e){
      
      CDBError("Unable to create tableName from '%s' '%s' '%s'",dataSource->cfgLayer->FilePath[0]->value.c_str(),dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dataSource->cfgLayer->Dimension[d]->attr.name.c_str());
      return 1;
    }
  }         
  return 0;
};
