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
#include <cstdio>
#include <CDebugger.h>

// Determines how outputs will be printed
enum ServiceExceptionMode { ExceptionPlainText, ExceptionJSON, ExceptionImage, ExceptionBlankImage, ExceptionWMS_1_0_0, ExceptionWMS_1_1_1, ExceptionWMS_1_3_0 }; // OGC WMS Exceptions

// OGC WMS Exceptions types.
enum ServiceExceptionType { OK, OperationNotSupported, InvalidDimensionValue, UnprocessableEntity, InvalidDataset, InvalidLayer };

// Status code types, used as return code of main function.
enum ServiceStatusCode { HTTPStatusOK_200 = 0, HTTPStatusNotFound_404 = 32, HTTPStatusUnProcessableEntity_422 = 33 };

void addErrorMessage(const char *text);
void setErrorMode(ServiceExceptionMode errormode);
void readyHandleError();
void resetErrors();
bool errorsOccured();
void setErrorImageSize(int w, int h, int format, bool _enableTransparency);
void setExceptionType(ServiceExceptionType code);
ServiceStatusCode getStatusCode();
std::string getExceptionCodeText(ServiceExceptionType code);

#endif
