#ifndef TRACE_TIMINGS_H
#define TRACE_TIMINGS_H
#include "CTString.h"

enum TimingTraceType { DB, FS, APP };

void traceTimingsCheckInit();

void traceTimingsSpanStart(TimingTraceType type);
void traceTimingsSpanEnd(TimingTraceType type);

CT::string traceTimingsGetHeader();

#endif
