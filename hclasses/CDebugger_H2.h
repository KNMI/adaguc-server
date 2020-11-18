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

#ifndef CDEBUGGER_H_H2
#define CDEBUGGER_H_H2

#include <stdio.h>
#include <iostream>
#include <vector>

extern unsigned int logMessageNumber;
extern unsigned long logProcessIdentifier;

void _printDebugStream(const char* message);
void _printWarningStream(const char* message);
void _printErrorStream(const char* message);


void printDebugStream(const char* message);
void printWarningStream(const char* message);
void printErrorStream(const char* message);

void setDebugFunction(void (*function)(const char*) );
void setWarningFunction(void (*function)(const char*) );
void setErrorFunction(void (*function)(const char*) );

void _printDebugLine(const char *pszMessage,...);
void _printWarningLine(const char *pszMessage,...);
void _printErrorLine(const char *pszMessage,...);

void _printDebug(const char *pszMessage,...);
void _printWarning(const char *pszMessage,...);
void _printError(const char *pszMessage,...);



#define CDBWarning             _printWarning("[W:%03d:pid%lu: %s, %d in %s] ",logMessageNumber, logProcessIdentifier, __FILE__,__LINE__,className);_printWarningLine
#define CDBError               _printError("[E:%03d:pid%lu: %s, %d in %s] ",logMessageNumber, logProcessIdentifier, __FILE__,__LINE__,className);_printErrorLine
#define CDBErrormessage        _printErrorLine
#define CDBDebug               _printDebug("[D:%03d:pid%lu: %s, %d in %s] ",logMessageNumber, logProcessIdentifier, __FILE__,__LINE__,className);_printDebugLine
#define CDBEnterFunction(name) const char *functionName=name;_printDebugLine("D %s, %d class %s: Entering function '%s'",__FILE__,__LINE__,className,functionName);
#define CDBReturn(id)          {_printDebug("D %s, %d class %s::%s: returns %d\n",__FILE__,__LINE__,className,functionName,id);return id;}
#define CDBDebugFunction       _printDebug("D %s, %d class %s::%s: ",__FILE__,__LINE__,className,functionName);_printDebugLine
#define DEF_ERRORFUNCTION()    static const char *className;
#define DEF_ERRORMAIN()        static const char *className="main";



template <class myFreeType>
    void _allocateArray(myFreeType *object[], size_t elements,const char*file,const int line){
  if(*object!=NULL){
    delete[] *object;
    *object=NULL;
  }
#ifdef MEMLEAKCHECK
  *object = new(file,line)myFreeType[elements];
#else
  *object = new myFreeType[elements];
#endif
    }
      
#define allocateArray(object,size) _allocateArray(object,size,__FILE__,__LINE__)
//allocateArray(&nc.varids,nc.numvars);
template <class myFreeType>
    void deleteArray(myFreeType **object){
  if(*object!=NULL){
    delete[] *object;
    *object=NULL;
  }
}
template <class myFreeType>
    void deleteObject(myFreeType *object){
  if(*object!=NULL){
    delete *object;
    *object=NULL;
  }
}
    
#endif
