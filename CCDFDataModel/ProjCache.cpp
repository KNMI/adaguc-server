#include <map>
#include "ProjCache.h"


PJ *proj_create_crs_to_crs_with_cache(CT::string source_crs, CT::string target_crs, PJ_AREA *area) {
  // NOTE: This will not work when called from multiple threads!!
  // Cache is held till Adaguc stops running, so no explicit cleanup is needed
  static std::map<std::string, PJ *> projections;

  source_crs.trimSelf();
  target_crs.trimSelf();

  std::string key = (source_crs + CT::string(":") + target_crs).c_str();
  PJ *projSourceToDest;
  if (projections.count(key)) {
    projSourceToDest = projections[key];
  } else {
    projSourceToDest = proj_create_crs_to_crs(PJ_DEFAULT_CTX, source_crs.c_str(), target_crs.c_str(), area);
    projections[key] = projSourceToDest;
  }

  return projSourceToDest;
}
