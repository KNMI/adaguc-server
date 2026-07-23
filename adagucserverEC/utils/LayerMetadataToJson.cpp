#include "LayerMetadataToJson.h"
#include <Types/LayerMetadataType.h>
#include "LayerMetadataStore.h"
#include <CDBStore.h>
#include <CDBFactory.h>
#include "LayerUtils.h"
#include "CRequestUtils.h"
#include "XMLGenUtils.h"
#include <unordered_set>
#include <traceTimings/traceTimings.h>

// metadataKey -> blob, for a single dataset/layer combination.
using LayerMetadataBlobs = std::map<std::string, std::string>;
// datasetName -> layerName -> metadataKey -> blob.
using DatasetMetadataIndex = std::map<std::string, std::map<std::string, LayerMetadataBlobs>>;

static std::string getBlob(const LayerMetadataBlobs &metadataBlobs, const char *metadataKey) {
  auto it = metadataBlobs.find(metadataKey);
  return it == metadataBlobs.end() ? "" : it->second;
}

static DatasetMetadataIndex buildDatasetMetadataIndex(CDBStore::Store *layerMetaDataStore) {
  DatasetMetadataIndex index;
  for (const auto &record: layerMetaDataStore->records) {
    const std::string &datasetName = record.get("datasetname");
    const std::string &layerName = record.get("layername");
    if (datasetName.empty() || layerName.empty()) {
      continue;
    }
    index[datasetName][layerName][record.get("metadatakey")] = record.get("blob");
  }
  return index;
}

std::map<std::string, std::vector<std::string>> getAllDimensionCombinationsFromDb(CDataSource dataSource) {
  std::map<std::string, std::vector<std::string>> dimensionNameAndValues;
  CRequest::fillDimValuesForDataSource(&dataSource, dataSource.srvParams);
  std::vector<COGCDims> newRequiredDims;
  if (dataSource.requiredDims.size() == 0) {
    return dimensionNameAndValues;
  }
  // Set other dims to * if not set in the request.
  for (auto dim: dataSource.requiredDims) {

    if (dim.queryValue.empty()) {
      newRequiredDims.push_back({.name = dim.name,
                                 .queryValue = "*",
                                 .value = "*",
                                 .netCDFDimName = dim.netCDFDimName,
                                 .uniqueValues = dim.uniqueValues,
                                 .isATimeDimension = dim.isATimeDimension,
                                 .hasFixedValue = dim.hasFixedValue,
                                 .hidden = dim.hidden});
    } else {
      newRequiredDims.push_back(dim);
    }
  }
  dataSource.requiredDims = newRequiredDims;

  CDBStore::Store *store = CDBFactory::getDBAdapter(dataSource.srvParams->cfg)->getFilesAndIndicesForDimensions(&dataSource, -1, false);
  if (store == nullptr || store->records.size() == 0) {
    delete store;
    CDBWarning("getAllDimensionCombinationsFromDb: No results for %s", dataSource.layerName.c_str());
    throw ServiceExceptionType::InvalidDimensionValue;
  }
  for (size_t d = 0; d < dataSource.requiredDims.size(); d++) {
    const auto &reqDim = dataSource.requiredDims[d];
    auto &reqDimList = dimensionNameAndValues[reqDim.name];
    std::unordered_set<std::string> seenValues(reqDimList.begin(), reqDimList.end());
    for (const auto &record: store->records) {
      const std::string &dimValueFromDb = record.values.at(1 + d * 2);
      // Only add if not already there.
      if (seenValues.insert(dimValueFromDb).second) {
        reqDimList.push_back(dimValueFromDb);
      }
    }
  }
  delete store;

  return dimensionNameAndValues;
}

json querySpecificDims(CServerParams *srvParams, const std::string &layerName) {
  if (srvParams->verbose) {
    CDBDebug("Start query specific dims for [%s]", layerName.c_str());
  }
  auto it = std::find_if(srvParams->cfg->Layer.begin(), srvParams->cfg->Layer.end(), [&layerName](auto *a) { return makeUniqueLayerName(a) == layerName; });

  if ((*it)->Variable.size() == 0) {
    CDBError("No variables defined");
    return 1;
  }
  CDataSource dataSource;
  auto index = it - srvParams->cfg->Layer.begin();
  dataSource.setCFGLayer(srvParams, (*it), index);
  if (dataSource.dataObjects.size() == 0) {
    CDBError("No dataobjects defined for %s", dataSource.layerName.c_str());
    return 1;
  }

  auto dimCombiAndData = getAllDimensionCombinationsFromDb(dataSource);

  std::vector<LayerMetadataDim> dimList;
  getDimsForLayer(&dataSource, dimList, dimCombiAndData);
  if (dataSource.srvParams->verbose) {
    CDBDebug("Succesfully queried specific dims for [%s]", layerName.c_str());
  }
  return getDimensionListAsJson(dimList);
}

json makeMetadataForDataSet(const std::map<std::string, LayerMetadataBlobs> &layerBlobsByLayer, const std::string &dataSetName, CServerParams *srvParams) {
  json datasetJSON;
  std::string layerNameInRequest;
  if (srvParams->requestedLayerNames.size() > 0) {
    layerNameInRequest = srvParams->requestedLayerNames[0];
  }
  std::string hasReferenceTimeValue;
  auto it = std::find_if(srvParams->requestDims.begin(), srvParams->requestDims.end(), [](const auto &a) { return a.name == "REFERENCE_TIME"; });
  if (it != srvParams->requestDims.end()) {
    hasReferenceTimeValue = (*it).value;
  }

  for (const auto &[layerName, metadataBlobs]: layerBlobsByLayer) {
    // CDBDebug("Checking %s = %s", layerNameInRequest.c_str(), layerName.c_str());
    if (layerNameInRequest.empty() || layerNameInRequest == layerName) {

      try {
        json layer;
        json a;
        layer["layer"] = a.parse(getBlob(metadataBlobs, "layermetadata").c_str());
        // do a query to find the matching times to the given reference time
        if (!hasReferenceTimeValue.empty()) {
          try {
            layer["dims"] = querySpecificDims(srvParams, layerName);
          } catch (...) {
            CDBWarning("No results for %s / %s. Using defaults.", dataSetName.c_str(), layerName.c_str());
            layer["dims"] = a.parse(getBlob(metadataBlobs, "dimensionlist").c_str());
          }
        } else {
          layer["dims"] = a.parse(getBlob(metadataBlobs, "dimensionlist").c_str());
        }
        // layer["styles"] = a.parse(getBlob(metadataBlobs, "stylelist").c_str());
        // layer["projected_extents"] = a.parse(getBlob(metadataBlobs, "projected_extents").c_str());

        datasetJSON[layerName] = layer;

      } catch (json::exception &e) {
        CDBWarning("Unable to parse json for layer %s", layerName.c_str());
      }
    }
  }
  return datasetJSON;
}

ServiceExceptionType getLayerMetadataAsJson(CServerParams *srvParams, json &result) {
  std::string datasetLocation = srvParams->datasetLocation;
  traceTimingsSpanStart(TraceTimingType::GETMETADATAJSONDB);
  CDBStore::Store *layerMetaDataStore = CDBFactory::getDBAdapter(srvParams->cfg)->getLayerMetadataStore(nullptr);
  traceTimingsSpanEnd(TraceTimingType::GETMETADATAJSONDB);
  if (layerMetaDataStore == nullptr) {
    CDBError("Unable to get layer metadata store");
    return ServiceExceptionType::UnprocessableEntity;
  }
  traceTimingsSpanStart(TraceTimingType::GETMETADATAJSONPARSE);
  // CDBDebug("Found %lu records in database", layerMetaDataStore->records.size());
  DatasetMetadataIndex metadataByDataset = buildDatasetMetadataIndex(layerMetaDataStore);

  // The following example demonstrates how the getmetadata call can be used with DIM_REFERENCE_TIME.
  // dataset=adaguc.tests.wikgeneric&&service=wms&version=1.3.0&request=getmetadata&format=application/json&DIM_REFERENCE_TIME=2025-11-21T00:00:00Z
  // dataset=adaguc.tests.wikgeneric&&service=wms&version=1.3.0&request=getmetadata&format=application/json&DIM_REFERENCE_TIME=2025-11-21T00:00:00Z&time=2025-11-21T06:00:00Z

  if (!datasetLocation.empty()) {
    auto it = metadataByDataset.find(datasetLocation);
    if (it == metadataByDataset.end()) {
      CDBError("DataSet not found");
      setExceptionType(ServiceExceptionType::InvalidDataset);
      return ServiceExceptionType::InvalidDataset;
    }
    // List single dataset.
    try {
      result[datasetLocation] = makeMetadataForDataSet(it->second, datasetLocation, srvParams);
    } catch (...) {
      CDBError("Unable to make metadata for dataset");
      setExceptionType(ServiceExceptionType::InvalidDimensionValue);
      return ServiceExceptionType::InvalidDimensionValue;
    }
  } else {
    // List all datasets.
    for (const auto &[dataSetName, layerBlobsByLayer]: metadataByDataset) {
      result[dataSetName] = makeMetadataForDataSet(layerBlobsByLayer, dataSetName, srvParams);
    }
  }
  traceTimingsSpanEnd(TraceTimingType::GETMETADATAJSONPARSE);
  return ServiceExceptionType::OK;
}
