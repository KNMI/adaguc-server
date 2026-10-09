#include "ThreadUtils.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <thread>

// For momentary testing: a NUMTHREADS=<n> key in QUERY_STRING overrides the detected thread count.
static int getNumThreadsFromQueryString() {
  const char *queryString = getenv("QUERY_STRING");
  if (queryString == nullptr) {
    return 0;
  }
  const char *needle = "NUMTHREADS=";
  const char *match = strcasestr(queryString, needle);
  if (match == nullptr || (match != queryString && match[-1] != '&')) {
    return 0;
  }
  int numThreads = atoi(match + strlen(needle));
  return numThreads > 0 ? numThreads : 0;
}

int getNumRenderThreads() {
  int numThreadsOverride = getNumThreadsFromQueryString();
  if (numThreadsOverride > 0) {
    return numThreadsOverride;
  }
  static const int maxThreads = 8;
  return std::max(1, std::min(maxThreads, (int)std::thread::hardware_concurrency()));
}
