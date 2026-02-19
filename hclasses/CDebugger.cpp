/******************************************************************************
 *
 * Project:  Helper classes
 * Purpose:  Generic functions
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

#include "CDebugger.h"
#include <iostream>
#include <unistd.h>

unsigned int logMessageNumber = 0;

unsigned long logProcessIdentifier = getpid();

#include "CTString.h"
/*
 * If these prototypes are changed, also change the extern
 * declarations in CReporter.cpp that are referring to the
 * pointers declared here.
 */
void (*_printErrorStreamPointer)(const char *) = &_printErrorStream;
void (*_printDebugStreamPointer)(const char *) = &_printDebugStream;
void (*_printWarningStreamPointer)(const char *) = &_printWarningStream;

// TODO: logProcessIdentifier gets executed only once by the parent. Child logging uses the pid from parent
void printDebugStream(const char *message) { _printDebugStreamPointer(message); }
void printWarningStream(const char *message) { _printWarningStreamPointer(message); }
void printErrorStream(const char *message) { _printErrorStreamPointer(message); }

void _printErrorStream(const char *pszMessage) { fprintf(stderr, "%s", pszMessage); }
void _printWarningStream(const char *pszMessage) { fprintf(stderr, "%s", pszMessage); }
void _printDebugStream(const char *pszMessage) { printf("%s", pszMessage); }
// void _printDebugStream(const char *pszMessage) { fprintf(stderr, "%s", pszMessage); }

void _printDebugLine(const char *a, ...) {
  logMessageNumber++;
  std::string buf(CDEBUGGER_PRINT_BUFFER_SIZE + 1, '\0');
  va_list ap;
  va_start(ap, a);
  int numWritten = vsnprintf(buf.data(), buf.size(), a, ap);
  va_end(ap);
  if (numWritten > CDEBUGGER_PRINT_BUFFER_SIZE) {
    buf.resize(numWritten + 1);
    va_list ap;
    va_start(ap, a);
    numWritten = vsnprintf(buf.data(), buf.size(), a, ap);
    va_end(ap);
  }
  buf.resize(numWritten);
  buf += "\n";
  printDebugStream(buf.c_str());
}

void _printWarningLine(const char *a, ...) {
  logMessageNumber++;
  std::string buf(CDEBUGGER_PRINT_BUFFER_SIZE + 1, '\0');
  va_list ap;
  va_start(ap, a);
  int numWritten = vsnprintf(buf.data(), buf.size(), a, ap);
  va_end(ap);
  if (numWritten > CDEBUGGER_PRINT_BUFFER_SIZE) {
    buf.resize(numWritten + 1);
    va_list ap;
    va_start(ap, a);
    numWritten = vsnprintf(buf.data(), buf.size(), a, ap);
    va_end(ap);
  }
  buf.resize(numWritten);
  buf += "\n";
  printWarningStream(buf.c_str());
}

void _printErrorLine(const char *a, ...) {
  logMessageNumber++;
  std::string buf(CDEBUGGER_PRINT_BUFFER_SIZE + 1, '\0');
  va_list ap;
  va_start(ap, a);
  int numWritten = vsnprintf(buf.data(), buf.size(), a, ap);
  va_end(ap);
  if (numWritten > CDEBUGGER_PRINT_BUFFER_SIZE) {
    buf.resize(numWritten + 1);
    va_list ap;
    va_start(ap, a);
    numWritten = vsnprintf(buf.data(), buf.size(), a, ap);
    va_end(ap);
  }
  buf.resize(numWritten);
  buf += "\n";
  printErrorStream(buf.c_str());
}

void makeEqualWidth(std::string &t1) {
  size_t i = t1.length();
  for (int j = i; j < CDEBUGGER_FILE_LINENUMBER_WIDTH; j++) {
    t1 += (" ");
  }
}
void _printDebug(const char *a, ...) {
  std::string buf(CDEBUGGER_PRINT_BUFFER_SIZE + 1, '\0');
  va_list ap;
  va_start(ap, a);
  int numWritten = vsnprintf(buf.data(), buf.size(), a, ap);
  va_end(ap);
  if (numWritten > CDEBUGGER_PRINT_BUFFER_SIZE) {
    buf.resize(numWritten + 1);
    va_list ap;
    va_start(ap, a);
    numWritten = vsnprintf(buf.data(), buf.size(), a, ap);
    va_end(ap);
  }
  buf.resize(numWritten);
  makeEqualWidth(buf);
  printDebugStream(buf.c_str());
}

void _printWarning(const char *a, ...) {
  std::string buf(CDEBUGGER_PRINT_BUFFER_SIZE + 1, '\0');
  va_list ap;
  va_start(ap, a);
  int numWritten = vsnprintf(buf.data(), buf.size(), a, ap);
  va_end(ap);
  if (numWritten > CDEBUGGER_PRINT_BUFFER_SIZE) {
    buf.resize(numWritten + 1);
    va_list ap;
    va_start(ap, a);
    numWritten = vsnprintf(buf.data(), buf.size(), a, ap);
    va_end(ap);
  }
  buf.resize(numWritten);
  makeEqualWidth(buf);
  printWarningStream(buf.c_str());
}

void _printError(const char *a, ...) {
  std::string buf(CDEBUGGER_PRINT_BUFFER_SIZE + 1, '\0');
  va_list ap;
  va_start(ap, a);
  int numWritten = vsnprintf(buf.data(), buf.size(), a, ap);
  va_end(ap);
  if (numWritten > CDEBUGGER_PRINT_BUFFER_SIZE) {
    buf.resize(numWritten + 1);
    va_list ap;
    va_start(ap, a);
    numWritten = vsnprintf(buf.data(), buf.size(), a, ap);
    va_end(ap);
  }
  buf.resize(numWritten);
  makeEqualWidth(buf);
  printErrorStream(buf.c_str());
}

void setDebugFunction(void (*function)(const char *)) { _printDebugStreamPointer = function; }
void setWarningFunction(void (*function)(const char *)) { _printWarningStreamPointer = function; }
void setErrorFunction(void (*function)(const char *)) { _printErrorStreamPointer = function; }
