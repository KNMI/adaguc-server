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


/* Tested with https://raw.githubusercontent.com/JuliaData/CSV.jl/master/test/testfiles/FL_insurance_sample.csv 
 * 
 * To develop this code, go to CCDFDataModel and do:
 * 
 * make anydump && ./anydump -h ../data/datasets/csvexample.csv
 * 
 */
#include "CCDFCSVReader.h"
#include "CTime.h"

const char *CDFCSVReader::className="CSVReader";

CDFCSVReader::CDFCSVReader():CDFReader(){
  #ifdef CCDFCSVREADER_DEBUG        
  CDBDebug("New CDFCSVReader");
  #endif
}

CDFCSVReader::~CDFCSVReader(){
  close();
}

int CDFCSVReader::open(const char *fileName){
  #ifdef CCDFCSVREADER_DEBUG        
  CDBDebug("CDFCSVReader::open %s", fileName);
  #endif
  if(cdfObject == NULL){
    CDBError("No CDFObject defined, use CDFObject::attachCDFReader(CDFNetCDFReader*). Please note that this function should be called by CDFObject open routines.");
    return 1;
  }
  if (this->csvLines.size() > 0 && this->fileName.equals(fileName)){
    CDBDebug("Already opened");
    return 0;
  }
  this->fileName=fileName;
  
  CT::string fileBaseName = CT::string(fileName).basename();
  
  /* Is this really csv */
  if (fileBaseName.endsWith(".csv")==false){
    CDBError("Filename does not end with \".csv\"");
    return 1;
  }
  
  /* Caching options, TODO: not tested with CSV*/
  if(cdfCache!=NULL){
    int cacheStatus = cdfCache->open(fileName,cdfObject,false);
    if(cacheStatus == 0) {
      CDBDebug("Succesfully opened from cache for file %s",fileName);
      return 0;
    }
  }
  
  /*This is opendap, there the CSV has already been converted to CDM by an IOServiceProvider.*/
  if(this->fileName.indexOf("http")==0){
    CDBDebug("This is opendap, no conversion needed.");    
    return 0;
  }
  
  /* Set global CDM attributes */
  cdfObject->addAttribute(new CDF::Attribute("Conventions","CF-1.6"));
  cdfObject->addAttribute(new CDF::Attribute("featureType","point"));
  cdfObject->addAttribute(new CDF::Attribute("ADAGUC_READER","CSV"));
  cdfObject->addAttribute(new CDF::Attribute("history","Metadata adjusted by ADAGUC from CSV to NetCDF-CF"));
  
  
  /* Read the CSV file */
  this->csvData=CReadFile::open(fileName);
  
  /* Detect variables from header */
  this->csvLines = csvData.splitToStackReferences("\n");
  if (this->csvLines.size() < 2){
    this->csvLines = csvData.splitToStackReferences("\r");
  }
  #ifdef CCDFCSVREADER_DEBUG     
  CDBDebug("Found %d lines", this->csvLines.size());
  #endif
  
  if (this->csvLines.size() < 2){
    CDBError("No CSV data found, less than 2 lines detected");
    return 1;
  }
  
  /* Skip header */
  this->headerStartsAtLine = 0;
  for(size_t j=0;j<this->csvLines.size();j++){
    if (this->csvLines[j].length() < 2 || (this->csvLines[j].length() > 1 && this->csvLines[j].c_str()[0] == '#')){
      this->headerStartsAtLine++;
    } else {
      break;
    }
  }
  
  size_t numLines = this->csvLines.size() -(1 + this->headerStartsAtLine); /* Minus header */
  
  CT::StackList<CT::string> header = CT::string(this->csvLines[this->headerStartsAtLine + 0].c_str()).splitToStack(",");
  CT::StackList<CT::stringref> firstLine = this->csvLines[this->headerStartsAtLine + 1].splitToStackReferences(",");
  
  if (header.size() < 3) {
    CDBError("No CSV data found, less than 3 columns detected");
    return 1;
  }

  /* Search for lat and lon variables */
  int foundLat = -1;
  int foundLon = -1;
  for(size_t c=0;c<header.size();c++){
    CT::string name = header[c];
    name = name.toLowerCase();
    if (foundLat == -1 && name.equals("lat")){
      foundLat = c;      
    } else if (foundLat == -1 && name.equals("y")){
      foundLat = c;      
    } else if (foundLat == -1 && name.indexOf("latitude") != -1){
      foundLat = c;      
    } else if (foundLat == -1 && name.indexOf("lat") != -1){
      foundLat = c;      
    }
    
    if (foundLon == -1 && name.equals("lon")){
      foundLon = c;      
    } else if (foundLon == -1 && name.equals("x")){
      foundLon = c;      
    } else if (foundLon == -1 && name.indexOf("longitude") != -1){
      foundLon = c;      
    } else if (foundLon == -1 && name.indexOf("lon") != -1){
      foundLon = c;      
    }
  }
  if (foundLat == -1 || foundLon == -1){
    CDBError("Unable to determine lat or lon variables");
    return 1;
  }
  
  header[foundLat] = "lat";
  header[foundLon] = "lon";

  /* Search for time dim */
  CT::string timeString;
  for(size_t j=0;j<this->headerStartsAtLine;j++){
   CT::string metadataLine = this->csvLines[j].c_str();
   int timeStart = metadataLine.indexOf("time=");
   if (timeStart!=-1){
     CT::string timeMetadataString = metadataLine.substring(timeStart, -1);
     int timeEnd = timeMetadataString.indexOf("&");
     if (timeEnd == -1) timeEnd = timeMetadataString.indexOf(";");
     if (timeEnd == -1) timeEnd =timeMetadataString.length();
     timeMetadataString.setSize(timeEnd);     
     CT::StackList<CT::string> kvp = timeMetadataString.splitToStack("=");
     if (kvp.size() == 2 && kvp[1].length() > 5){
       timeString = kvp[1].c_str();
     }
   }
  }
  /* Define station dimension and variable */
  CDF::Dimension * stationDim = new CDF::Dimension();stationDim->setName("station");
  stationDim->setSize(numLines);
  cdfObject->addDimension(stationDim);

  CDF::Variable * stationVar = new CDF::Variable();cdfObject->addVariable(stationVar);
  stationVar->setName(stationDim->getName());stationVar->currentType=CDF_STRING;stationVar->nativeType=CDF_STRING;stationVar->setType(CDF_STRING);stationVar->isDimension=true;
  stationVar->allocateData(numLines);
  stationVar->dimensionlinks.push_back(stationDim);

  /* Define time dimension and variable */
  CDF::Dimension * timeDim = new CDF::Dimension();timeDim->setName("time");
  timeDim->setSize(1);
  cdfObject->addDimension(timeDim); 
  
  CDF::Variable * timeVar = new CDF::Variable();cdfObject->addVariable(timeVar);
  timeVar->setName(timeDim->getName());timeVar->currentType=CDF_DOUBLE;timeVar->nativeType=CDF_DOUBLE;timeVar->setType(CDF_DOUBLE);timeVar->isDimension=true;timeVar->allocateData(timeDim->getSize());
  timeVar->dimensionlinks.push_back(timeDim);
  timeVar->setAttributeText("standard_name", "time");
  timeVar->setAttributeText("long_name", "time");
  timeVar->setAttributeText("units", "seconds since 1970-1-1");
  if (timeString.length() > 5){
    CDBDebug("timeString = [%s]", timeString.c_str());
    CTime *ctime = CTime::GetCTimeInstance(timeVar);
    ((double*)timeVar->data)[0] = ctime->dateToOffset(ctime->freeDateStringToDate(timeString.c_str()));
  } else {
    ((double*)timeVar->data)[0] = 0;
  }
  

  /* Just in case open is done twice, clear the var indices vector */
  this->variableIndexer.clear();
  
  /* Determine data vars */
  for(size_t c=0;c<header.size();c++){
    #ifdef CCDFCSVREADER_DEBUG     
    CDBDebug("col %d is [%s] with value %s", c, header[c].c_str(), firstLine[c].c_str());
    #endif
    CT::string col = firstLine[c].c_str();
    CDFType dataType = CDF_FLOAT;
    if (col.isNumeric()){ 
      dataType = CDF_INT; 
    }else if (col.isFloat()){ 
      dataType = CDF_FLOAT;
    }else {dataType = CDF_STRING;}
    /* Add new variable to cdfObject */
    CDF::Variable * dataVar = new CDF::Variable();cdfObject->addVariable(dataVar);
    dataVar->setCDFReaderPointer((void*)this);
    dataVar->setName(header[c].c_str());dataVar->currentType=dataType;dataVar->nativeType=dataType;dataVar->setType(dataType);dataVar->isDimension=false;
    if (timeString.length() > 5 && int(c) != foundLat && int(c) != foundLon){
      dataVar->dimensionlinks.push_back(timeDim);
    }
    dataVar->dimensionlinks.push_back(stationDim);
    this->variableIndexer.push_back(dataVar);
  }
  
  /* Done */
  return 0;
}

int CDFCSVReader::close() {
  CDBDebug("Closing CSV reader");
  return 0;
}

int CDFCSVReader::_readVariableData(CDF::Variable *varToRead, CDFType type){
  if (this->csvLines.size() < 2){
    CDBError("No CSV data found, less than 2 lines detected");
    return 1;
  }
  size_t varSize = this->csvLines.size() - (1 + this->headerStartsAtLine);
  if (varToRead->getSize() == varSize && type == varToRead->currentType){
    CDBDebug("Already loaded variable %s", varToRead->name.c_str());
    return 0;
  }
  varToRead->currentType = type;
  varToRead->allocateData(varSize);
  varToRead->setSize(varSize);
  
  for(size_t j=(1 + this->headerStartsAtLine);j<this->csvLines.size();j++){
    size_t varPointer = j - (1 + this->headerStartsAtLine);
    if (this->csvLines[j].length() == 0) {
      CDBWarning("Found empty CSV line at line %d", j);
      continue;
    }
    CT::StackList<CT::stringref> csvColumns = this->csvLines[j].splitToStackReferences(",");
    
    if (csvColumns.size() != this->variableIndexer.size()){
      CDBWarning("CSV Columns at line %d have unexpected size of %d, expected %d", csvColumns.size(), this->variableIndexer.size());
      continue;
    }
    bool foundVar = false;
    for(size_t c=0;c<csvColumns.size();c++){
      CDF::Variable *var = this->variableIndexer[c];
      if (var->name.equals(varToRead->name)){
        foundVar = true;
        if (var->currentType == CDF_STRING){
          const char*stringToAdd = csvColumns[c].c_str();
          size_t length = strlen(stringToAdd);
          ((char**)var->data)[varPointer]=(char*)malloc(length+1);
          snprintf(((char**)var->data)[varPointer],length+1,"%s",stringToAdd);
        }
        if (var->currentType == CDF_INT){
          ((int*)var->data)[varPointer] =  CT::string(csvColumns[c].c_str()).toInt();
        }
        if (var->currentType == CDF_FLOAT){
          ((float*)var->data)[varPointer] =  CT::string(csvColumns[c].c_str()).toFloat();
        }
        if (var->currentType == CDF_DOUBLE){
          ((double*)var->data)[varPointer] =  CT::string(csvColumns[c].c_str()).toDouble();
        }
        break;
      }

    }
    if (foundVar == false){
      CDBError("Unable to read variable %s", varToRead->name.c_str());
      return 1;
    }
  
  }
  
  return 0;

};
    
int CDFCSVReader::_readVariableData(CDF::Variable *var, CDFType type,size_t *start,size_t *count,ptrdiff_t *stride){
  CDBError("Error: CSV readVariableData with start,count and stride is NOT YET IMPLEMENTED");
  return 1;
};
