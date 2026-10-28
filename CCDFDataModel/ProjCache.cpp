#include <map>
#include "ProjCache.h"

// File scope
static std::map<std::string, PJ *> projections;

PJ *proj_create_crs_to_crs_with_cache(std::string source_crs, std::string target_crs, PJ_AREA *area) {
  // NOTE: This will not work when called from multiple threads!!

  // PROJ logs messages like "laea: Invalid latitude" to stderr whenever a transform lands outside a
  // projection's valid domain (e.g. warping a grid past a pole, or near a cut line). This is expected
  // and harmless here - the caller already detects the same failure through the returned inf/nan
  // coordinate and proj_context_errno(), so only the noisy log line is silenced, not error detection.
  // Idempotent and cheap (sets a flag on the context), so no need to guard this to run only once.
  proj_log_level(PJ_DEFAULT_CTX, PJ_LOG_NONE);

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
