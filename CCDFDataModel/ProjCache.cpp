#include <map>
#include "ProjCache.h"

// File scope
static std::map<std::string, PJ *> projections;

PJ *proj_create_crs_to_crs_with_cache(std::string source_crs, std::string target_crs, PJ_AREA *area) {
  // NOTE: This will not work when called from multiple threads!!

  source_crs = CT::trim(source_crs);
  target_crs = CT::trim(target_crs);

  std::string key = source_crs + ":" + target_crs;
  PJ *projSourceToDest;
  if (projections.count(key)) {
    projSourceToDest = projections[key];
  } else {
    projSourceToDest = proj_create_crs_to_crs(PJ_DEFAULT_CTX, source_crs.c_str(), target_crs.c_str(), area);
    projections[key] = projSourceToDest;
  }

  return projSourceToDest;
}

void proj_clear_cache() {
  for (auto const &p: projections) {
    proj_destroy(p.second);
  }
}
