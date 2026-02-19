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
#include "writelogfile.h"
#include <cstdio>
#include <cstddef>
#include <ctime>
#include <cstring>
#include <sys/time.h>
#include <cstdlib>
#include <string>
#include <CTString.h>
#include <CServerError.h>
#include <unistd.h>

FILE *pLogDebugFile = NULL;
enum LogBufferMode { LogBufferMode_TRUE, LogBufferMode_FALSE, LogBufferMode_DISABLELOGGING };
LogBufferMode logMode = LogBufferMode::LogBufferMode_FALSE;

void writeLogFile(const char *msg) {
  if (logMode == LogBufferMode_DISABLELOGGING) return;
  if (pLogDebugFile != NULL) {
    fputs(msg, pLogDebugFile);
    // If message line contains data like [D:008:pid250461: adagucserverEC/CCairoPlotter.cpp:878], also append the time.
    if (strncmp(msg, "[D:", 3) == 0 || strncmp(msg, "[W:", 3) == 0 || strncmp(msg, "[E:", 3) == 0) {
      char szTemp[128];
      struct timeval tv;
      gettimeofday(&tv, NULL);
      time_t curtime = tv.tv_sec;
      tm *myUsableTime = localtime(&curtime);
      snprintf(szTemp, 127, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3dZ ", myUsableTime->tm_year + 1900, myUsableTime->tm_mon + 1, myUsableTime->tm_mday, myUsableTime->tm_hour, myUsableTime->tm_min,
               myUsableTime->tm_sec, int(tv.tv_usec / 1000));
      fputs(szTemp, pLogDebugFile);
    }
  }
}

void checkLogSettings() {
  /* Check if ADAGUC_LOGFILE is set */
  const char *ADAGUC_LOGFILE = getenv("ADAGUC_LOGFILE");
  if (ADAGUC_LOGFILE != NULL) {
    pLogDebugFile = fopen(ADAGUC_LOGFILE, "a");
    if (pLogDebugFile == NULL) {
      fprintf(stderr, "Unable to write ADAGUC_LOGFILE %s\n", ADAGUC_LOGFILE);
    }
  }

  /* Check if we enable logbuffer:
     - FALSE (unset / default): unbuffered output with live logging but can have a slower service as effect
     - TRUE: buffered logging
     - DISABLELOGGING: no logging at all
   */
  const char *ADAGUC_ENABLELOGBUFFER = getenv("ADAGUC_ENABLELOGBUFFER");
  if (ADAGUC_ENABLELOGBUFFER != NULL) {
    std::string check = ADAGUC_ENABLELOGBUFFER;
    if (CT::equalsIgnoreCase(check, "true")) {
      logMode = LogBufferMode::LogBufferMode_TRUE;
    }
    if (CT::equalsIgnoreCase(check, "DISABLELOGGING")) {
      logMode = LogBufferMode::LogBufferMode_DISABLELOGGING;
    }
  }

  if (pLogDebugFile != NULL && logMode == LogBufferMode_FALSE) {
    // Set line buffering on log file
    setvbuf(pLogDebugFile, NULL, _IONBF, 1024);
  }
}

void closeLogFile() {
  if (pLogDebugFile != NULL) {
    fclose(pLogDebugFile);
    pLogDebugFile = NULL;
  }
}

void logBufferCheckMode() {
  if (logMode == LogBufferMode_FALSE) {
    // Set line buffering on stdout
    setvbuf(stdout, NULL, _IOLBF, 1024);
  }
}

void writeErrorFile(const char *msg) { writeLogFile(msg); }

/* Function pointers called by CDebugger */
void serverDebugFunction(const char *msg) {
  writeLogFile(msg);
  printdebug(msg, 1);
}
/* Function pointers called by CDebugger */
void serverErrorFunction(const char *msg) {
  writeErrorFile(msg);
  printerror(msg);
}
/* Function pointers called by CDebugger */
void serverWarningFunction(const char *msg) {
  writeLogFile(msg);
  printdebug(msg, 1);
}

void serverLogFunctionCMDLine(const char *msg) { printf("%s", msg); }