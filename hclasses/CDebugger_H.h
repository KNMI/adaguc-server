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

#ifndef CDEBUGGER_H_H
#define CDEBUGGER_H_H
#include <map>
#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <new>   
#include <stdarg.h>

void * operator new (size_t size, char const * file, int line);
void * operator new[] (size_t size, char const * file, int line);
void operator delete (void * p, char const * file, int line);
void * operator new (std::size_t mem,const std::nothrow_t&) ;
void operator delete (void * p,const std::nothrow_t&);

class Tracer
{
  private:
    class Entry
    {
      public:
        Entry (char const * file, int line)
        : _file (file), _line (line)
        {}
        Entry ()
        : _file (0), _line (0)
        {}
        char const * File () const { return _file; }
        int Line () const { return _line; }
      private:
        char const * _file;
        int _line;
    };
    class Lock
    {
      public:
        Lock (Tracer & tracer)
        : _tracer (tracer)
        {
          _tracer.lock ();
        }
        ~Lock ()
        {
          _tracer.unlock ();
        }
      private:
        Tracer & _tracer;
    };
    typedef std::map<void *, Entry>::iterator iterator;
    friend class Lock;
  public:
    Tracer ();
    ~Tracer ();
    void Add (void * p, char const * file, int line);
    void Remove (void * p);
    int Dump ();

    static bool Ready;
  private:
    void lock () { _lockCount++; }
    void unlock () { _lockCount--; }
  private:

    std::map<void *, Entry> _map;
    int _lockCount;
};

using namespace std;
/*
void _printErrorStream(const char *pszMessage);
void _printDebugStream(const char *pszMessage);
static void (*printErrorStream)(const char*)=&_printErrorStream;
static void (*printDebugStream)(const char*)=&_printDebugStream;*/





#endif



