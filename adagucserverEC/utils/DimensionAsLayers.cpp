#include "DimensionAsLayers.h"
#include <algorithm>
#include <map>
#include <set>
#include <CDBFactory.h>
#include <CDBStore.h>
#include <CDebugger.h>
#include <CDataSource.h>
#include "CRequestUtils.h"
#include "LayerUtils.h"
#include "LayerMetadataStore.h"
#include "XMLGenUtils.h"

static const CServerConfig::XMLE_Dimension *findDimensionAsLayers(const CServerConfig::XMLE_Layer &layer) {
  auto it = std::ranges::find_if(layer.Dimension, [](const auto &dim) { return dim.attr.aslayers; });
  return it == layer.Dimension.end() ? nullptr : &(*it);
}

bool layerHasDimensionAsLayers(const CServerConfig::XMLE_Layer &layer) { return findDimensionAsLayers(layer) != nullptr; }

/** Makes a value usable in a layer name, e.g. 2026-11-12T08:09:22Z becomes 2026-11-12T080922Z */
static std::string makeLayerNameSuffix(const std::string &value) {
  std::string suffix;
  for (char c: value) {
    if (isalnum(c) || c == '-' || c == '.') {
      suffix += c;
    } else if (c != ':') {
      suffix += '_';
    }
  }
  return suffix;
}

/** Queries the distinct values of the dimension from the database, in ascending order. Layers with the same FilePath and filter share their values via the cache. */
static std::vector<std::string> queryDimensionValues(CServerParams *srvParam, const CServerConfig::XMLE_Layer &layer, const CServerConfig::XMLE_Dimension &dim, DimensionValuesCache *cache) {
  std::vector<std::string> values;
  if (layer.FilePath.empty()) return values;

  std::string cacheKey = layer.FilePath[0].elementValue + "\n" + layer.FilePath[0].attr.filter + "\n" + dim.attr.name;
  if (cache != nullptr) {
    auto cached = cache->find(cacheKey);
    if (cached != cache->end()) return cached->second;
  }

  CDataSource dataSource;
  if (dataSource.setCFGLayer(srvParam, const_cast<CServerConfig::XMLE_Layer *>(&layer), -1) != 0) return values;

  auto dbAdapter = CDBFactory::getDBAdapter(srvParam->cfg);
  if (dbAdapter == nullptr) return values;

  std::string dimName = CT::toLowerCase(dim.attr.name);
  std::string tableName;
  try {
    auto tables = dbAdapter->getTableNamesForPathFilterAndDimensions(layer.FilePath[0].elementValue, layer.FilePath[0].attr.filter, {dimName}, &dataSource);
    tableName = tables.at(dimName).tableName;
  } catch (...) {
    CDBWarning("Unable to find the table for dimension %s", dimName.c_str());
    return values;
  }

  bool isTimeDim = isATimeDimension(const_cast<CServerConfig::XMLE_Dimension *>(&dim));
  CDBStore::Store *store = isTimeDim ? dbAdapter->getUniqueValuesOrderedByValue(dimName.c_str(), 0, true, tableName.c_str())
                                     : dbAdapter->getUniqueValuesOrderedByIndex(dimName.c_str(), 0, true, tableName.c_str());
  if (store == nullptr) {
    // The table does not exist yet, the dataset has not been scanned.
    return values;
  }
  for (auto &record: store->records) {
    values.push_back(isTimeDim ? makeIsoStringFromDbString(record.get(0)) : record.get(0));
  }
  delete store;
  if (cache != nullptr) (*cache)[cacheKey] = values;
  return values;
}

/** Fixes and hides the dimension with the given service name in the layer. Returns false when the layer does not have it. */
static bool hasDimensionAsLayers(const CServerConfig::XMLE_Layer &layer, const std::string &serviceName) {
  return std::ranges::any_of(layer.Dimension, [&serviceName](const auto &dim) { return dim.attr.aslayers && dim.elementValue == serviceName; });
}

static bool fixDimension(CServerConfig::XMLE_Layer &layer, const std::string &serviceName, const std::string &value) {
  bool found = false;
  for (auto &dim: layer.Dimension) {
    if (dim.elementValue == serviceName) {
      dim.attr.fixvalue = value;
      dim.attr.hidden = true;
      dim.attr.aslayers = false;
      found = true;
    }
  }
  return found;
}

static CServerConfig::XMLE_Layer *findLayerByName(CServerParams *srvParam, const std::string &layerName) {
  for (auto &layer: srvParam->cfg->Layer) {
    if (makeUniqueLayerName(&layer) == layerName) return &layer;
  }
  return nullptr;
}

std::vector<CServerConfig::XMLE_Layer> makeLayersForDimensionValues(CServerParams *srvParam, const CServerConfig::XMLE_Layer &layer, DimensionValuesCache *cache) {
  std::vector<CServerConfig::XMLE_Layer> generatedLayers;
  const auto *dimAsLayers = findDimensionAsLayers(layer);
  if (dimAsLayers == nullptr) return generatedLayers;

  // Copy before anything else: makeUniqueLayerName may add a Name element to the layer it gets.
  CServerConfig::XMLE_Layer baseLayer = layer;
  std::string baseName = makeUniqueLayerName(&baseLayer);
  std::string baseNameWithoutGroup = baseLayer.Name[0].elementValue;
  std::string baseTitle = baseLayer.Title.empty() || baseLayer.Title[0].elementValue.empty() ? baseName : baseLayer.Title[0].elementValue;
  std::string serviceName = dimAsLayers->elementValue;

  auto values = queryDimensionValues(srvParam, baseLayer, *dimAsLayers, cache);
  if (values.empty()) {
    CDBDebug("No values found for dimension %s of layer %s, no layers generated", serviceName.c_str(), baseName.c_str());
    return generatedLayers;
  }

  std::set<std::string> generatedNames;
  for (const auto &value: values) {
    std::string suffix = makeLayerNameSuffix(value);
    CServerConfig::XMLE_Layer newLayer = baseLayer;
    newLayer.Name[0].elementValue = baseNameWithoutGroup + "_" + suffix;
    if (newLayer.Title.empty()) addXmlObj(newLayer.Title);
    newLayer.Title[0].elementValue = baseTitle + " " + value;
    fixDimension(newLayer, serviceName, value);

    // Additional layers do not take dimension values from the layer they are added to, but from the request.
    // Refer to the layer for this value instead: the one the additional layer generates itself when it has the same aslayers dimension,
    // otherwise a hidden copy with the dimension fixed.
    for (auto &additionalLayerRef: newLayer.AdditionalLayer) {
      auto *additionalLayer = findLayerByName(srvParam, additionalLayerRef.elementValue);
      if (additionalLayer == nullptr) continue;
      CServerConfig::XMLE_Layer additionalCopy = *additionalLayer;
      bool additionalLayerIsExpandedItself = hasDimensionAsLayers(additionalCopy, serviceName);
      if (!fixDimension(additionalCopy, serviceName, value)) continue;
      makeUniqueLayerName(&additionalCopy);
      additionalCopy.Name[0].elementValue += "_" + suffix;
      additionalLayerRef.elementValue = makeUniqueLayerName(&additionalCopy);
      if (additionalLayerIsExpandedItself) continue;
      additionalCopy.attr.hidden = "true";
      if (generatedNames.insert(additionalLayerRef.elementValue).second) {
        generatedLayers.push_back(std::move(additionalCopy));
      }
    }
    if (generatedNames.insert(makeUniqueLayerName(&newLayer)).second) {
      generatedLayers.push_back(std::move(newLayer));
    }
  }
  return generatedLayers;
}

int expandDimensionsAsLayers(CServerParams *srvParam) {
  if (srvParam == nullptr || srvParam->cfg == nullptr) return 1;
  auto &layers = srvParam->cfg->Layer;
  if (std::ranges::none_of(layers, layerHasDimensionAsLayers)) return 0;

  // Generate all layers first, the lookup of additional layers needs the original layer list.
  std::map<size_t, std::vector<CServerConfig::XMLE_Layer>> generatedLayersPerIndex;
  DimensionValuesCache cache;
  for (size_t j = 0; j < layers.size(); j++) {
    if (layerHasDimensionAsLayers(layers[j])) {
      generatedLayersPerIndex[j] = makeLayersForDimensionValues(srvParam, layers[j], &cache);
    }
  }

  std::vector<CServerConfig::XMLE_Layer> expandedLayers;
  std::set<std::string> layerNames;
  for (size_t j = 0; j < layers.size(); j++) {
    auto generated = generatedLayersPerIndex.find(j);
    if (generated == generatedLayersPerIndex.end()) {
      layerNames.insert(makeUniqueLayerName(&layers[j]));
      expandedLayers.push_back(std::move(layers[j]));
      continue;
    }
    for (auto &generatedLayer: generated->second) {
      // Additional layer copies can be shared by several generated layers
      if (layerNames.insert(makeUniqueLayerName(&generatedLayer)).second) {
        expandedLayers.push_back(std::move(generatedLayer));
      }
    }
  }
  layers = std::move(expandedLayers);
  return 0;
}
