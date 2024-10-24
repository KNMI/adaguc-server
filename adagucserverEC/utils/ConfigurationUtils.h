#ifndef CONFIGURATIONUTILS_H
#define CONFIGURATIONUTILS_H

#include <vector>
#include <string>
#include <CServerParams.h>
#include <CRequest.h>

std::vector<std::string> getEnabledDatasetsConfigurations(CServerParams *srvParam);

bool checkIfPathIsFile(CT::string filePath);

int setCRequestConfigFromEnvironment(CRequest *request, CT::string additionalDataset = "");

#endif
