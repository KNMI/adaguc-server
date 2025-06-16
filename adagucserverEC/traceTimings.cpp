#include <stdio.h>
#include <iostream>
#include <map>
#include <chrono>

#include "traceTimings.h"
#include "CDebugger.h"

bool adagucTraceTimings = false;
uint64_t tracingStart = 0;

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
      tracingStart = micros();
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
  // Should not be zero, but it could happen. TODO, what if it does?
  if (timingsMapRel[type].size() == 0) {
    return;
  }
  auto current = micros();
  auto last = timingsMapRel[type].back();

  timingsMapRel[type].pop_back();
  auto relative = (current - last);
  if (timingsMapTotal.find(type) == timingsMapTotal.end()) {
    timingsMapTotal[type] = {.total = 0, .numevents = 0};
  };

  timingsMapTotal[type].total += relative;
  timingsMapTotal[type].numevents++;
}

CT::string typeToString(TimingTraceType typein) {
  CT::string type;
  switch (typein) {
  case TimingTraceType::DB:
    return "DB";
  case TimingTraceType::DBCONNECT:
    return "DBCONNECT";
  case TimingTraceType::DBCHECKTABLE:
    return "DBCHECKTABLE";
  case TimingTraceType::DBCLOSE:
    return "DBCLOSE";
  case TimingTraceType::FSREADVAR:
    return "FSREADVAR";
  case TimingTraceType::WARPIMAGERENDER:
    return "WARPIMAGE.RENDER";
  case TimingTraceType::WARPIMAGE:
    return "WARPIMAGE";
  case TimingTraceType::FSOPEN:
    return "FSOPEN";
  case TimingTraceType::APP:
    return "APP";
  default:
    return "?";
  }
}

CT::string traceTimingsGetInfo() {
  if (!adagucTraceTimings) return "";

  CT::string summary;

  for (auto timingsInfo : timingsMapTotal) {
    auto type = typeToString(timingsInfo.first);
    if (summary.length() > 0) {
      summary.concat(",");
    }
    auto allPopped = timingsMapRel[timingsInfo.first].size();
    summary.printconcat("[%s %0.1f/%d/%d]", type.c_str(), timingsInfo.second.total / 1000., timingsInfo.second.numevents, allPopped);
  }

  auto tracingTotal = micros() - tracingStart;
  summary.printconcat(",[total %0.1fms]", tracingTotal / 1000.);
  CDBDebug("Tracing summary: %s", summary.c_str());
  return summary;
}

CT::string traceTimingsGetHeader() {
  if (!adagucTraceTimings) return "";
  CT::string summary = traceTimingsGetInfo();
  return CT::string("\r\nX-Trace-Timings: ") + summary;
}
