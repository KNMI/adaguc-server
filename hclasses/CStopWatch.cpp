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

#include "CStopWatch.h"
#include "CTString.h"
/* Stopwatch functions for timing */

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
  _printDebugLine("[T] %.1f ms\t%.3f ms: %s", stop - start, stop - CSTOPWATCH_H_prevTime, msg);
  CSTOPWATCH_H_prevTime = stop;
}

void _StopWatch_Stop(const char *a, ...) {
  va_list ap;
  char szTemp[8192 + 1];
  va_start(ap, a);
  vsnprintf(szTemp, 8192, a, ap);
  va_end(ap);
  szTemp[8192] = '\0';
  /*
  CT::string t=szTemp;
  int i=t.indexOf("]")+1;
  CT::string t1=t.substringr(0,i);
  CT::string t2=t.substringr(i,-1);
  size_t l=t1.length();
  for(int j=l;j<40;j++){
    t1.concat(" ");
  }
  t1.concat(&t2);*/

  __StopWatch_Stop(szTemp);
}
