/* 
 * Copyright (C) 2012, Royal Netherlands Meteorological Institute (KNMI)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or any 
 * later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Project    : ADAGUC
 *
 * initial programmer :  M.Plieger
 * initial date       :  20120610
 */

#include "CDebugger_H.h"
#include "CDebugger_H2.h"
#include <iostream>
#include <stdlib.h>
 
bool Tracer::Ready = false;
extern Tracer NewTrace;
Tracer NewTrace;

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

void Tracer::Dump ()
{
  if (_map.size () != 0)
  {
    //std::cout << _map.size () << " memory leaks detected\n";
    for (iterator it = _map.begin (); it != _map.end (); ++it)
    {
      char const * file = it->second.File ();
      int line = it->second.Line ();
      if(line!=0){
        _printErrorLine("*** Memory leak in %s, line %d",file,line);
        //std::cout << "*** Memory leak in " << file << ", line "  << line << std::endl;
      }
    }
  }
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
void * operator new (size_t size)
{
  void * p = malloc (size);
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
  fprintf(stdout,"%s",pszMessage);
}

void _printDebugLine(const char *pszMessage,...){
  char szTemp[1024];
  va_list ap;
  va_start (ap, pszMessage);
  vsnprintf ( szTemp, 1023,pszMessage, ap);
  printDebugStream(szTemp);
  va_end (ap);
  printDebugStream("\n");
}

void _printWarningLine(const char *pszMessage,...){
  char szTemp[1024];
  va_list ap;
  va_start (ap, pszMessage);
  vsnprintf ( szTemp, 1023,pszMessage, ap);
  printWarningStream(szTemp);
  va_end (ap);
  printWarningStream("\n");
}

void _printErrorLine(const char *pszMessage,...){
  char szTemp[1024];
  va_list ap;
  va_start (ap, pszMessage);
  vsnprintf ( szTemp, 1023,pszMessage, ap);
  printErrorStream(szTemp);
  va_end (ap);
  printErrorStream("\n");
}
void _printDebug(const char *pszMessage,...){
  char szTemp[1024];
  va_list ap;
  va_start (ap, pszMessage);
  vsnprintf ( szTemp, 1023,pszMessage, ap);
  printDebugStream(szTemp);
  va_end (ap);
}

void _printWarning(const char *pszMessage,...){
  char szTemp[1024];
  va_list ap;
  va_start (ap, pszMessage);
  vsnprintf ( szTemp, 1023,pszMessage, ap);
  printWarningStream(szTemp);
  va_end (ap);
}

void _printError(const char *pszMessage,...){
  char szTemp[1024];
  va_list ap;
  va_start (ap, pszMessage);
  vsnprintf ( szTemp, 1023,pszMessage, ap);
  printErrorStream(szTemp);
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
