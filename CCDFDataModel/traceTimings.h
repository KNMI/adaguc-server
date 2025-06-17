#ifndef TRACE_TIMINGS_H
#define TRACE_TIMINGS_H
#include "CTString.h"

enum TimingTraceType { DBCONNECT, DBCLOSE, DBCHECKTABLE, DB, FSREADVAR, FSOPEN, APP, WARPIMAGERENDER, WARPIMAGE };

void traceTimingsCheckInit();

void traceTimingsSpanStart(TimingTraceType type);
void traceTimingsSpanEnd(TimingTraceType type);

CT::string traceTimingsGetInfo();
CT::string traceTimingsGetHeader();

#endif
