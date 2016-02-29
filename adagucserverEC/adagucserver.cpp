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

#include "adagucserver.h"
DEF_ERRORMAIN();


void writeLogFile(const char * msg){
  char * logfile=getenv("ADAGUC_LOGFILE");
  if(logfile!=NULL){
    FILE * pFile = NULL;
    pFile = fopen (logfile , "a" );
    if(pFile != NULL){
 //     setvbuf(pFile, NULL, _IONBF, 0);
      fputs  (msg, pFile );
      if(strncmp(msg,"[D:",3)==0||strncmp(msg,"[W:",3)==0||strncmp(msg,"[E:",3)==0){
        time_t myTime = time(NULL);
        tm *myUsableTime = localtime(&myTime);
        char szTemp[128];
        snprintf(szTemp,127,"%.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ ",
                myUsableTime->tm_year+1900,myUsableTime->tm_mon+1,myUsableTime->tm_mday,
                myUsableTime->tm_hour,myUsableTime->tm_min,myUsableTime->tm_sec
                );
        fputs  (szTemp, pFile );
      }
      fclose (pFile);
    }else fprintf(stderr,"Unable to write logfile %s\n",logfile);
  }
}

void writeErrorFile(const char * msg){
  //fprintf(stderr,"%s",msg);
  char * logfile=getenv("ADAGUC_ERRORFILE");
  if(logfile!=NULL){
    FILE * pFile;
    pFile = fopen (logfile , "a" );
    if(pFile != NULL){
//      setvbuf(pFile, NULL, _IONBF, 0);
      fputs  (msg, pFile );
      if(strncmp(msg,"[D:",3)==0||strncmp(msg,"[W:",3)==0||strncmp(msg,"[E:",3)==0){
        time_t myTime = time(NULL);
        tm *myUsableTime = localtime(&myTime);
        char szTemp[128];
        snprintf(szTemp,127,"%.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ ",
                 myUsableTime->tm_year+1900,myUsableTime->tm_mon+1,myUsableTime->tm_mday,
                 myUsableTime->tm_hour,myUsableTime->tm_min,myUsableTime->tm_sec
        );
        fputs  (szTemp, pFile );
      }
      fclose (pFile);
    }else fprintf(stderr,"Unable to write error logfile %s\n",logfile);
  };
  writeLogFile(msg);
}

// Called by CDebugger
void serverDebugFunction(const char *msg){
  writeLogFile(msg);
  printdebug(msg,1);
}
// Called by CDebugger
void serverErrorFunction(const char *msg){
  writeErrorFile(msg);
  printerror(msg);
}
// Called by CDebugger
void serverWarningFunction(const char *msg){
  writeLogFile(msg);
  printdebug(msg,1);
  if(strncmp(msg,"[W: ",4)!=0){
    printerror(msg);
  }
}


//Set config file from environment variable ADAGUC_CONFIG
int setCRequestConfigFromEnvironment(CRequest *request){
  char * configfile=getenv("ADAGUC_CONFIG");
  if(configfile!=NULL){
    int status = request->setConfigFile(configfile);
    return status;
  }else{
    CDBError("No configuration file is set. Please set ADAGUC_CONFIG environment variable accordingly.");
    //request->setConfigFile("/nobackup/users/plieger/cpp/oper/config/DWD.xml");
    return 1;
  }
  return 0;
}

//Start handling the OGC request
int runRequest(){
  CRequest request;
  int status = setCRequestConfigFromEnvironment(&request);
  if(status!=0){
    CDBError("Unable to read configuration file.");
    return 1;
  }
  return request.runRequest();
}

// #include "CDBAdapterSQLLite.h"
// #include "CPGSQLDB.h"
int main(int argc, const char *argv[]){
//   CDBDebug("Start");
//   CDBAdapterSQLLite::CSQLLiteDB *db = new CDBAdapterSQLLite::CSQLLiteDB();
//   db->connect("test.db");
//   
// //   CPGSQLDB *db = new CPGSQLDB();
// //   db->connect("dbname=autoopendap  host=127.0.0.1 user=plieger");
// 
//   
//   CDBStore::Store *s = db->queryToStore("PRAGMA table_info(t20150506t165354054_irajwhnerk3l6twcfta)");
//   if(s == NULL){
//     CDBError("error!");
//     return 0;
//   }
//   CT::string col = "";
//   for(size_t c=0;c<s->getColumnModel()->getSize();c++){
//     col.printconcat("%s\t\t",s->getColumnModel()->getName(c));
//   }
//   CDBDebug("%s",col.c_str());
//   
//   
//   for(size_t j=0;j<s->getSize();j++){
//      CDBStore::Record* r =s->getRecord(j);
//      CT::string row = "";
//      for(size_t c=0;c<s->getColumnModel()->getSize();c++){
//        row.printconcat("%s\t\t",r->get(c)->c_str());
//      }
//      CDBDebug("%s",row.c_str());
//   }
//   
//   delete s;
//   delete db;
//   return 0;

  // Initialize error functions
  seterrormode(EXCEPTIONS_PLAINTEXT);


  //Check if a database update was requested
  if(argc>=2){
  
    if(strncmp(argv[1],"--updatedb",10)==0||strncmp(argv[1],"--createtiles",13)==0){
      int scanFlags = 0;
      if(strncmp(argv[1],"--updatedb",10)==0){
        scanFlags+=CDBFILESCANNER_UPDATEDB;
      }
      if(strncmp(argv[1],"--createtiles",13)==0){
        scanFlags+=CDBFILESCANNER_CREATETILES;
      }
      CDBDebug("***** Starting DB update *****\n");
      CRequest request;
      int configSet = 0;
     
      CT::string tailPath,layerPathToScan;
      for(int j=0;j<argc;j++){
        if(strncmp(argv[j],"--config",8)==0&&argc>j+1){
          
          //CDBDebug("Setting environment variable ADAGUC_CONFIG to \"%s\"\n",argv[j+1]);
          setenv("ADAGUC_CONFIG",argv[j+1],0);
          configSet = 1;
        }
        if(strncmp(argv[j],"--tailpath",10)==0&&argc>j+1){
          //printf("Setting tailpath to \"%s\"\n",argv[j+1]);
          tailPath.copy(argv[j+1]);
        }
        if(strncmp(argv[j],"--path",6)==0&&argc>j+1){
          //printf("Setting path to \"%s\"\n",argv[j+1]);
          layerPathToScan.copy(argv[j+1]);
        }
        if(strncmp(argv[j],"--rescan",8)==0){
          CDBDebug("RESCAN: Forcing rescan of dataset");
          scanFlags|=CDBFILESCANNER_RESCAN;
        }
        if(strncmp(argv[j],"--nocleanup",11)==0){
          CDBDebug("NOCLEANUP: Leave all records in DB, don't check if files have disappeared");
          scanFlags|=CDBFILESCANNER_DONTREMOVEDATAFROMDB;
        }
        
      }
      if(configSet == 0){
        CDBError("Error: Configuration file is not set: use '--updatedb --config configfile.xml'" );
        CDBError("And --tailpath for scanning specific sub directory, specify --path for a absolute path to update" );

        return 0;
      }
      int status = setCRequestConfigFromEnvironment(&request);
      if(status!=0){
        CDBError("Unable to read configuration file");
        return 1;
      }

      status = request.updatedb(&tailPath,&layerPathToScan,scanFlags);
      if(status != 0){
        CDBError("Error occured in updating the database");
      }
    

                
      readyerror();
      return status;
    }
    
    if(strncmp(argv[1],"--getlayers",11)==0){
      int status = 0;
      CT::string file;
      CT::string inspireDatasetCSW;
      CT::string datasetPath;
      
      for(int j=0;j<argc;j++){
        CT::string argument = argv[j];
        if(j+1<argc&&argument.equals("--file"))file = argv[j+1];
        if(j+1<argc&&argument.equals("--inspiredatasetcsw"))inspireDatasetCSW = argv[j+1];
        if(j+1<argc&&argument.equals("--datasetpath"))datasetPath = argv[j+1];
        
      }
      if(file.empty()){
         CDBError("--file parameter missing");
         CDBError("Optional parameters are: --datasetpath <path> and --inspiredatasetcsw <cswurl>");
         status=1;
      }else{
        
        setWarningFunction(serverWarningFunction);
        setDebugFunction(serverDebugFunction);
        
        CT::string fileInfo = CGetFileInfo::getLayersForFile(file.c_str());
        if(inspireDatasetCSW.empty() == false){
          inspireDatasetCSW.encodeXMLSelf();
          CT::string inspireDatasetCSWXML;
          inspireDatasetCSWXML.print("<!--header-->\n\n  <WMS>\n    <Inspire>\n      <DatasetCSW>%s</DatasetCSW>\n    </Inspire>\n  </WMS>",inspireDatasetCSW.c_str());
          fileInfo.replaceSelf("<!--header-->",inspireDatasetCSWXML.c_str());
          
        
          
        }
        if(datasetPath.empty() == false){
          fileInfo.replaceSelf("[DATASETPATH]",datasetPath.c_str());
        }
        printf("%s\n",fileInfo.c_str());
        status = 0;
      }
      readyerror();
      return status;
    }
    
    if(strncmp(argv[1],"--test",6)==0){
      CDBDebug("Test");
      CProj4ToCF proj4ToCF;
      proj4ToCF.debug=true;
      proj4ToCF.unitTest();
      return 0;
    }
  }
  //Process the OGC request
  setErrorFunction(serverErrorFunction);
  setWarningFunction(serverWarningFunction);
  setDebugFunction(serverDebugFunction);
  
#ifdef MEASURETIME
  StopWatch_Start();
#endif
  
  int status = runRequest();
  //Display errors if any
  readyerror();
#ifdef MEASURETIME
   StopWatch_Stop("Ready!!!");
#endif
   

  return status;
}

