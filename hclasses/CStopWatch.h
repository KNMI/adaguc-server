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

#ifndef CSTOPWATCH_H
#define CSTOPWATCH_H
#include "CDebugger.h"

// Timing output is switched on/off centrally with STOPWATCH_MEASURETIME in CDebugger.h

void StopWatch_Start();
void _StopWatch_Measure(const char *a, ...);
extern unsigned int logMessageNumber;
extern unsigned long logProcessIdentifier;
#ifdef STOPWATCH_MEASURETIME
#define StopWatch_Measure(...)                                                                                                                                                                         \
  do {                                                                                                                                                                                                 \
    _printDebug("[D:%03d:pid%lu: %s:%d] ", logMessageNumber, logProcessIdentifier, __FILENAME__, __LINE__);                                                                                            \
    _StopWatch_Measure(__VA_ARGS__);                                                                                                                                                                   \
  } while (0)
#else
#define StopWatch_Measure(...)                                                                                                                                                                         \
  do {                                                                                                                                                                                                 \
  } while (0)
#endif
#endif
