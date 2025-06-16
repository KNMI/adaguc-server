#include <stdio.h>
#include <iostream>
#include <map>
#include <chrono>

#include "traceTimings.h"
#include "CDebugger.h"

bool adagucTraceTimings = false;

std::map<TimingTraceType, std::vector<uint64_t>> timingsMapRel;

struct TraceInfo {
  uint64_t total;
  int numevents;
};

std::map<TimingTraceType, TraceInfo> timingsMapTotal;

// Get time stamp in microseconds.
uint64_t micros() {
  uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  return us;
}
void traceTimingsCheckInit() {
  const char *ADAGUC_TRACE_TIMINGS = getenv("ADAGUC_TRACE_TIMINGS");
  if (ADAGUC_TRACE_TIMINGS != NULL) {
    CT::string check = ADAGUC_TRACE_TIMINGS;
    if (check.equalsIgnoreCase("true")) {
      adagucTraceTimings = true;
    }
  }
}

void traceTimingsSpanStart(TimingTraceType type) {
  if (!adagucTraceTimings) return;
  auto current = micros();
  timingsMapRel[type].push_back(current);
}

void traceTimingsSpanEnd(TimingTraceType type) {
  if (!adagucTraceTimings) return;
  auto current = micros();
  auto last = timingsMapRel[type].back();
  timingsMapRel[type].pop_back();
  auto relative = (current - last);
  if (timingsMapTotal.find(type) == timingsMapTotal.end()) {
    timingsMapTotal[type] = {.total = 0, .numevents = 0};
  };

  timingsMapTotal[type].total += relative;
  timingsMapTotal[type].numevents++;
  //   CDBDebug("traceTimingsSpanEnd: %u %d ", timingsMapTotal[type].total, timingsMapTotal[type].numevents);
}

CT::string traceTimingsGetHeader() {
  if (!adagucTraceTimings) return "";

  return "\r\nX-Trace-Timings: test";
}
