#include "./lintDataset.h"
#include <CDebugger.h>
#include "ConfigurationUtils.h"

void removeStringFromVector(const std::string &name, std::vector<std::string> &list) {
  auto toErase = std::find(list.begin(), list.end(), name);
  if (toErase != list.end()) list.erase(toErase);
}

int lintDataset(std::string config) {

  // Parse base config without additional datasets included:
  CServerParams srvParamsBase;
  int status = parseAndCheckConfigFile(CT::split(config, ",")[0], &srvParamsBase);
  if (status != 0 || srvParamsBase.cfg == nullptr) {
    return 1;
  }

  // Parse all serverparams
  CServerParams serverParamsAll;
  status = parseAndCheckConfigFile(config, &serverParamsAll);
  if (status != 0 || serverParamsAll.cfg == nullptr) {
    return 1;
  }

  // Make a list of legends and styles
  std::vector<std::string> availableLegendNames;
  for (auto Legend: serverParamsAll.cfg->Legend) {
    availableLegendNames.push_back(Legend->attr.name);
  }
  std::vector<std::string> availableStyleNames;
  for (auto Style: serverParamsAll.cfg->Style) {
    availableStyleNames.push_back(Style->attr.name);
  }

  // Remove the ones already configured in the base
  for (auto Legend: srvParamsBase.cfg->Legend) {
    removeStringFromVector(Legend->attr.name, availableLegendNames);
  }

  for (auto Style: srvParamsBase.cfg->Style) {
    removeStringFromVector(Style->attr.name, availableStyleNames);
  }

  // Remove the ones used in each layer
  for (auto Layer: serverParamsAll.cfg->Layer) {
    CDataSource dataSource(&serverParamsAll, Layer);
    auto styleList = dataSource.getStyleListForDataSource();
    for (const auto &style: styleList) {
      removeStringFromVector(style.styleName, availableStyleNames);
      removeStringFromVector(style.legendName, availableLegendNames);
    }
  }
  std::string datasetName = (CT::split(config, ",").back());
  if (availableLegendNames.size() > 0) {
    CDBWarning("[LINT]: In [%s]: no references for legend(s) [%s]", CT::basename(datasetName).c_str(), CT::join(availableLegendNames).c_str());
  }
  if (availableStyleNames.size() > 0) {
    CDBWarning("[LINT]: In [%s]: no references for style(s) [%s] ", CT::basename(datasetName).c_str(), CT::join(availableStyleNames).c_str());
  }

  int numIssues = numXMLAttributesNotRecognized + availableLegendNames.size() + availableStyleNames.size();
  if (numIssues > 0) {
    CDBWarning("[LINT]: Found [%d] issues in dataset [%s]", numIssues, datasetName.c_str());
    return 1;
  }
  return 0;
}
