#include "./lintDataset.h"
#include <CDebugger.h>
#include "ConfigurationUtils.h"
#include <list>
int lintOutputEnabled = 0;

void removeStringFromVector(const std::string &name, std::vector<std::string> &list) {
  auto toErase = std::find(list.begin(), list.end(), name);
  if (toErase != list.end()) list.erase(toErase);
}

std::vector<std::string> getDuplicates(std::vector<std::string> &inputList) {
  std::vector<std::string> duplicateLegends;
  std::sort(inputList.begin(), inputList.end());
  std::set<std::string> uvec(inputList.begin(), inputList.end());
  set_difference(inputList.begin(), inputList.end(), uvec.begin(), uvec.end(), std::back_inserter(duplicateLegends));
  return duplicateLegends;
}

int lintDataset(std::string config) {
  lintOutputEnabled = 1;

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

  std::string datasetName = (CT::split(config, ",").back());
  // Check for missing styles or legend
  for (auto Layer: serverParamsAll.cfg->Layer) {
    CDataSource dataSource(&serverParamsAll, Layer);
    auto styleList = dataSource.getStyleListForDataSource();
    for (const auto &style: styleList) {
      if (style.styleName.size() > 0) {
        if (std::find(availableStyleNames.begin(), availableStyleNames.end(), style.styleName) == availableStyleNames.end()) {
          CDBLint("ERROR: In dataset \"%s\": layer \"%s\", Style [%s] not defined", CT::basename(datasetName).c_str(), dataSource.getLayerName(), style.styleName.c_str());
          numXMLAttributesNotRecognized++;
        }
      }
      if (style.legendName.size() > 0) {
        if (std::find(availableLegendNames.begin(), availableLegendNames.end(), style.legendName) == availableLegendNames.end()) {
          CDBLint("ERROR: In dataset \"%s\": layer \"%s\", Legend [%s] not defined", CT::basename(datasetName).c_str(), dataSource.getLayerName(), style.legendName.c_str());
          numXMLAttributesNotRecognized++;
        }
      }
    }
  }

  // List duplicates
  auto duplicateStyles = getDuplicates(availableStyleNames);
  auto duplicateLegends = getDuplicates(availableLegendNames);
  for (const auto &style: duplicateStyles) {
    CDBLint("ERROR: In dataset \"%s\": duplicate style [%s] detected ", CT::basename(datasetName).c_str(), style.c_str());
    numXMLAttributesNotRecognized++;
  }

  for (const auto &legend: duplicateLegends) {
    CDBLint("ERROR: In dataset \"%s\": duplicate legend [%s] detected ", CT::basename(datasetName).c_str(), legend.c_str());
    numXMLAttributesNotRecognized++;
  }
  // Remove the ones already configured in the base
  for (auto Legend: srvParamsBase.cfg->Legend) {
    removeStringFromVector(Legend->attr.name, availableLegendNames);
  }

  for (auto Style: srvParamsBase.cfg->Style) {
    removeStringFromVector(Style->attr.name, availableStyleNames);
  }

  // Remove the ones with standardname mappings, these cannot be properly checked.
  for (auto Style: srvParamsBase.cfg->Style) {
    if (Style->StandardNames.size() > 0) {
      removeStringFromVector(Style->attr.name, availableStyleNames);
    }
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

  if (availableLegendNames.size() > 0) {
    CDBLint("[Note] In dataset \"%s\": no uses for (%lu) legend(s) [%s]", CT::basename(datasetName).c_str(), availableLegendNames.size(), CT::join(availableLegendNames).c_str());
  }
  if (availableStyleNames.size() > 0) {
    CDBLint("[NOTE] In dataset \"%s\": no uses for (%lu) style(s) [%s] ", CT::basename(datasetName).c_str(), availableStyleNames.size(), CT::join(availableStyleNames).c_str());
  }

  int numIssues = numXMLAttributesNotRecognized;
  if (numIssues > 0) {
    CDBLint("Found (%d) issues in dataset [%s]", numIssues, datasetName.c_str());
    return 1;
  }
  CDBLint("No issues found in dataset [%s]", datasetName.c_str());
  return 0;
}
