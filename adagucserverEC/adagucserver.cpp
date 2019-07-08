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
#include "CReporter.h"
#include "CReportWriter.h"
#include <getopt.h>
#include "CDebugger_H.h"
extern Tracer NewTrace;

DEF_ERRORMAIN();

FILE * pLogDebugFile = NULL;
bool useLogBuffer = false;

int myPID = int(getpid());

void writeLogFile(const char * msg){
  if(pLogDebugFile != NULL){
    if (useLogBuffer == false) {
      setvbuf(pLogDebugFile, NULL, _IONBF, 0);
    }
    fputs  (msg, pLogDebugFile );
    if(strncmp(msg,"[D:",3)==0||strncmp(msg,"[W:",3)==0||strncmp(msg,"[E:",3)==0){
      time_t myTime = time(NULL);
      tm *myUsableTime = localtime(&myTime);
      char szTemp[128];
      snprintf(szTemp,127,"%.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ/%d ",
              myUsableTime->tm_year+1900,myUsableTime->tm_mon+1,myUsableTime->tm_mday,
              myUsableTime->tm_hour,myUsableTime->tm_min,myUsableTime->tm_sec,
              myPID
              );
      fputs  (szTemp, pLogDebugFile );
    }
  }
}

void writeErrorFile(const char * msg){
//   //fprintf(stderr,"%s",msg);
//   char * logfile=getenv("ADAGUC_ERRORFILE");
//   if(logfile!=NULL){
//     FILE * pFile;
//     pFile = fopen (logfile , "a" );
//     if(pFile != NULL){
// //      setvbuf(pFile, NULL, _IONBF, 0);
//       fputs  (msg, pFile );
//       if(strncmp(msg,"[D:",3)==0||strncmp(msg,"[W:",3)==0||strncmp(msg,"[E:",3)==0){
//         time_t myTime = time(NULL);
//         tm *myUsableTime = localtime(&myTime);
//         char szTemp[128];
//         snprintf(szTemp,127,"%.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ/%d ",
//                  myUsableTime->tm_year+1900,myUsableTime->tm_mon+1,myUsableTime->tm_mday,
//                  myUsableTime->tm_hour,myUsableTime->tm_min,myUsableTime->tm_sec,
//                  myPID
//         );
//         fputs  (szTemp, pFile );
//       }
//       fclose (pFile);
//     }else fprintf(stderr,"Unable to write error logfile %s\n",logfile);
//   };
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
//   if(strncmp(msg,"[W: ",4)!=0){ //<-- do not enable: when something printed with printerror causes getmap to fail!!!
//     printerror(msg);
//   }
}

void serverLogFunctionCMDLine(const char *msg){
  printf("%s", msg);
}


//Set config file from environment variable ADAGUC_CONFIG
int setCRequestConfigFromEnvironment(CRequest *request){
  char * configfile=getenv("ADAGUC_CONFIG");
  if(configfile!=NULL){
    // CDBDebug( "Setting to [%s]",configfile);
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

      
      
int _main(int argc, char **argv, char **envp){

  // Initialize error functions
  seterrormode(EXCEPTIONS_PLAINTEXT);
  setErrorFunction(serverLogFunctionCMDLine);
  setWarningFunction(serverLogFunctionCMDLine);
  setDebugFunction(serverLogFunctionCMDLine);

  int opt;
  int scanFlags = 0;
  int configSet = 0;
  bool getlayers=false;
  CT::string tailPath, layerPathToScan;
  CT::string file;
  CT::string inspireDatasetCSW;
  CT::string datasetPath;
  
 
  while(true) {
      int opt_idx = 0;
      static struct option long_options[] = {
          { "updatedb", no_argument, 0, 0 },
          { "config", required_argument, 0, 0 },
          { "createtiles", no_argument, 0, 0 },
          { "tailpath", required_argument, 0, 0 },
          { "path", required_argument, 0, 0 },
          { "rescan", no_argument, 0, 0 },
          { "nocleanup", no_argument, 0, 0 },
          { "recreate", no_argument, 0, 0 },
          { "getlayers", no_argument, 0, 0 },
          { "file", required_argument, 0, 0 },
          { "inspiredatasetcsw", required_argument, 0, 0 },
          { "datasetpath", required_argument, 0, 0 },
          { "test", no_argument, 0, 0 },
          { "report", optional_argument, 0, 0 }
      };

      opt = getopt_long(argc, argv, "", long_options, &opt_idx);
      if (opt == -1) {
          break;
      } else if (opt == 0) {
          if(strncmp(long_options[opt_idx].name,"test",4)==0){
              CDBDebug("Test");
              CProj4ToCF proj4ToCF;
              proj4ToCF.debug=true;
              proj4ToCF.unitTest();
              return 0;
          }
          if(strncmp(long_options[opt_idx].name,"updatedb",8)==0){
              scanFlags+=CDBFILESCANNER_UPDATEDB;
          }
          if(strncmp(long_options[opt_idx].name,"createtiles",11)==0){
              scanFlags+=CDBFILESCANNER_CREATETILES + CDBFILESCANNER_UPDATEDB;
          }
          if(strncmp(long_options[opt_idx].name,"config",6)==0){
              setenv("ADAGUC_CONFIG",optarg,1);
              configSet = 1;
          }
          if(strncmp(long_options[opt_idx].name,"tailpath",8)==0){
              tailPath.copy(optarg);
          }
          if(strncmp(long_options[opt_idx].name,"path",4)==0){
              layerPathToScan.copy(optarg);
          }
          if(strncmp(long_options[opt_idx].name,"rescan",6)==0){
              CDBDebug("RESCAN: Forcing rescan of dataset");
              scanFlags|=CDBFILESCANNER_RESCAN;
          }
          if(strncmp(long_options[opt_idx].name,"nocleanup",9)==0){
              CDBDebug("NOCLEANUP: Leave all records in DB, don't check if files have disappeared");
              scanFlags|=CDBFILESCANNER_DONTREMOVEDATAFROMDB;
          }
          if(strncmp(long_options[opt_idx].name,"recreate",8)==0){
              CDBDebug("RECREATE: Drop tables and recreate them");
              scanFlags|=CDBFILESCANNER_RECREATETABLES;
          }
          if(strncmp(long_options[opt_idx].name,"getlayers",9)==0){
              getlayers = true;
          }
          if(strncmp(long_options[opt_idx].name, "file", 4) == 0)
              file = optarg;
          if(strncmp(long_options[opt_idx].name, "inspiredatasetcsw", 17) == 0)
              inspireDatasetCSW = optarg;
          if(strncmp(long_options[opt_idx].name, "datasetpath", 11) == 0)
              datasetPath = optarg;
          if(strncmp(long_options[opt_idx].name, "report", 6) == 0) {
              if(optarg) CReporter::getInstance()->filename(optarg);
              else if (getenv("ADAGUC_CHECKER_FILE"))
                  CReporter::getInstance()->filename(getenv("ADAGUC_CHECKER_FILE"));
              else CReporter::getInstance()->filename(REPORT_DEFAULT_FILE);
          }
      }
  }

  int status = -1; // exit status. Tests should fail if exit status is not set: -1 is never OK.
  
  //Check if a database update was requested
  if(((scanFlags & CDBFILESCANNER_UPDATEDB) == CDBFILESCANNER_UPDATEDB) && (configSet == 0)){
      CDBError("Error: Configuration file is not set: use '--updatedb --config configfile.xml'" );
      CDBError("And --tailpath for scanning specific sub directory, specify --path for a absolute path to update" );
      return 1;
  } else if (((scanFlags & CDBFILESCANNER_UPDATEDB) == CDBFILESCANNER_UPDATEDB) && (configSet == 1)) { // Update database
      CRequest request;
      status = setCRequestConfigFromEnvironment(&request);
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

  // Check if layers need to be obtained.
  if(getlayers && file.empty()){
      CDBError("--file parameter missing");
      CDBError("Optional parameters are: --datasetpath <path> and --inspiredatasetcsw <cswurl>");
      status=1;
  } else if (getlayers) {
        
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
      
      readyerror();
      return status;
  }

  //Process the OGC request
  setErrorFunction(serverErrorFunction);
  setWarningFunction(serverWarningFunction);
  setDebugFunction(serverDebugFunction);
  
//    if (envp != NULL){
//     for (char **env = envp; *env != 0; env++)  {
//       char *thisEnv = *env;
//       if (thisEnv!=NULL){
//         CDBDebug("%s", thisEnv);    
//       }
//     }
//   }
  
#ifdef MEASURETIME
  StopWatch_Start();
#endif
  
  status = runRequest();
  //Display errors if any
  readyerror();
#ifdef MEASURETIME
   StopWatch_Stop("Ready!!!");
#endif

  return status;
}


int main(int argc, char **argv, char **envp){
  
  const char * ADAGUC_LOGFILE=getenv("ADAGUC_LOGFILE");
  if(ADAGUC_LOGFILE!=NULL){
    pLogDebugFile = fopen (ADAGUC_LOGFILE , "a" );
    if(pLogDebugFile ==NULL){
      fprintf(stderr,"Unable to write ADAGUC_LOGFILE %s\n",ADAGUC_LOGFILE);
    }
  }
  const char * ADAGUC_ENABLELOGBUFFER=getenv("ADAGUC_ENABLELOGBUFFER");
  if(ADAGUC_ENABLELOGBUFFER!=NULL){
    CT::string check = ADAGUC_ENABLELOGBUFFER;
    if(check.equalsIgnoreCase("true")){
      useLogBuffer = true;
    } 
  }
  int status = _main(argc,argv, envp);

  // Print the check report formatted as JSON.
  CReportWriter::writeJSONReportToFile();

  CCachedDirReader::free();
  
  CTime::cleanInstances();
  
  /* Check Tracer for leaks */
  if (NewTrace.Dump() != 0){
    if (status == 0) status = 1; //Indicates that we have a memory leak
  }
  
  if(pLogDebugFile!= NULL){
    fclose (pLogDebugFile);
    pLogDebugFile = NULL;
  }
  
  return status;
}
