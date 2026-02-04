#include <getopt.h>
#include "CDebugger.h"
#include "CTString.h"
#include "CServerError.h"
#include "Definitions.h"
#include "utils/writelogfile.h"
#include "CDBFileScanner.h"
#include "CReporter.h"
#include "CReportWriter.h"
#include "serverutils.h"
#include "CRequest.h"
#include "ConfigurationUtils.h"
#include "CDBFactory.h"
#include "CGetFileInfo.h"
#include "UpdateLayerMetadata.h"

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

int processCMDArgs(int argc, char **argv, char **) {

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
      return SCAN_EXITCODE_FILENOEXIST;
    }
    std::set<std::string> datasetsToScan;

    if (automaticallyFindMatchingDataset) {
      datasetsToScan = findDataSetsToScan(layerPathToScan, verbose);
      if (datasetsToScan.size() == 0) {
        CDBWarning("Found no matching datasets.");
        return SCAN_EXITCODE_FILENOMATCH;
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
        return SCAN_EXITCODE_DATASETNOEXIST;
      }
      logBufferCheckMode();
      status = request.updatedb(&tailPath, &layerPathToScan, scanFlags, layerName);
      if (status != 0) {
        CDBError("Error occured in updating the database");
        return SCAN_EXITCODE_SCANERROR;
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

    logBufferCheckMode();
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
  return -1;
}
