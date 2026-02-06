#include "cdfVariableCache.h"
#include <map>
#include <string>
#include "CCDFObject.h"
#include <cstddef>

static std::map<std::string, bool> datamap;

void varCacheClear() { datamap.clear(); }

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
  return CT::printf("[file:%s][var:%s][id:%d][type:%d][dims:%s][pointer:%p]", cdfVariable->getParentCDFObject()->currentFile.c_str(), cdfVariable->name.c_str(), cdfVariable->id,
                    cdfVariable->getType(), dimensionlinks.c_str(), cdfVariable->data);
}

int varCacheReturn(CDF::Variable *cdfVariable, size_t *start, size_t *count, ptrdiff_t *stride) {
  auto key = _makeCacheKey(cdfVariable, start, count, stride);
  if (key.empty()) return 1;
  if (datamap.contains(key) && datamap[key] == true) {
    if (cdfVariable->data != nullptr) {
      return 0;
    }
    datamap.erase(key);
  }
  return 1;
}

void varCacheAdd(CDF::Variable *cdfVariable, size_t *start, size_t *count, ptrdiff_t *stride) {
  auto key = _makeCacheKey(cdfVariable, start, count, stride);
  if (cdfVariable->data != nullptr && !datamap.contains(key)) {
    datamap[key] = true;
    CDBDebug("Made Cache [%s]", key.c_str());
  }
}
