#include "./ConfigurationUtils.h"

std::vector<std::string> getEnabledDatasetsConfigurations(CServerParams *srvParam) {
  std::vector<std::string> datasetList;
  for (auto dataset : srvParam->cfg->Dataset) {
    if (dataset->attr.enabled.equals("true") && dataset->attr.location.empty() == false) {
      if (srvParam->verbose) {
        CDBDebug("Dataset locations %s", dataset->attr.location.c_str());
      }
      auto files = CDirReader::listDir(dataset->attr.location.c_str(), false, "^.*\\.xml$");
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
