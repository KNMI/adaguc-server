/******************************************************************************
 * 
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
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

#ifndef CCDFDATAMODEL_H
#define CCDFDATAMODEL_H

//#define CCDFDATAMODEL_DEBUG

#define CCDFDATAMODEL_DUMP_STANDARD 0
#define CCDFDATAMODEL_DUMP_JSON     1


#include "CCDFTypes.h"
#include "CCDFAttribute.h"
#include "CCDFDimension.h"
#include "CCDFVariable.h"
#include "CCDFObject.h"
#include "CCDFReader.h"
#include "CCDFWarper.h"
namespace CDF{
  
  void _dump(CDFObject* cdfObject,CT::string* dumpString, int returnType);
  void _dump(CDF::Variable* cdfVariable,CT::string* dumpString, int returnType);
  CT::string dump(CDFObject* cdfObject);
  CT::string dump(CDF::Variable* cdfVariable);
  CT::string dumpAsJSON(CDFObject* cdfObject);
  void _dumpPrintAttributes(const char *variableName, std::vector<CDF::Attribute *>attributes,CT::string *dumpString, int returnType);
  
};

#endif
