#include "cdfVariableCache.h"
#include <map>
#include <string>
#include "CCDFObject.h"
#include <cstddef>

static std::map<std::string, CDF::Variable *> datamap;

void varCacheClear() {
  for (auto iter : datamap) {
    CDBDebug("Clear Cache [%s]", iter.first.c_str());
    delete iter.second;
    datamap.erase(iter.first);
  }
}

std::string _makeCacheKey(CDF::Variable *var, size_t *start, size_t *count, ptrdiff_t *stride) {
  if (var->getParentCDFObject()->currentFile.empty()) {
    return "";
  }
  std::string dimensionlinks;
  if (start != nullptr && count != nullptr && stride != nullptr) {
    size_t di = 0;
    for (auto &dim : var->dimensionlinks) {
      CT::printfconcat(dimensionlinks, "[%lu: %s %lu %lu %lu]", di, dim->getName().c_str(), start[di], count[di], stride[di]);
      di++;
    }
  } else {
    dimensionlinks = "Full read";
  }
  return CT::printf("[file:%s][var:%s][id:%d][type:%d][dims:%s]", var->getParentCDFObject()->currentFile.c_str(), var->name.c_str(), var->id, var->getType(), dimensionlinks.c_str());
}

int varCacheReturn(CDF::Variable *var, size_t *start, size_t *count, ptrdiff_t *stride) {
  if (!var->enableCache) {
    return 1;
  }
  auto key = _makeCacheKey(var, start, count, stride);
  if (key.empty()) return 1;
  if (datamap.contains(key)) {
    if (var->data == nullptr && datamap[key]->data != nullptr) {
      var->copy(datamap[key]);
      return 0;
    }
    delete datamap[key];
    datamap.erase(key);
  }
  return 1;
}

void varCacheAdd(CDF::Variable *var, size_t *start, size_t *count, ptrdiff_t *stride) {
  if (!var->enableCache) {
    return;
  }
  auto key = _makeCacheKey(var, start, count, stride);
  if (var->data != nullptr && !datamap.contains(key)) {
    datamap[key] = var->clone(var->getType(), var->name.c_str());
    datamap[key]->copy(var);
    CDBDebug("Made Cache [%s]", key.c_str());
  }
}