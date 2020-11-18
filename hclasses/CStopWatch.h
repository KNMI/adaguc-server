/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
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
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "CDebugger.h"
#include <stdio.h>
#include <stdarg.h>

void StopWatch_Start();
void _StopWatch_Stop(const char *a, ...);
extern unsigned int logMessageNumber;
extern unsigned long logProcessIdentifier;
#define StopWatch_Stop  _printDebug("[D:%03d:pid%lu: %s, %d in %s] ",logMessageNumber, logProcessIdentifier, __FILE__,__LINE__,className);_StopWatch_Stop
#endif
