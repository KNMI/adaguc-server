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

#include "traceTimings/traceTimings.h"
#include "ProjCache.h"
#include "adagucserver.h"
#include "Types/ProjectionStore.h"
#include "utils/UpdateLayerMetadata.h"
#include "utils/ConfigurationUtils.h"
#include "CDBFactory.h"
#include "CConvertGeoJSON.h"
#include "utils/serverutils.h"

DEF_ERRORMAIN();

FILE *pLogDebugFile = NULL;
enum LogBufferMode { LogBufferMode_TRUE, LogBufferMode_FALSE, LogBufferMode_DISABLELOGGING };
LogBufferMode logMode = LogBufferMode::LogBufferMode_FALSE;

void writeLogFile(const char *msg) {
  if (logMode == LogBufferMode_DISABLELOGGING) return;
  if (pLogDebugFile != NULL) {
    fputs(msg, pLogDebugFile);
    // If message line contains data like [D:008:pid250461: adagucserverEC/CCairoPlotter.cpp:878], also append the time.
    if (strncmp(msg, "[D:", 3) == 0 || strncmp(msg, "[W:", 3) == 0 || strncmp(msg, "[E:", 3) == 0) {
      char szTemp[128];
      struct timeval tv;
      gettimeofday(&tv, NULL);
      time_t curtime = tv.tv_sec;
      tm *myUsableTime = localtime(&curtime);
      snprintf(szTemp, 127, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3dZ ", myUsableTime->tm_year + 1900, myUsableTime->tm_mon + 1, myUsableTime->tm_mday, myUsableTime->tm_hour, myUsableTime->tm_min,
               myUsableTime->tm_sec, int(tv.tv_usec / 1000));
      fputs(szTemp, pLogDebugFile);
    }
  }
}

void writeErrorFile(const char *msg) { writeLogFile(msg); }

/* Called by CDebugger */
void serverDebugFunction(const char *msg) {
  writeLogFile(msg);
  printdebug(msg, 1);
}
/* Called by CDebugger */
void serverErrorFunction(const char *msg) {
  writeErrorFile(msg);
  printerror(msg);
}
/* Called by CDebugger */
void serverWarningFunction(const char *msg) {
  writeLogFile(msg);
  printdebug(msg, 1);
}

void serverLogFunctionCMDLine(const char *msg) { printf("%s", msg); }

/**
 * @param layerPathToScan: the provided file to scan
 */
std::set<std::string> findDataSetsToScan(CT::string layerPathToScan, bool verbose) {
  std::set<std::string> datasetsToScan;
  // loop all datasets
  CRequest baseRequest;
  int status = setCRequestConfigFromEnvironment(&baseRequest);
  if (status != 0) {
    return datasetsToScan;
  }
  auto srvParam = baseRequest.getServerParams();

  // Get the directory of the file to scan:
  CT::string directoryOfFileToScan = layerPathToScan = CDirReader::makeCleanPath(layerPathToScan);
  directoryOfFileToScan.substringSelf(0, directoryOfFileToScan.length() - directoryOfFileToScan.basename().length());
  directoryOfFileToScan = CDirReader::makeCleanPath(directoryOfFileToScan) + "/";
  if (verbose) {
    CDBDebug("directoryOfFileToScan = [%s]", directoryOfFileToScan.c_str());
  }
  srvParam->verbose = false;
  auto datasetList = getEnabledDatasetsConfigurations(srvParam);

  for (auto &dataset : datasetList) {
    if (verbose) {
      CDBDebug("Testing dataset %s", dataset.c_str());
    }
    CRequest configParser;
    auto configSrvParam = configParser.getServerParams();
    configSrvParam->verbose = false;
    setCRequestConfigFromEnvironment(&configParser, dataset.c_str());

    if (configSrvParam && configSrvParam->cfg) {
      auto layers = configSrvParam->cfg->Layer;
      for (auto layer : layers) {
        if (checkIfFileMatchesLayer(layerPathToScan, layer)) {
          datasetsToScan.insert(dataset.c_str());
          break;
        }
      }
    }
  }

  return datasetsToScan;
}

/* Start handling the OGC request */
int runRequest() {
  CRequest request;
  int status = setCRequestConfigFromEnvironment(&request);
  if (status != 0) {
    CDBError("Unable to read configuration file.");
    return 1;
  }
  return request.runRequest();
}

int _main(int argc, char **argv, char **) {

  /* Initialize error functions */
  seterrormode(EXCEPTIONS_PLAINTEXT);
  setStatusCode(HTTP_STATUSCODE_200_OK);
  setErrorFunction(serverLogFunctionCMDLine);
  setWarningFunction(serverLogFunctionCMDLine);
  setDebugFunction(serverLogFunctionCMDLine);

  int opt;
  int scanFlags = 0;
  int configSet = 0;
  bool getlayers = false;
  bool automaticallyFindMatchingDataset = false;
  bool verbose = true;
  CT::string tailPath, layerPathToScan;
  CT::string file;
  CT::string inspireDatasetCSW;
  CT::string datasetPath;
  CT::string layerName;

  while (true) {
    int opt_idx = 0;
    static struct option long_options[] = {{"updatedb", no_argument, 0, 0},
                                           {"config", required_argument, 0, 0},
                                           {"createtiles", no_argument, 0, 0},
                                           {"updatelayermetadata", no_argument, 0, 0},
                                           {"tailpath", required_argument, 0, 0},
                                           {"path", required_argument, 0, 0},
                                           {"rescan", no_argument, 0, 0},
                                           {"nocleanup", no_argument, 0, 0},
                                           {"cleanfiles", optional_argument, 0, 0},
                                           {"recreate", no_argument, 0, 0},
                                           {"getlayers", no_argument, 0, 0},
                                           {"file", required_argument, 0, 0},
                                           {"inspiredatasetcsw", required_argument, 0, 0},
                                           {"datasetpath", required_argument, 0, 0},
                                           {"test", no_argument, 0, 0},
                                           {"report", optional_argument, 0, 0},
                                           {"layername", required_argument, 0, 0},
                                           {"verboseoff", no_argument, 0, 0},
                                           {"autofinddataset", no_argument, 0, 0},
                                           {NULL, 0, NULL, 0}};

    opt = getopt_long(argc, argv, "", long_options, &opt_idx);
    if (opt == -1) {
      break;
    } else if (opt == 0) {
      if (strncmp(long_options[opt_idx].name, "verboseoff", 10) == 0) {
        verbose = false;
      }
      if (strncmp(long_options[opt_idx].name, "autofinddataset", 15) == 0) {
        automaticallyFindMatchingDataset = true;
      }

      if (strncmp(long_options[opt_idx].name, "updatelayermetadata", 19) == 0) {
        scanFlags += CDBUPDATE_LAYER_METADATA;
      }

      if (strncmp(long_options[opt_idx].name, "test", 4) == 0) {
        CDBDebug("Test");
        CProj4ToCF proj4ToCF;
        proj4ToCF.debug = true;
        proj4ToCF.unitTest();
        return 0;
      }
      if (strncmp(long_options[opt_idx].name, "updatedb", 8) == 0) {
        scanFlags += CDBFILESCANNER_UPDATEDB;
      }
      if (strncmp(long_options[opt_idx].name, "createtiles", 11) == 0) {
        scanFlags += CDBFILESCANNER_CREATETILES + CDBFILESCANNER_UPDATEDB;
      }
      if (strncmp(long_options[opt_idx].name, "config", 6) == 0) {
        setenv("ADAGUC_CONFIG", optarg, 1);
        configSet = 1;
      }
      if (strncmp(long_options[opt_idx].name, "tailpath", 8) == 0) {
        tailPath.copy(optarg);
      }
      if (strncmp(long_options[opt_idx].name, "layername", 9) == 0) {
        layerName.copy(optarg);
      }
      if (strncmp(long_options[opt_idx].name, "path", 4) == 0) {
        layerPathToScan.copy(optarg);
      }
      if (strncmp(long_options[opt_idx].name, "rescan", 6) == 0) {
        CDBDebug("RESCAN: Forcing rescan of dataset");
        scanFlags |= CDBFILESCANNER_RESCAN;
      }
      if (strncmp(long_options[opt_idx].name, "nocleanup", 9) == 0) {
        CDBDebug("NOCLEANUP: Leave all records in DB, don't check if files have disappeared");
        scanFlags |= CDBFILESCANNER_DONTREMOVEDATAFROMDB;
      }
      if (strncmp(long_options[opt_idx].name, "cleanfiles", 10) == 0) {
        CDBDebug("CLEAN: Delete old files according to Layer configuration");
        scanFlags |= CDBFILESCANNER_CLEANFILES + CDBFILESCANNER_UPDATEDB;
      }
      if (strncmp(long_options[opt_idx].name, "recreate", 8) == 0) {
        CDBDebug("RECREATE: Drop tables and recreate them");
        scanFlags |= CDBFILESCANNER_RECREATETABLES;
      }
      if (strncmp(long_options[opt_idx].name, "getlayers", 9) == 0) {
        getlayers = true;
      }
      if (strncmp(long_options[opt_idx].name, "file", 4) == 0) file = optarg;
      if (strncmp(long_options[opt_idx].name, "inspiredatasetcsw", 17) == 0) inspireDatasetCSW = optarg;
      if (strncmp(long_options[opt_idx].name, "datasetpath", 11) == 0) datasetPath = optarg;
      if (strncmp(long_options[opt_idx].name, "report", 6) == 0) {
        if (optarg)
          CReporter::getInstance()->filename(optarg);
        else if (getenv("ADAGUC_CHECKER_FILE"))
          CReporter::getInstance()->filename(getenv("ADAGUC_CHECKER_FILE"));
        else
          CReporter::getInstance()->filename(REPORT_DEFAULT_FILE);
      }
    }
  }

  int status = -1; /* exit status. Tests should fail if exit status is not set: -1 is never OK. */

  /* Check if a database update was requested */
  if (((scanFlags & CDBFILESCANNER_UPDATEDB) == CDBFILESCANNER_UPDATEDB) && (configSet == 0)) {
    CDBError("Error: Configuration file is not set: use '--updatedb --config configfile.xml'");
    CDBError("And --tailpath for scanning specific sub directory, specify --path for a absolute path to update");
    return 1;
  } else if (((scanFlags & CDBFILESCANNER_UPDATEDB) == CDBFILESCANNER_UPDATEDB) && (configSet == 1)) { /* Update database */

    if (!layerPathToScan.empty() && !CDirReader::isFile(layerPathToScan.c_str())) {
      CDBError("Provided file does not exist [%s].", layerPathToScan.c_str());
      return 1;
    }
    std::set<std::string> datasetsToScan;

    if (automaticallyFindMatchingDataset) {
      datasetsToScan = findDataSetsToScan(layerPathToScan, verbose);
      if (datasetsToScan.size() == 0) {
        CDBDebug("Found no matching datasets.");
      } else {
        CDBDebug("Found matching datasets to scan: %d", datasetsToScan.size());
      }
    } else {
      // Empty string to make for loop work.
      datasetsToScan.insert("");
    }
    for (auto &dataset : datasetsToScan) {
      if (dataset.length() > 0) {
        CDBDebug("***** Scanning dataset [%s]", dataset.c_str());
      }
      CRequest request;
      request.getServerParams()->verbose = verbose;
      status = setCRequestConfigFromEnvironment(&request, dataset.c_str());
      if (status != 0) {
        CDBError("Unable to read configuration file");
        return 1;
      }
      if (logMode == LogBufferMode_FALSE) {
        // Set line buffering on stdout
        setvbuf(stdout, NULL, _IOLBF, 1024);
      }
      status = request.updatedb(&tailPath, &layerPathToScan, scanFlags, layerName);
      if (status != 0) {
        CDBError("Error occured in updating the database");
      }
    }
    readyerror();
    return status;
  } else if (scanFlags & CDBUPDATE_LAYER_METADATA) {
    CRequest baseRequest;
    int status = setCRequestConfigFromEnvironment(&baseRequest);
    if (status != 0) {
      CDBError("setCRequestConfigFromEnvironment failed");
      return 1;
    }
    auto db = CDBFactory::getDBAdapter(baseRequest.getServerParams()->cfg);
    if (db == nullptr || db->tryAdvisoryLock(LOCK_METADATATABLE_ID) == false) {
      CDBDebug("UPDATELAYERMETADATA: Skipping updateLayerMetadata already busy (tryAdvisoryLock)");
      return 1;
    } else {
      CDBDebug("UPDATELAYERMETADATA: tryAdvisoryLock obtained");
    }

    if (logMode == LogBufferMode_FALSE) {
      // Set line buffering on stdout
      setvbuf(stdout, NULL, _IOLBF, 1024);
    }
    CDBDebug("UPDATELAYERMETADATA: STARTING");
    status = updateLayerMetadata(baseRequest);
    CDBDebug("UPDATELAYERMETADATA: DONE");
    db->advisoryUnLock(LOCK_METADATATABLE_ID);
    CDBDebug("UPDATELAYERMETADATA: Unlocked");
    return status;
  }

  /* Check if layers need to be obtained. */
  if (getlayers && file.empty()) {
    CDBError("--file parameter missing");
    CDBError("Optional parameters are: --datasetpath <path> and --inspiredatasetcsw <cswurl>");
    status = 1;
  } else if (getlayers) {

    setWarningFunction(serverWarningFunction);
    setDebugFunction(serverDebugFunction);

    CT::string fileInfo = CGetFileInfo::getLayersForFile(file.c_str());
    if (inspireDatasetCSW.empty() == false) {
      inspireDatasetCSW.encodeXMLSelf();
      CT::string inspireDatasetCSWXML;
      inspireDatasetCSWXML.print("<!--header-->\n\n  <WMS>\n    <Inspire>\n      <DatasetCSW>%s</DatasetCSW>\n    </Inspire>\n  </WMS>", inspireDatasetCSW.c_str());
      fileInfo.replaceSelf("<!--header-->", inspireDatasetCSWXML.c_str());
    }
    if (datasetPath.empty() == false) {
      fileInfo.replaceSelf("[DATASETPATH]", datasetPath.c_str());
    }
    printf("%s\n", fileInfo.c_str());
    status = 0;

    readyerror();
    return status;
  }

  /* Process the OGC request */
  setErrorFunction(serverErrorFunction);
  setWarningFunction(serverWarningFunction);
  setDebugFunction(serverDebugFunction);

  traceTimingsCheckEnabled();

#ifdef MEASURETIME
  StopWatch_Start();
#endif

  status = runRequest();
  /* Display errors if any */
  readyerror();
#ifdef MEASURETIME
  StopWatch_Stop("Ready!!!");
#endif

  return getStatusCode();
}

int main(int argc, char **argv, char **envp) {
  /* Check if ADAGUC_LOGFILE is set */
  const char *ADAGUC_LOGFILE = getenv("ADAGUC_LOGFILE");
  if (ADAGUC_LOGFILE != NULL) {
    pLogDebugFile = fopen(ADAGUC_LOGFILE, "a");
    if (pLogDebugFile == NULL) {
      fprintf(stderr, "Unable to write ADAGUC_LOGFILE %s\n", ADAGUC_LOGFILE);
    }
  }

  /* Check if we enable logbuffer:
     - FALSE (unset / default): unbuffered output with live logging but can have a slower service as effect
     - TRUE: buffered logging
     - DISABLELOGGING: no logging at all
   */
  const char *ADAGUC_ENABLELOGBUFFER = getenv("ADAGUC_ENABLELOGBUFFER");
  if (ADAGUC_ENABLELOGBUFFER != NULL) {
    CT::string check = ADAGUC_ENABLELOGBUFFER;
    if (check.equalsIgnoreCase("true")) {
      logMode = LogBufferMode::LogBufferMode_TRUE;
    }
    if (check.equalsIgnoreCase("DISABLELOGGING")) {
      logMode = LogBufferMode::LogBufferMode_DISABLELOGGING;
    }
  }

  if (pLogDebugFile != NULL && logMode == LogBufferMode_FALSE) {
    // Set line buffering on log file
    setvbuf(pLogDebugFile, NULL, _IONBF, 1024);
  }

  /* Check if ADAGUC_PATH is set, if not set it here */
  const char *ADAGUC_PATH = getenv("ADAGUC_PATH");
  if (ADAGUC_PATH == NULL) {
    char str[1024];
    (void)!getcwd(str, 1023); /* TODO: maybe CWD is not the best */
    CT::string currentPath = str;
    currentPath.replaceSelf("/adaguc-server/adagucserverEC", "/adaguc-server/"); /* If we are developing directly in adagucserverEC path, remove the last dir */
    currentPath.replaceSelf("/adaguc-server/bin", "/adaguc-server/");            /* If we are developing directly in adagucserverEC path, remove the last dir */
    setenv("ADAGUC_PATH", currentPath.c_str(), currentPath.length());
    ADAGUC_PATH = getenv("ADAGUC_PATH");
    CDBDebug("ADAGUC_PATH environment variable is not set, guessing path using CWD: [%s]", ADAGUC_PATH);
  }

  /* Check if ADAGUC_TMP is set, if not set here */
  const char *ADAGUC_TMP = getenv("ADAGUC_TMP");
  if (ADAGUC_TMP == NULL) {
    setenv("ADAGUC_TMP", "/tmp/", 6);
    ADAGUC_TMP = getenv("ADAGUC_TMP");
    CDBDebug("ADAGUC_TMP environment variable is not set, setting to : [%s]", ADAGUC_TMP);
  }

  int status = _main(argc, argv, envp);

  /* Print the check report formatted as JSON. */
  CReportWriter::writeJSONReportToFile();

  CCachedDirReader::free();

  CTime::cleanInstances();

  CDFObjectStore::getCDFObjectStore()->clear();

  CDBFactory::clear();

  proj_clear_cache();
  BBOXProjectionClearCache();

  if (pLogDebugFile != NULL) {
    fclose(pLogDebugFile);
    pLogDebugFile = NULL;
  }

  return status;
}
