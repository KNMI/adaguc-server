#include "./ConfigurationUtils.h"

std::vector<std::string> getEnabledDatasetsConfigurations(CServerParams *srvParam) {
  std::vector<std::string> datasetList;
  if (srvParam->cfg->Dataset.size() == 0) {
    CDBWarning("No dataset paths are configured");
  }
  for (auto dataset : srvParam->cfg->Dataset) {
    if (dataset->attr.enabled.equals("true") && dataset->attr.location.empty() == false) {
      // if (srvParam->verbose) {
      CDBDebug("Checking dataset location %s", dataset->attr.location.c_str());
      // }
      auto files = CDirReader::listDir(dataset->attr.location.c_str(), false, "^.*\\.xml$");
      if (files.size() == 0) {
        CDBWarning("No datasets found in directory [%s]", dataset->attr.location.c_str());
      }
      datasetList.insert(datasetList.end(), files.begin(), files.end());
    }
  }
  return datasetList;
}

// TODO: configure a list of possible extensions for adaguc at a central place
bool checkIfPathIsFile(CT::string filePath) {
  return (filePath.endsWith(".nc") || filePath.endsWith(".h5") || filePath.endsWith(".hdf5") || filePath.endsWith(".he5") || filePath.endsWith(".png") || filePath.endsWith(".csv") ||
          filePath.endsWith(".geojson") || filePath.endsWith(".json") || filePath.startsWith("http://") || filePath.startsWith("https://") || filePath.startsWith("dodsc://"));
}

void serverLogFunctionNothing(const char *) {}

/* Set config file from environment variable ADAGUC_CONFIG */
int setCRequestConfigFromEnvironment(CRequest *request, CT::string additionalDataset) {
  char *configfile = getenv("ADAGUC_CONFIG");
  if (configfile != NULL) {
    CT::string configWithAdditionalDataset = configfile;
    if (additionalDataset.empty() == false) {
      configWithAdditionalDataset.concat(",");
      configWithAdditionalDataset.concat(additionalDataset);
    } else {
      CDBDebug("No additional dataset provided");
    }

    CDBDebug("Using config %s", configWithAdditionalDataset.c_str());

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
