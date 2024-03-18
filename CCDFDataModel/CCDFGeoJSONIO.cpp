/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Packages GeoJSON into a NetCDF file
 * Author:   Ernst de Vreede (KNMI)
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

#include "CCDFGeoJSONIO.h"
#include <CReadFile.h>
// #define CCDFGEOJSONIO_DEBUG

const char *CDFGeoJSONReader::className = "GeoJSONReader";

CDFGeoJSONReader::CDFGeoJSONReader() : CDFReader() {
#ifdef CCDFGEOJSONIO_DEBUG
  CDBDebug("New CDFGeoJSONReader");
#endif
}

CDFGeoJSONReader::~CDFGeoJSONReader() { close(); }

int CDFGeoJSONReader::open(const char *fileName) {

  if (cdfObject == NULL) {
    CDBError("No CDFObject defined, use CDFObject::attachCDFReader(CDFNetCDFReader*). Please note that this function should be called by CDFObject open routines.");
    return 1;
  }
  this->fileName = fileName;

  // This is opendap, there the geojson has already been converted to CDM by an IOServiceProvider.
  if (this->fileName.indexOf("http") == 0) {
    CDBDebug("This is opendap, no conversion needed.");

    return 0;
  }

  cdfObject->addAttribute(new CDF::Attribute("Conventions", "CF-1.6"));
  cdfObject->addAttribute(new CDF::Attribute("history", "Metadata adjusted by ADAGUC from GeoJSON to NetCDF-CF"));

  CT::string fileBaseName;
  const char *last = rindex(fileName, '/');
  if ((last != NULL) && (*last)) {
    fileBaseName.copy(last + 1);
  } else {
    fileBaseName.copy(fileName);
  }

  if (!((strlen(fileName) > 4) || (strcmp("json", fileName + strlen(fileName - 4)) == 0))) {
    return 1;
  }
  CT::string jsonData = CReadFile::open(fileName);
  CDF::Variable *jsonVar = new CDF::Variable();

  // jsonVar->setCDFReaderPointer((void*)this); TODO: Check if this is really needed.
  cdfObject->addVariable(jsonVar);
  jsonVar->setName("jsoncontent");
  jsonVar->currentType = CDF_CHAR;
  jsonVar->nativeType = CDF_CHAR;
  jsonVar->setType(CDF_CHAR);
  jsonVar->isDimension = false;
  jsonVar->allocateData(jsonData.length() + 1);
  strncpy((char *)jsonVar->data, jsonData.c_str(), jsonData.length() + 1);
  CDF::Attribute *attr = new CDF::Attribute();
  cdfObject->addAttribute(attr);
  attr->setName("ADAGUC_GEOJSON");
  attr->setData(CDF_CHAR, "", 0);
  CDF::Attribute *fileAttr = new CDF::Attribute();
  jsonVar->addAttribute(fileAttr);
  fileAttr->setName("ADAGUC_BASENAME");
  fileAttr->setData(CDF_CHAR, fileBaseName.c_str(), fileBaseName.length() + 1);

  return 0;
}

int CDFGeoJSONReader::close() { return 0; }

int CDFGeoJSONReader::_readVariableData(CDF::Variable *, CDFType) { return 0; };

int CDFGeoJSONReader::_readVariableData(CDF::Variable *, CDFType, size_t *, size_t *, ptrdiff_t *) { return 0; };
