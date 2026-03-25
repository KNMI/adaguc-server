#include "./ConfigurationUtils.h"
#include <CAutoResource.h>
#include "parseQueryString.h"
#include <algorithm>
#include <ranges>
#include <vector>
#include <iostream>

std::vector<std::string> getEnabledDatasetsConfigurations(CServerParams *srvParam) {
  std::vector<std::string> datasetList;
  if (srvParam->cfg->Dataset.size() == 0) {
    CDBWarning("No dataset paths are configured");
  }
  for (auto dataset: srvParam->cfg->Dataset) {
    if (dataset->attr.enabled.equals("true") && dataset->attr.location.empty() == false) {
      if (srvParam->verbose) {
        CDBDebug("Checking dataset location %s", dataset->attr.location.c_str());
      }
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
    }

    int status = parseAndCheckConfigFile(configWithAdditionalDataset, request->getServerParams());

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

int parseAndCheckConfigFile(std::string configFile, CServerParams *srvParam) {
  if (configFile.empty()) {
    CDBError("No config file set");
    return 1;
  }
  // The configfile can be a comma separated list of configuration files
  std::vector<std::string> configFileList = CT::split(configFile, ",");

  // Parse the main configuration file
  int status = srvParam->parseConfigFile(configFileList[0]);
  if (status != 0 || srvParam->configObj->Configuration.size() != 1) {
    CDBError("Invalid XML file %s", configFile.c_str());
    return 1;
  }

  srvParam->configFileName = configFile;
  srvParam->cfg = srvParam->configObj->Configuration[0];

  // Include additional config files given as argument
  if (configFileList.size() > 1) {
    for (size_t j = 1; j < configFileList.size() - 1; j++) {
      if (srvParam->verbose) {
        CDBDebug("Include '%s'", configFileList[j].c_str());
      }
      status = srvParam->parseConfigFile(configFileList[j]);
      if (status != 0) {
        CDBError("There is an error with include '%s'", configFileList[j].c_str());
        return 1;
      }
    }

    // The last configration file is considered the dataset one, strip path and extension and give it to configurer
    srvParam->datasetLocation = CT::basename(configFileList[configFileList.size() - 1]);
    srvParam->datasetLocation.substringSelf(0, srvParam->datasetLocation.lastIndexOf("."));
    if (srvParam->verbose) {
      CDBDebug("Dataset name based on passed configfile is [%s]", srvParam->datasetLocation.c_str());
    }

    status = CAutoResource::configureDataset(srvParam, false);
    if (status != 0) {
      CDBError("ConfigureDataset failed for %s", configFileList[1].c_str());
      return status;
    }
  }

  // Check if a dataset was given in the querystring
  if (getDatasetAndSldFromQueryString(*srvParam) != 0) {
    return 1;
  }
  // Include additional config files given in the include statement of the config file, last config file is included first
  for (const auto &include: std::views::reverse(srvParam->cfg->Include)) {
    if (include->attr.location.empty() == false) {
      if (srvParam->verbose) {
        CDBDebug("Include '%s'", include->attr.location.c_str());
      }
      status = srvParam->parseConfigFile(include->attr.location);
      if (status != 0) {
        CDBError("There is an error with include '%s'", include->attr.location.c_str());
        return 1;
      }
    }
  }

  // Check for mandatory attributes in Layer
  int layerIndex = -1;
  for (const auto &layer: srvParam->cfg->Layer) {
    layerIndex++;
    if (layer->attr.type.equals("database")) {
      if (layer->Variable.size() == 0) {
        CDBError("Configuration error at layer %d: <Variable> not defined", layerIndex);
        return 1;
      }
      if (layer->FilePath.size() == 0) {
        CDBError("Configuration error at layer %d: <FilePath> not defined", layerIndex);
        return 1;
      }
    }
  }

  // Check for autoscan elements
  layerIndex = -1;
  for (const auto &layer: srvParam->cfg->Layer) {
    layerIndex++;
    if (layer->attr.type.equals("autoscan")) {
      if (layer->FilePath.size() == 0) {
        CDBError("Configuration error at layer %d: <FilePath> not defined", layerIndex);
        return 1;
      }
      try {
        /* Create the list of layers from a directory list */
        const char *baseDir = layer->FilePath[0]->value.c_str();

        CDBDebug("autoscan");
        std::vector<std::string> fileList;
        try {
          fileList = CDBFileScanner::searchFileNames(baseDir, layer->FilePath[0]->attr.filter.c_str(), NULL);
        } catch (int linenr) {
          CDBError("Could not find any file in directory '%s'", baseDir);
          throw(__LINE__);
        }

        if (fileList.size() == 0) {
          CDBError("Could not find any file in directory '%s'", baseDir);
          throw(__LINE__);
        }
        size_t nrOfFileErrors = 0;
        for (const auto &fileName: fileList) {
          try {
            std::string baseDirStr = baseDir;
            std::string groupName = CT::substring(fileName, baseDirStr.length(), -1);

            // Open file
            // CDBDebug("Opening file %s",fileName.c_str());
            CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(NULL, srvParam, fileName.c_str());
            if (cdfObject == NULL) {
              CDBError("Unable to read file %s", fileName.c_str());
              throw(__LINE__);
            }

            // std::vector<CT::string> variables;
            // List variables
            for (size_t v = 0; v < cdfObject->variables.size(); v++) {
              CDF::Variable *var = cdfObject->variables[v];
              if (var->isDimension == false) {
                if (var->dimensionlinks.size() >= 2) {
                  CServerConfig::XMLE_Layer *xmleLayer = new CServerConfig::XMLE_Layer();
                  CServerConfig::XMLE_Group *xmleGroup = new CServerConfig::XMLE_Group();
                  CServerConfig::XMLE_Variable *xmleVariable = new CServerConfig::XMLE_Variable();
                  CServerConfig::XMLE_FilePath *xmleFilePath = new CServerConfig::XMLE_FilePath();
                  xmleLayer->attr.type = "database";
                  xmleVariable->value = var->name;
                  xmleFilePath->value = fileName;
                  xmleGroup->attr.value = groupName;
                  xmleLayer->Variable.push_back(xmleVariable);
                  xmleLayer->FilePath.push_back(xmleFilePath);
                  xmleLayer->Group.push_back(xmleGroup);
                  srvParam->cfg->Layer.push_back(xmleLayer);
                }
              }
            }
          } catch (int e) {
            nrOfFileErrors++;
          }
        }
        if (nrOfFileErrors != 0) {
          CDBError("%lu files are not readable", nrOfFileErrors);
        }
      } catch (int line) {
        return 1;
      }
    }
  }

  return status;
}
