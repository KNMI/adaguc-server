/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl, GST - GeoSpatialTeam KNMI
 * Date:     2026-09-10
 *
 ******************************************************************************
 *
 * Copyright 2026, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include "CStopWatch.h"
#include "CTString.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sys/time.h>
/* Stopwatch functions for timing */

extern unsigned int logMessageNumber;
extern unsigned long logProcessIdentifier;

bool adagucMeasureTime = false;

void checkMeasureTimeEnabled() {
  const char *env = getenv("ADAGUCENV_MEASURETIME");
  if (env != NULL && CT::equalsIgnoreCase(env, "true")) {
    adagucMeasureTime = true;
  }
}

timespec starttime, stoptime, currenttime;
double CSTOPWATCH_H_prevTime = 0;
int content_type_provided = 0;
int firstTime = 0;

void StopWatch_Start() {
#if _POSIX_TIMERS > 0
  clock_gettime(CLOCK_REALTIME, &starttime);
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  starttime.tv_sec = tv.tv_sec;
  starttime.tv_nsec = tv.tv_usec * 1000;
#endif
  double stop;
  stop = double(stoptime.tv_nsec) / 1000000 + stoptime.tv_sec * 1000;
  CSTOPWATCH_H_prevTime = stop;
}

void __StopWatch_Stop(const char *msg) {
#if _POSIX_TIMERS > 0
  clock_gettime(CLOCK_REALTIME, &stoptime);
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  stoptime.tv_sec = tv.tv_sec;
  stoptime.tv_nsec = tv.tv_usec * 1000;
#endif
  double start, stop;
  start = double(starttime.tv_nsec) / 1000000 + starttime.tv_sec * 1000;
  stop = double(stoptime.tv_nsec) / 1000000 + stoptime.tv_sec * 1000;
  if (firstTime == 0) {
    CSTOPWATCH_H_prevTime = stop;
    firstTime = 1;
  }
  _printDebugLine("[T] %5.1f ms %5.3f ms: %s", stop - start, stop - CSTOPWATCH_H_prevTime, msg);
  CSTOPWATCH_H_prevTime = stop;
}

static std::string formatMessage(const char *format, va_list ap) {
  std::string buf(300, '\0');
  va_list apCopy;
  va_copy(apCopy, ap);
  int numWritten = vsnprintf(buf.data(), buf.size() + 1, format, apCopy);
  va_end(apCopy);
  if (numWritten < 0) return "";
  if ((size_t)numWritten > buf.size()) {
    buf.resize(numWritten);
    vsnprintf(buf.data(), buf.size() + 1, format, ap);
  } else {
    buf.resize(numWritten);
  }
  return buf;
}

void _StopWatch_Stop(const char *a, ...) {
  va_list ap;
  va_start(ap, a);
  std::string message = formatMessage(a, ap);
  va_end(ap);
  __StopWatch_Stop(message.c_str());
}

void _printDebugLineMeasured(const char *a, ...) {
  va_list ap;
  va_start(ap, a);
  std::string message = formatMessage(a, ap);
  va_end(ap);
  if (adagucMeasureTime) {
    // Same output as StopWatch_Stop, so debug messages show up in the timing overview
    __StopWatch_Stop(message.c_str());
    return;
  }
  logMessageNumber++;
  message += "\n";
  printDebugStream(message.c_str());
}
