
#include "serverutils.h"
#include "CRequest.h"
#include "ConfigurationUtils.h"

bool checkIfFileMatchesLayer(CT::string layerPathToScan, CServerConfig::XMLE_Layer *layer) {
  // Get the directory of the file to scan:
  CT::string directoryOfFileToScan = layerPathToScan = CDirReader::makeCleanPath(layerPathToScan.c_str());
  directoryOfFileToScan.substringSelf(0, directoryOfFileToScan.length() - CT::basename(directoryOfFileToScan).length());
  directoryOfFileToScan = CDirReader::makeCleanPath(directoryOfFileToScan.c_str()) + "/";

  if (layer->attr.type.empty() || layer->attr.type.equals("database")) {
    if (layer->FilePath.size() > 0) {
      CT::string filePath = CDirReader::makeCleanPath(layer->FilePath[0]->value.c_str());
      // Directories need to end with a /
      CT::string filePathWithTrailingSlash = filePath + "/";
      CT::string filter = layer->FilePath[0]->attr.filter;
      // When the FilePath in the Layer configuration is exactly the same as the file to scan, give a Match
      if (layerPathToScan.equals(filePath)) {
        return true;
        // When the directory of the file to scan matches the FilePath and the filter matches, give a Match
      } else if (directoryOfFileToScan.startsWith(filePathWithTrailingSlash.c_str())) {
        if (CDirReader::testRegEx(CT::basename(layerPathToScan).c_str(), filter.c_str()) == 1) {
          return true;
        }
      }
    }
  }
  return false;
}

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
  CT::string directoryOfFileToScan = layerPathToScan = CDirReader::makeCleanPath(layerPathToScan.c_str());
  directoryOfFileToScan.substringSelf(0, directoryOfFileToScan.length() - CT::basename(directoryOfFileToScan).length());
  directoryOfFileToScan = CDirReader::makeCleanPath(directoryOfFileToScan.c_str()) + "/";
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
