//============================================================================
// Name        : adagucserverEC.cpp
// Author      : Maarten Plieger
// Version     :
// Copyright   : LGPL
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "adagucserverEC.h"
DEF_ERRORMAIN();


void writeLogFile(const char * msg){
  char * logfile=getenv("ADAGUC_LOGFILE");
  if(logfile!=NULL){
    FILE * pFile = NULL;
    pFile = fopen (logfile , "a" );
    if(pFile != NULL){
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
      fputs  (msg, pFile );
      if(strlen(msg)>1){
        time_t myTime = time(NULL);
        tm *myUsableTime = localtime(&myTime);
        char szTemp[128];
        snprintf(szTemp,127," at %.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ ",
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
    CDBError("No configuration file is set!");
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
    CDBError("Unable to read configuration file");
    return 1;
  }
  request.process_querystring();
  return 0;
}

int main(int argc, const char *argv[]){
  

  // Initialize error functions
  seterrormode(EXCEPTIONS_PLAINTEXT);
  #ifdef MEASURETIME
  StopWatch_Start();
  #endif

  //Check if a database update was requested
  if(argc>=2){
    if(strncmp(argv[1],"--updatedb",10)==0){
      printf("**** Starting DB update\n");
      CRequest request;
      int configSet = 0;
      CT::string tailPath,layerPathToScan;
      for(int j=0;j<argc;j++){
        if(strncmp(argv[j],"--config",8)==0){
          printf("Setting environment variable ADAGUC_CONFIG to \"%s\"\n",argv[j+1]);
          setenv("ADAGUC_CONFIG",argv[j+1],0);
          configSet = 1;
        }
        if(strncmp(argv[j],"--tailpath",10)==0){
          printf("Setting tailpath to \"%s\"\n",argv[j+1]);
          tailPath.copy(argv[j+1]);
        }
        if(strncmp(argv[j],"--path",6)==0){
          printf("Setting path to \"%s\"\n",argv[j+1]);
          layerPathToScan.copy(argv[j+1]);
        }
      }
      if(configSet == 0){
        printf("Configuration file is not set: use '--updatedb --config configfile.xml/'\n" );
        printf("And --tailpath for scanning specific sub directory, specify --path for a absolute path to update\n" );

        return 0;
      }
      int status = setCRequestConfigFromEnvironment(&request);
      if(status!=0){
        CDBError("Unable to read configuration file");
        return 1;
      }
      //printf("\n");
      status = request.updatedb(&tailPath,&layerPathToScan);
      if(status != 0){
        CDBError("Error occured in updating the database");
      }
      readyerror();
      return status;
    }
  }
  //Process the OGC request
  setErrorFunction(serverErrorFunction);
  setWarningFunction(serverWarningFunction);
  setDebugFunction(serverDebugFunction);
  runRequest();
  //Display errors if any
  readyerror();
#ifdef MEASURETIME
   StopWatch_Stop("Took");
#endif

  return 0;
}

