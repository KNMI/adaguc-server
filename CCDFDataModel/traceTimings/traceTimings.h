#ifndef TRACE_TIMINGS_H
#define TRACE_TIMINGS_H
#include <stdio.h>
#include <iostream>
#include <map>
#include <chrono>
#include "CTString.h"

enum TraceTimingType { DBCONNECT, DBCLOSE, DBCHECKTABLE, DB, FSREADVAR, FSOPEN, APP, WARPIMAGERENDER, WARPIMAGE };

struct TraceTimingsInfo {
  uint64_t total;
  int numevents;
};

typedef std::map<TraceTimingType, TraceTimingsInfo> TraceTimingsReport;

/**
 * Enabled and initialize.
 */

void traceTimingsEnableAndInit();

/**
 * This checks if tracing is enabled and measures the current time. Place at the start of the application.
 */
void traceTimingsCheckEnabled();

/**
 * Place the span start function before a method to start tracing
 */
void traceTimingsSpanStart(TraceTimingType type);

/**
 * Place the span end function after a method to measure the time it took.
 */
void traceTimingsSpanEnd(TraceTimingType type);

/**
 * Returns a report with the cumulative time taken for each type.
 */
CT::string traceTimingsGetReport();

/**
 * Returns a report in HTTP HEADER form with the cumulative time taken for each type.
 */
CT::string traceTimingsGetHeader();

TraceTimingsReport traceTimingsGetMap();

#endif
