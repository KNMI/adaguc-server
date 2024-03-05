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

extern unsigned int logMessageNumber;
unsigned int logMessageNumber = 0;

extern unsigned long logProcessIdentifier;
unsigned long logProcessIdentifier = getpid();

#include "CTypes.h"
/*
 * If these prototypes are changed, also change the extern
 * declarations in CReporter.cpp that are referring to the
 * pointers declared here.
 */
void (*_printErrorStreamPointer)(const char *) = &_printErrorStream;
void (*_printDebugStreamPointer)(const char *) = &_printDebugStream;
void (*_printWarningStreamPointer)(const char *) = &_printWarningStream;

void printDebugStream(const char *message) { _printDebugStreamPointer(message); }
void printWarningStream(const char *message) { _printWarningStreamPointer(message); }
void printErrorStream(const char *message) { _printErrorStreamPointer(message); }

void _printErrorStream(const char *pszMessage) { fprintf(stderr, "%s", pszMessage); }
void _printWarningStream(const char *pszMessage) { fprintf(stderr, "%s", pszMessage); }
void _printDebugStream(const char *pszMessage) { printf("%s", pszMessage); }

void _printDebugLine(const char *pszMessage, ...) {
  logMessageNumber++;
  char szTemp[2048];
  va_list ap;
  va_start(ap, pszMessage);
  vsnprintf(szTemp, 2047, pszMessage, ap);
  printDebugStream(szTemp);
  va_end(ap);
  printDebugStream("\n");
}

void _printWarningLine(const char *pszMessage, ...) {
  logMessageNumber++;
  char szTemp[2048];
  va_list ap;
  va_start(ap, pszMessage);
  vsnprintf(szTemp, 2047, pszMessage, ap);
  printWarningStream(szTemp);
  va_end(ap);
  printWarningStream("\n");
}

void _printErrorLine(const char *pszMessage, ...) {
  logMessageNumber++;
  char szTemp[2048];
  va_list ap;
  va_start(ap, pszMessage);
  vsnprintf(szTemp, 2047, pszMessage, ap);
  printErrorStream(szTemp);
  va_end(ap);
  printErrorStream("\n");
}

void makeEqualWidth(CT::string *t1) {
  size_t i = t1->length();
  for (int j = i; j < CDEBUGGER_FILE_LINENUMBER_WIDTH; j++) {
    t1->concat(" ");
  }
}
void _printDebug(const char *pszMessage, ...) {
  char szTemp[2048];
  va_list ap;
  va_start(ap, pszMessage);
  vsnprintf(szTemp, 2047, pszMessage, ap);
  CT::string t1 = szTemp;
  makeEqualWidth(&t1);
  printDebugStream(t1.c_str());
  va_end(ap);
}

void _printWarning(const char *pszMessage, ...) {
  char szTemp[2048];
  va_list ap;
  va_start(ap, pszMessage);
  vsnprintf(szTemp, 2047, pszMessage, ap);
  CT::string t1 = szTemp;
  makeEqualWidth(&t1);
  printWarningStream(t1.c_str());
  va_end(ap);
}

void _printError(const char *pszMessage, ...) {
  char szTemp[2048];
  va_list ap;
  va_start(ap, pszMessage);
  vsnprintf(szTemp, 2047, pszMessage, ap);
  CT::string t1 = szTemp;
  makeEqualWidth(&t1);
  printErrorStream(t1.c_str());
  va_end(ap);
}

void setDebugFunction(void (*function)(const char *)) { _printDebugStreamPointer = function; }
void setWarningFunction(void (*function)(const char *)) { _printWarningStreamPointer = function; }
void setErrorFunction(void (*function)(const char *)) { _printErrorStreamPointer = function; }
