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

#ifndef COGCDIMS_H
#define COGCDIMS_H
#include <stdio.h>
#include "CTypes.h"

class COGCDims{
  public:
    COGCDims(){
      isATimeDimension = false;
    }
    /**
     * OGC name
     */
    
    CT::string name;
    /** 
     * Value, are all values as given in the KVP request string, can contain / and , tokens
     */
    CT::string value;

    /**
     * NetCDF name
     */
    CT::string netCDFDimName;
    
    /**
     * Unique values, similar to values except that they are filtered and selected as available from the file (distinct/grouped), 
     */
    std::vector<CT::string> uniqueValues;
    
    void addValue(const char *value);
    
    bool isATimeDimension;
    
};

class CCDFDims {
  private:
  DEF_ERRORFUNCTION();
    class NetCDFDim{
      public:
      CT::string name;
      CT::string value;
      size_t index;
    };
    std::vector <NetCDFDim*> dimensions;  
    int getArrayIndexForName(const char *name);
  public:
    ~CCDFDims ();
    void addDimension(const char *name,const char *value,size_t index);
    size_t getDimensionIndex(const char *name);
    size_t getDimensionIndex(int j);
    size_t getNumDimensions();
    bool isTimeDimension(int j);
    CT::string *getDimensionValuePointer(int j);
    CT::string getDimensionValue(const char *name);
    CT::string getDimensionValue(int j);
    const char *getDimensionName(int j);
    void copy(CCDFDims * dim);
};
#endif
