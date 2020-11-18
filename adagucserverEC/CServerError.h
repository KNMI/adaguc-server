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

#ifndef MY_ERROR_H
#define MY_ERROR_H
#include <stdio.h>
#include <CDebugger.h>

enum ServiceExceptionCode { OperationNotSupported, InvalidDimensionValue}; // OGC WMS Exceptions



void printerror(const char * text);
void printdebug(const char * text,int prioritylevel);
void seterrormode(int errormode);
void readyerror();
void printerrorImage(void * drawImage);
void resetErrors();
bool errorsOccured();
void setErrorImageSize(int w,int h,int format,bool _enableTransparency);
void setExceptionType(ServiceExceptionCode code);
const char * getExceptionCodeText(ServiceExceptionCode code);
#endif

