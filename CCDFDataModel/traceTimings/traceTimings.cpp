

#include "traceTimings.h"
#include "CDebugger.h"

bool adagucTraceTimings = false;
uint64_t tracingStart = 0;

static std::map<TraceTimingType, std::vector<uint64_t>> timingsMapRel;

static TraceTimingsReport timingsMapTotal;

// Get time stamp in microseconds.
uint64_t micros() {
  uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  return us;
}
void traceTimingsCheckEnabled() {
  const char *ADAGUC_TRACE_TIMINGS = getenv("ADAGUC_TRACE_TIMINGS");
  if (ADAGUC_TRACE_TIMINGS != NULL) {
    CT::string check = ADAGUC_TRACE_TIMINGS;
    if (check.equalsIgnoreCase("true")) {
      traceTimingsEnableAndInit();
    }
  }
}

void traceTimingsEnableAndInit() {
  adagucTraceTimings = true;
  tracingStart = micros();
}

void traceTimingsSpanStart(TraceTimingType type) {
  if (!adagucTraceTimings) return;
  auto current = micros();
  timingsMapRel[type].push_back(current);
}

void traceTimingsSpanEnd(TraceTimingType type) {
  if (!adagucTraceTimings) return;
  // Should always be defined, but it could theoratically not be. TODO, what if it does?
  if (timingsMapRel[type].empty()) {
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

CT::string typeToString(TraceTimingType typein) {
  CT::string type;
  switch (typein) {
  case TraceTimingType::DB:
    return "DB";
  case TraceTimingType::DBCONNECT:
    return "DBCONNECT";
  case TraceTimingType::DBCHECKTABLE:
    return "DBCHECKTABLE";
  case TraceTimingType::DBCLOSE:
    return "DBCLOSE";
  case TraceTimingType::FSREADVAR:
    return "FSREADVAR";
  case TraceTimingType::WARPIMAGERENDER:
    return "WARPIMAGE.RENDER";
  case TraceTimingType::WARPIMAGE:
    return "WARPIMAGE";
  case TraceTimingType::FSOPEN:
    return "FSOPEN";
  case TraceTimingType::APP:
    return "APP";
  case TraceTimingType::GETMETADATAJSON:
    return "GETMETADATAJSON";
  default:
    return "?";
  }
}

CT::string traceTimingsGetReport() {
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
  auto tracingTotalMs = tracingTotal / 1000.;
  summary.printconcat(",[total %0.1fms] {%0.0f}", tracingTotalMs, tracingTotalMs);
  return summary;
}

CT::string traceTimingsGetHeader() {
  if (!adagucTraceTimings) return "";
  CT::string summary = traceTimingsGetReport();
  return CT::string("\r\nX-Trace-Timings: ") + summary;
}

TraceTimingsReport traceTimingsGetMap() { return timingsMapTotal; }
