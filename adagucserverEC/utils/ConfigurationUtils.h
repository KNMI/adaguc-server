#ifndef CONFIGURATIONUTILS_H
#define CONFIGURATIONUTILS_H

#include <vector>
#include <string>
#include <CServerParams.h>
#include <CRequest.h>

std::vector<std::string> getEnabledDatasetsConfigurations(CServerParams *srvParam);

bool checkIfPathIsFile(std::string filePath);

int setCRequestConfigFromEnvironment(CRequest *request, std::string additionalDataset = "");

int parseAndCheckConfigFile(std::string configFile, CServerParams *srvParam);
#endif
