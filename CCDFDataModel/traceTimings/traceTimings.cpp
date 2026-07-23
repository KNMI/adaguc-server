

#include "traceTimings.h"
#include "CDebugger.h"
#include "CTString.h"

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
    std::string check = ADAGUC_TRACE_TIMINGS;
    if (CT::equalsIgnoreCase(check, "true")) {
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
  auto &starts = timingsMapRel[type];
  // Should always be defined, but it could theoratically not be. TODO, what if it does?
  if (starts.empty()) {
    return;
  }
  auto current = micros();
  auto last = starts.back();
  starts.pop_back();
  auto relative = (current - last);
  if (timingsMapTotal.find(type) == timingsMapTotal.end()) {
    timingsMapTotal[type] = {.total = 0, .numevents = 0};
  };

  timingsMapTotal[type].total += relative;
  timingsMapTotal[type].numevents++;
}

std::string typeToString(TraceTimingType typein) {
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
  case TraceTimingType::GETMETADATAJSONREQ:
    return "GETMETADATAJSONREQ";
  case TraceTimingType::GETMETADATAJSONDB:
    return "GETMETADATAJSONDB";
  case TraceTimingType::GETMETADATAJSONPARSE:
    return "GETMETADATAJSONPARSE";
  default:
    return "?";
  }
}

std::string traceTimingsGetReport() {
  if (!adagucTraceTimings) return "";

  std::string summary;

  for (const auto &timingsInfo: timingsMapTotal) {
    auto type = typeToString(timingsInfo.first);
    if (summary.length() > 0) {
      summary += ",";
    }
    auto allPopped = timingsMapRel[timingsInfo.first].size();
    CT::printfconcat(summary, "[%s %0.1f/%d/%zu]", type.c_str(), timingsInfo.second.total / 1000., timingsInfo.second.numevents, allPopped);
  }

  auto tracingTotal = micros() - tracingStart;
  auto tracingTotalMs = tracingTotal / 1000.;
  CT::printfconcat(summary, ",[total %0.1fms] {%0.0f}", tracingTotalMs, tracingTotalMs);
  return summary;
}

std::string traceTimingsGetHeader() {
  if (!adagucTraceTimings) return "";
  std::string summary = traceTimingsGetReport();
  return "\r\nX-Trace-Timings: " + summary;
}

TraceTimingsReport traceTimingsGetMap() { return timingsMapTotal; }
