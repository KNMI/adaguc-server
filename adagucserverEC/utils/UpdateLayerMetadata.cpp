#include "UpdateLayerMetadata.h"
#include <CDebugger.h>
#include <CRequest.h>
#include "ConfigurationUtils.h"

void serverLogFunctionNothing(const char *) {}

/* Set config file from environment variable ADAGUC_CONFIG */
int setCRequestConfigFromEnvironment(CRequest *request, const char *additionalDataset) {
  char *configfile = getenv("ADAGUC_CONFIG");
  if (configfile != NULL) {
    CT::string configWithAdditionalDataset = configfile;
    if (additionalDataset != nullptr && strlen(additionalDataset) > 0) {
      configWithAdditionalDataset.concat(",");
      configWithAdditionalDataset.concat(additionalDataset);
    }
    int status = request->setConfigFile(configWithAdditionalDataset.c_str());

    /* Check logging level */
    if (request->getServerParams()->isDebugLoggingEnabled() == false) {
      setDebugFunction(serverLogFunctionNothing);
    }

    return status;
  } else {
    CDBError("No configuration file is set. Please set ADAGUC_CONFIG environment variable accordingly.");
    return 1;
  }
  return 0;
}

int updateLayerMetadata(CRequest &request) {
  CServerParams *srvParam = request.getServerParams();

  auto datasetList = getEnabledDatasetsConfigurations(srvParam);
  // TODO: Remove datasets in metadatable which don't have a matching configuration
  // TODO: Remove dimension tables which don't have a matching configuration

  for (auto &dataset : datasetList) {
    if (dataset.length() > 0) {
      CDBDebug("***** Scanning dataset [%s]", dataset.c_str());
    }
    CRequest requestPerDataset;
    // dataset = "/data/adaguc-datasets/test.uwcw_ha43_dini_5p5km_10x8.xml";
    // dataset = "/data/adaguc-datasets/testdata.xml";
    int status = setCRequestConfigFromEnvironment(&requestPerDataset, dataset.c_str());
    if (status != 0) {
      CDBError("Unable to read configuration file");
      return 1;
    }
    status = requestPerDataset.updatedb(nullptr, nullptr, CDBFILESCANNER_UPDATEDB | CDBFILESCANNER_DONTREMOVEDATAFROMDB | CDBFILESCANNER_UPDATEDB_ONLYFILEFROMDEFAULTQUERY, "");
    if (status != 0) {
      CDBError("Error occured in updating the database");
    }
    // return 0;
  }
  return 0;
}
