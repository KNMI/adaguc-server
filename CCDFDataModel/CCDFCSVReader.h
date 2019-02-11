/******************************************************************************
 * 
 * Project:  Generic common data format
 * Purpose:  Packages CSV into a ADAGUC Common Data Model
 * Author:   Maarten Plieger (KNMI)
 * Date:     2018-11-12
 *
 ******************************************************************************
 *
 * Copyright 2018, Royal Netherlands Meteorological Institute (KNMI)
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

#ifndef CCDFCSVREADER_H
#define CCDFCSVREADER_H

#include <stdio.h>
#include <vector>
#include <iostream>
#include <netcdf.h>
#include <math.h>
#include <strings.h>
#include "CCDFDataModel.h"
#include "CCDFCache.h"
#include "CCDFReader.h"
#include "CDebugger.h"

// #define CCDFCSVREADER_DEBUG
// #define CCDFCSVREADER_DEBUG_OPEN

class CDFCSVReader :public CDFReader{
  private:
    DEF_ERRORFUNCTION();
    std::vector<CDF::Variable*> variableIndexer;
    CT::StackList<CT::stringref> csvLines;
    CT::string csvData;
    size_t headerStartsAtLine;
  public:
    CDFCSVReader();
    ~CDFCSVReader();
    
    int open(const char *fileName);
    
    int close();
    
    int _readVariableData(CDF::Variable *var, CDFType type);
    
    int _readVariableData(CDF::Variable *var, CDFType type,size_t *start,size_t *count,ptrdiff_t *stride);
};
#endif
