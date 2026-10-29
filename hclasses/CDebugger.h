/******************************************************************************
 *
 * Project:  Helper classes
 * Purpose:  Generic functions
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

#ifndef CDEBUGGER_H
#define CDEBUGGER_H

// Central switch for timing output. When STOPWATCH_MEASURETIME is defined, CDBDebug lines are prefixed with time statistics and StopWatch_Measure is enabled.
// When STOPWATCH_MEASURETIME is undefined, StopWatch_Measure calls and their arguments are removed at compile time.
// Enable it with ./compile.sh --measuretime (CMake option -DADAGUC_MEASURETIME=ON), or by uncommenting the define below.
// This lives here and not in adagucserverEC/Definitions.h, because hclasses and CCDFDataModel can not include that file.
// #define STOPWATCH_MEASURETIME

#ifndef SOURCE_PATH_SIZE
#define SOURCE_PATH_SIZE 70
#endif

#define CDEBUGGER_FILE_LINENUMBER_WIDTH 100
#define CDEBUGGER_PRINT_BUFFER_SIZE 200

#define __FILENAME__ (&__FILE__[SOURCE_PATH_SIZE])

#include "printfCheckMacro.h"
// Used to silence -Wunused-parameter warnings
template <class T> void ignoreParameter(const T &) {}

extern unsigned int logMessageNumber;
extern unsigned long logProcessIdentifier;

void _printDebugStream(const char *message);
void _printWarningStream(const char *message);
void _printErrorStream(const char *message);

void printDebugStream(const char *message);
void printWarningStream(const char *message);
void printErrorStream(const char *message);

void setDebugFunction(void (*function)(const char *));
void setWarningFunction(void (*function)(const char *));
void setErrorFunction(void (*function)(const char *));

void _printDebugLine(const char *pszMessage, ...) PRINTF_FORMAT_CHECK(1, 2);
void _printWarningLine(const char *pszMessage, ...) PRINTF_FORMAT_CHECK(1, 2);
void _printErrorLine(const char *pszMessage, ...) PRINTF_FORMAT_CHECK(1, 2);

void _printDebug(const char *pszMessage, ...) PRINTF_FORMAT_CHECK(1, 2);
void _printWarning(const char *pszMessage, ...) PRINTF_FORMAT_CHECK(1, 2);
void _printLint(const char *pszMessage, ...) PRINTF_FORMAT_CHECK(1, 2);
void _printError(const char *pszMessage, ...) PRINTF_FORMAT_CHECK(1, 2);

// Prints the time since StopWatch_Start and since the previous timed message, implemented in CStopWatch.cpp
void _printStopWatchTime();

#define CDBWarning                                                                                                                                                                                     \
  _printWarning("[W:%03d:pid%lu: %s:%d] ", logMessageNumber, logProcessIdentifier, __FILENAME__, __LINE__);                                                                                            \
  _printWarningLine
#define CDBLint                                                                                                                                                                                        \
  _printLint("[LINT]: ");                                                                                                                                                                              \
  _printWarningLine
#define CDBError                                                                                                                                                                                       \
  _printError("[E:%03d:pid%lu: %s:%d] ", logMessageNumber, logProcessIdentifier, __FILENAME__, __LINE__);                                                                                              \
  _printErrorLine
#ifdef STOPWATCH_MEASURETIME
#define CDBDebug                                                                                                                                                                                       \
  _printDebug("[D:%03d:pid%lu: %s:%d] ", logMessageNumber, logProcessIdentifier, __FILENAME__, __LINE__);                                                                                              \
  _printStopWatchTime();                                                                                                                                                                               \
  _printDebugLine
#else
#define CDBDebug                                                                                                                                                                                       \
  _printDebug("[D:%03d:pid%lu: %s:%d] ", logMessageNumber, logProcessIdentifier, __FILENAME__, __LINE__);                                                                                              \
  _printDebugLine
#endif

#endif

/**
 * Set the logProcessIdentifier to `getpid`. Used in the fork server, so that child processes show their own PID in the logging.
 */
void setLoggerPid();