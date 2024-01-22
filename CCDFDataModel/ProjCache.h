#ifndef ADAGUC_SERVER_PROJCACHE_H
#define ADAGUC_SERVER_PROJCACHE_H
#include <proj.h>
#include "CTString.h"

PJ *proj_create_crs_to_crs_with_cache(CT::string source_crs, CT::string target_crs, PJ_AREA *area);
void proj_clear_cache();

#endif // ADAGUC_SERVER_PROJCACHE_H
