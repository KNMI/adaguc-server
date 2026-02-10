/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2015-01-26
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

#ifndef CDATAPOSTPROCESSOR_H
#define CDATAPOSTPROCESSOR_H

#include <cstdint>
#include "CDataSource.h"
#include "CDPPInterface.h"

class CDPPExecutor {
public:
  std::vector<CDPPInterface *> *dataPostProcessorList;
  CDPPExecutor();
  ~CDPPExecutor();
  const std::vector<CDPPInterface *> *getPossibleProcessors();
  int executeProcessors(CDataSource *dataSource, int mode);
  int executeProcessors(CDataSource *dataSource, int mode, double *data, size_t numItems);
};

CDPPExecutor *getCDPPExecutor();
#endif
