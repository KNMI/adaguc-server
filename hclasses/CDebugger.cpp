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

#include "CDebugger_H.h"
#include "CDebugger_H2.h"
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <new>   



bool Tracer::Ready = false;
extern Tracer NewTrace;
Tracer NewTrace;

extern unsigned int logMessageNumber;
unsigned int logMessageNumber = 0;

extern unsigned long logProcessIdentifier;
unsigned long logProcessIdentifier = getpid();

void Tracer::Add (void * p, char const * file, int line)
{
  if (_lockCount > 0)
    return;
  Tracer::Lock lock (*this);
  _map [p] = Entry (file, line);
}
void Tracer::Remove (void * p)
{
  if (_lockCount > 0)
    return;

  Tracer::Lock lock (*this);

  iterator it = _map.find (p);
  if (it != _map.end ())
  {
    _map.erase (it);
  }
}
Tracer::~Tracer ()
{
  Ready = false;
  Dump ();
}

int Tracer::Dump ()
{
  int status = 0;
  if (_map.size () != 0)
  {
    //std::cout << _map.size () << " memory leaks detected\n";
    for (iterator it = _map.begin (); it != _map.end (); ++it)
    {
      char const * file = it->second.File ();
      int line = it->second.Line ();
      if(line!=0){
        status++;
        _printErrorLine("*** Memory leak in %s, line %d",file,line);
        //std::cout << "*** Memory leak in " << file << ", line "  << line << std::endl;
      }
    }
  }
  _map.clear();
  return status;
}

Tracer::Tracer () 
  : _lockCount (0) 
{
  Ready = true;
}


void * operator new (size_t size, char const * file, int line)
{
  void * p = malloc (size);
  if (Tracer::Ready)
    NewTrace.Add (p, file, line);
  return p;
}
void * operator new[] (size_t size, char const * file, int line)
{
  void * p = malloc (size);
  if (Tracer::Ready)
    NewTrace.Add (p, file, line);
  return p;
}

void operator delete (void * p, char const * file, int line)
{
  if (Tracer::Ready)
    NewTrace.Remove (p);
  free (p);
}
void * operator new (std::size_t mem)
{
  void * p = malloc (mem);
  if (Tracer::Ready)
    NewTrace.Add (p, "?", 0);
  return p;
}
void operator delete (void * p)
{
  if (Tracer::Ready)
    NewTrace.Remove (p);
  free (p);
}

#include "CTypes.h"
/*
 * If these prototypes are changed, also change the extern
 * declarations in CReporter.cpp that are referring to the
 * pointers declared here.
 */
void (*_printErrorStreamPointer)(const char*)=&_printErrorStream;
void (*_printDebugStreamPointer)(const char*)=&_printDebugStream;
void (*_printWarningStreamPointer)(const char*)=&_printWarningStream;

void printDebugStream(const char* message){
  _printDebugStreamPointer(message);
}
void printWarningStream(const char* message){
  _printWarningStreamPointer(message);
}
void printErrorStream(const char* message){
  _printErrorStreamPointer(message);
}

void _printErrorStream(const char *pszMessage){
    
  fprintf(stderr,"%s",pszMessage);
}
void _printWarningStream(const char *pszMessage){
  fprintf(stderr,"%s",pszMessage);
}
void _printDebugStream(const char *pszMessage){
  //fprintf(stdout,"%s",pszMessage);
  printf("%s", pszMessage);
}

void _printDebugLine(const char *pszMessage,...){
  logMessageNumber++;
  char szTemp[2048];
  va_list ap;
  va_start (ap, pszMessage);
  vsnprintf ( szTemp, 2047,pszMessage, ap);
  printDebugStream(szTemp);
  va_end (ap);
  printDebugStream("\n");
}

void _printWarningLine(const char *pszMessage,...){
  logMessageNumber++;
  char szTemp[2048];
  va_list ap;
  va_start (ap, pszMessage);
  vsnprintf ( szTemp, 2047,pszMessage, ap);
  printWarningStream(szTemp);
  va_end (ap);
  printWarningStream("\n");
}

void _printErrorLine(const char *pszMessage,...){
  logMessageNumber++;
  char szTemp[2048];
  va_list ap;
  va_start (ap, pszMessage);
  vsnprintf ( szTemp, 2047,pszMessage, ap);
  printErrorStream(szTemp);
  va_end (ap);
  printErrorStream("\n");
}

void makeEqualWidth(CT::string *t1){
  //int i=t.indexOf("]")+1;
  size_t i=t1->length();
  //CT::string t1=t.substringr(0,i);
  //CT::string t2=t.substringr(i,-1);
  //size_t l=t1.length();
  for(int j=i;j<72;j++){t1->concat(" ");}
  // t1.concat(&t2);
}
void _printDebug(const char *pszMessage,...){
  char szTemp[2048];
  va_list ap;
  va_start (ap, pszMessage);
  vsnprintf ( szTemp, 2047,pszMessage, ap);
  CT::string t1=szTemp;
  makeEqualWidth(&t1); 
  printDebugStream(t1.c_str());
  va_end (ap);
}

void _printWarning(const char *pszMessage,...){
  char szTemp[2048];
  va_list ap;
  va_start (ap, pszMessage);
  vsnprintf ( szTemp, 2047,pszMessage, ap);
  CT::string t1=szTemp;
  makeEqualWidth(&t1); 
  printWarningStream(t1.c_str());
  va_end (ap);
}

void _printError(const char *pszMessage,...){
  char szTemp[2048];
  va_list ap;
  va_start (ap, pszMessage);
  vsnprintf ( szTemp, 2047,pszMessage, ap);
  CT::string t1=szTemp;
  makeEqualWidth(&t1); 
  printErrorStream(t1.c_str());
  va_end (ap);
}


void setDebugFunction(void (*function)(const char*) ){
  _printDebugStreamPointer=function;
}
void setWarningFunction(void (*function)(const char*) ){
  _printWarningStreamPointer=function;
}
void setErrorFunction(void (*function)(const char*) ){
  _printErrorStreamPointer=function;
}
