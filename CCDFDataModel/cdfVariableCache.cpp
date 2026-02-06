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
  }
  datamap.clear();
}

std::string _makeCacheKey(CDF::Variable *cdfVariable, size_t *start, size_t *count, ptrdiff_t *stride) {
  if (cdfVariable->getParentCDFObject()->currentFile.empty()) {
    return "";
  }
  std::string dimensionlinks;
  if (start != nullptr && count != nullptr && stride != nullptr) {
    size_t di = 0;
    for (auto &dim : cdfVariable->dimensionlinks) {
      CT::printfconcat(dimensionlinks, "[%lu: %s %lu %lu %lu]", di, dim->getName().c_str(), start[di], count[di], stride[di]);
      di++;
    }
  } else {
    dimensionlinks = "Full read";
  }
  return CT::printf("[file:%s][var:%s][id:%d][type:%d][dims:%s]", cdfVariable->getParentCDFObject()->currentFile.c_str(), cdfVariable->name.c_str(), cdfVariable->id, cdfVariable->getType(),
                    dimensionlinks.c_str());
}

int varCacheReturn(CDF::Variable *cdfVariable, size_t *start, size_t *count, ptrdiff_t *stride) {
  auto key = _makeCacheKey(cdfVariable, start, count, stride);
  if (key.empty()) return 1;
  if (datamap.contains(key)) {
    if (cdfVariable->data == nullptr && datamap[key]->data != nullptr) {
      cdfVariable->copy(datamap[key]);
      return 0;
    }
    delete datamap[key];
    datamap.erase(key);
  }
  return 1;
}

void varCacheAdd(CDF::Variable *cdfVariable, size_t *start, size_t *count, ptrdiff_t *stride) {
  auto key = _makeCacheKey(cdfVariable, start, count, stride);
  if (cdfVariable->data != nullptr && !datamap.contains(key)) {
    datamap[key] = cdfVariable->clone(cdfVariable->getType(), cdfVariable->name.c_str());
    datamap[key]->copy(cdfVariable);
    CDBDebug("Made Cache [%s]", key.c_str());
  }
}