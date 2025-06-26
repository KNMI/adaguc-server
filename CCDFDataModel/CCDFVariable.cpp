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

#include "CCDFVariable.h"
#include "CCDFObject.h"
#include "CCDFReader.h"
#include "CTime.h"
#include "traceTimings/traceTimings.h"
const char *CDF::Variable::className = "Variable";

extern CDF::Variable::CustomMemoryReader customMemoryReaderInstance;
CDF::Variable::CustomMemoryReader customMemoryReaderInstance;
CDF::Variable::CustomMemoryReader *CDF::Variable::CustomMemoryReaderInstance = &customMemoryReaderInstance;

// #define CCDFDATAMODEL_DEBUG
int CDF::Variable::readData(CDFType type) { return readData(type, NULL, NULL, NULL); }

int CDF::Variable::readData(bool applyScaleOffset) { return readData(-1, applyScaleOffset); }

int CDF::Variable::readData(CDFType readType, bool applyScaleOffset) { return readData(readType, NULL, NULL, NULL, applyScaleOffset); }

/**
 * Reads data
 * @param readType The datatype to read. When -1 is given, this is determined automatically
 * @param applyScaleOffset Whether or not to apply scale and offset
 */
int CDF::Variable::readData(CDFType readType, size_t *_start, size_t *_count, ptrdiff_t *_stride, bool applyScaleOffset) {

  if (data != NULL && currentType != readType) {
    // CDBDebug("CDF::Variable::readData freeing data");
    freeData();
  }
  if (data != NULL) {
    // TODO Check start,stop, stride settings first!!!
#ifdef CCDFDATAMODEL_DEBUG
    CDBDebug("Data is already defined");
#endif
    return 0;
  }
#ifdef CCDFDATAMODEL_DEBUG
  CDBDebug("applyScaleOffset = %d", applyScaleOffset);
#endif
  if (applyScaleOffset == false) {
    return readData(currentType, _start, _count, _stride);
  }

  double scaleFactor = 1, addOffset = 0, fillValue = 0;
  bool hasFillValue = false;
  int scaleType = currentType;
  try {

    CDF::Attribute *a = getAttribute("scale_factor");
    a->getData(&scaleFactor, 1);
    scaleType = a->type;
  } catch (int e) {
  }

  try {
    getAttribute("add_offset")->getData(&addOffset, 1);
  } catch (int e) {
  }
  try {
    getAttribute("_FillValue")->getData(&fillValue, 1);
    hasFillValue = true;
  } catch (int e) {
  }

  bool reallyApplyScaleOffset = false;

  if (scaleFactor != 1 || addOffset != 0) {
    reallyApplyScaleOffset = true;
  }

  if (readType != -1) {
    scaleType = readType;
  }

  if (readType != -1 && reallyApplyScaleOffset) {
    if (readType != CDF_FLOAT && readType != CDF_DOUBLE) {
      CDBError("Unable to apply scale offset for readtype %s", CDF::getCDFDataTypeName(readType).c_str());
      return 1;
    }
  }

  // CDBDebug("Start reading data of type %d with reallyApplyScaleOffset = %d",scaleType,reallyApplyScaleOffset);

  int status = readData(scaleType, _start, _count, _stride);

  //   if(scaleType == CDF_FLOAT){
  //     CDBDebug("%s has %f",name.c_str(),((float*)data)[0]);
  //   }

  if (status != 0) return status;
  // CDBDebug("applyScaleOffset = %f %f",scaleFactor,addOffset);
  // Apply scale and offset
  if (reallyApplyScaleOffset) {
    size_t lsize = getSize();
    // CDBDebug("ScaleType = %s",CDF::getCDFDataTypeName(scaleType).c_str());
    if (scaleType == CDF_FLOAT) {
      float *scaleData = (float *)data;
      float fscale = float(scaleFactor);
      float foffset = float(addOffset);
      for (size_t j = 0; j < lsize; j++) scaleData[j] = scaleData[j] * fscale + foffset;
      float newFillValue = fillValue * fscale + foffset;
      if (hasFillValue) getAttribute("_FillValue")->setData(CDF_FLOAT, &newFillValue, 1);
    }

    if (scaleType == CDF_DOUBLE) {
      double *scaleData = (double *)data;
      for (size_t j = 0; j < lsize; j++) scaleData[j] = scaleData[j] * scaleFactor + addOffset;
      double newFillValue = fillValue * scaleFactor + addOffset;
      if (hasFillValue) getAttribute("_FillValue")->setData(CDF_DOUBLE, &newFillValue, 1);
    }
    // removeAttribute("scale_factor");
    // removeAttribute("add_offset");
  }
  return 0;
}

int CDF::Variable::readData(CDFType type, size_t *_start, size_t *_count, ptrdiff_t *_stride) {
  traceTimingsSpanStart(TraceTimingType::FSREADVAR);
  int status = _readData(type, _start, _count, _stride);
  traceTimingsSpanEnd(TraceTimingType::FSREADVAR);
  return status;
}

int CDF::Variable::_readData(CDFType type, size_t *_start, size_t *_count, ptrdiff_t *_stride) {

#ifdef CCDFDATAMODEL_DEBUG
  CDBDebug("reading variable %s", name.c_str());
  if (_start == NULL) {
    CDBDebug("_start not defined for reading variable %s", name.c_str());
  } else {
    CDBDebug("_start = %d", _start[0]);
  }
#endif

  if (data != NULL && type != this->currentType) {
#ifdef CCDFDATAMODEL_DEBUG
    CDBDebug("Freeing orignal variable %s", name.c_str());
#endif
    freeData();
  }

  // TODO needs to cope correctly with cdfReader.
  if (data != NULL) {
#ifdef CCDFDATAMODEL_DEBUG
    CDBDebug("Data is already defined");
#endif
    return 0;
  }

  // Check for iterative dimension
  bool needsDimIteration = false;
  int iterativeDimIndex = getIterativeDimIndex();
  if (iterativeDimIndex != -1 && isDimension == false) {
    needsDimIteration = true;
  }

  if (needsDimIteration == true) {
    // CDF::Dimension * iterativeDim;
    bool useStartCountStride = false;
    if (_start != NULL && _count != NULL) {
      useStartCountStride = true;
    }
    // iterativeDim=dimensionlinks[iterativeDimIndex];
    // Make start and count params.
    size_t *start = new size_t[dimensionlinks.size()];
    size_t *count = new size_t[dimensionlinks.size()];
    ptrdiff_t *stride = new ptrdiff_t[dimensionlinks.size()];

    for (size_t j = 0; j < dimensionlinks.size(); j++) {
      start[j] = 0;
      count[j] = dimensionlinks[j]->getSize();
      stride[j] = 1;

      if (useStartCountStride) {
        start[j] = _start[j]; // TODO in case of multiple readers for the same variable, this offset will change!
        count[j] = _count[j];
        stride[j] = _stride[j];
      }
    }

    // Allocate data for this chunk.
    size_t totalVariableSize = 1;
    for (size_t i = 0; i < dimensionlinks.size(); i++) {
      totalVariableSize *= count[i];
    }
    setSize(totalVariableSize);
    int status = CDF::allocateData(type, &data, getSize());
    if (data == NULL || status != 0) {
      CDBError("Variable data allocation failed, unable to allocate %d elements", totalVariableSize);
      return 1;
    }
    // Now make the iterative dim of length zero
    size_t iterDimStart = start[iterativeDimIndex];
    // size_t iterDimCount=count[iterativeDimIndex];
#ifdef CCDFDATAMODEL_DEBUG
    for (size_t i = 0; i < dimensionlinks.size(); i++) {
      CDBDebug("%d\t%d", start[i], count[i]);
    }
#endif

    size_t dataReadOffset = 0;
    // for(size_t j=iterDimStart;j<iterDimCount+iterDimStart;j++)
    int j = iterDimStart;
    {

      try {

        // Get the right CDF reader for this dimension set
        start[iterativeDimIndex] = j;
        count[iterativeDimIndex] = 1;
        CDFObjectClass *tCDFObjectClass = (CDFObjectClass *)getCDFObjectClassPointer(start, count);
        CDFObject *tCDFObject = (CDFObject *)tCDFObjectClass->cdfObjectPointer;
        if (tCDFObject == NULL) {
          CDBError("Unable to read variable %s because tCDFObject==NULL", name.c_str());
          throw(CDF_E_ERROR);
        }
        // Get the variable from this reader
        // CDBDebug("cdfObject->dimIndex %d",tCDFObject->dimIndex);
        //

        for (size_t d = 0; d < dimensionlinks.size(); d++) {
          if (useStartCountStride) {
            start[d] = 0; // TODO in case of multiple readers for the same variable, this offset will change!
            count[d] = _count[d];
            stride[d] = _stride[d];
          } else {
            start[d] = 0;
            count[d] = dimensionlinks[d]->getSize();
            stride[d] = 1;
          }
        }
        start[iterativeDimIndex] = tCDFObjectClass->dimIndex;
        count[iterativeDimIndex] = 1;
        // Read the data!

        if (!_hasCustomReader) {
          // TODO NEEDS BETTER CHECKS
          if (cdfReaderPointer == NULL) {
            CDBError("No CDFReader defined for variable %s", name.c_str());
            delete[] start;
            delete[] count;
            delete[] stride;
            return 1;
          }
          Variable *tVar = tCDFObject->getVariable(name.c_str());
          if (tVar->readData(type, start, count, stride) != 0) throw(__LINE__);
          // Put the read data chunk in our destination variable
#ifdef CCDFDATAMODEL_DEBUG
          CDBDebug("Copying %d elements to variable %s", tVar->getSize(), name.c_str());
#endif
          DataCopier::copy(data, type, tVar->data, type, dataReadOffset, 0, tVar->getSize());
          dataReadOffset += tVar->getSize();
          // Free the read data
#ifdef CCDFDATAMODEL_DEBUG
          CDBDebug("Free tVar %s", tVar->name.c_str());
#endif
          tVar->freeData();
          tCDFObject->close();
        }

        if (_hasCustomReader) {
          status = customReader->readData(this, _start, count, stride);
        }

#ifdef CCDFDATAMODEL_DEBUG
        CDBDebug("Variable->data==NULL: %d", data == NULL);
#endif
      } catch (int e) {

        CDBError("Exception at line %d", e);
        delete[] start;
        delete[] count;
        delete[] stride;
        return 1;
      }
    }
    delete[] start;
    delete[] count;
    delete[] stride;

    if (status != 0) return 1;
  }

  if (needsDimIteration == false) {
#ifdef CCDFDATAMODEL_DEBUG
    CDBDebug("needsDimIteration=false");
#endif
    // TODO NEEDS BETTER CHECKS
    if (cdfReaderPointer == NULL) {
      if (_hasCustomReader) {
        if (this->customReader != NULL) {
          this->setType(type);
          this->customReader->readData(this, _start, _count, _stride);
          return 0;
        }
      }
      CDBError("No CDFReader defined for variable %s", name.c_str());
      return 1;
    }

    CDFReader *cdfReader = (CDFReader *)cdfReaderPointer;

    // CDBDebug("OK");
    int status = 0;
    bool useStartCountStride = false;
    if (_start != NULL && _count != NULL) {
      useStartCountStride = true;
      // When start and count are exactly the same as its dimension size, we do not need to use start,count and stride.
      bool dimSizesAreSameAsRequested = true;
      for (size_t j = 0; j < dimensionlinks.size(); j++) {
        if (_start[j] == 0 && dimensionlinks[j]->getSize() == _count[j] && _stride[j] == 1)
          ;
        else {
          dimSizesAreSameAsRequested = false;
          break;
        }
      }
      if (dimSizesAreSameAsRequested) useStartCountStride = false;
    }
    // CDBDebug("OK");
    if (useStartCountStride == true) {
      // CDBDebug("OK");
#ifdef CCDFDATAMODEL_DEBUG
      CDBDebug("_readVariableData start count stride");
#endif
      status = cdfReader->_readVariableData(this, type, _start, _count, _stride);
      // CDBDebug("OK");
    } else {
      // CDBDebug("OK");
#ifdef CCDFDATAMODEL_DEBUG
      CDBDebug("_readVariableDat");
#endif
      status = cdfReader->_readVariableData(this, type);
    }
    // CDBDebug("OK");
    if (status != 0) {
      CDBError("Unable to read data for variable %s", name.c_str());
      return 1;
    }
  }
#ifdef CCDFDATAMODEL_DEBUG
  CDBDebug("Data for %s read %d", name.c_str(), data != NULL);
#endif

  return 0;
}

void CDF::Variable::setCDFObjectDim(CDF::Variable *sourceVar, const char *dimName) {

#ifdef CCDFDATAMODEL_DEBUG
  CDBDebug("[setCDFObjectDim for %s %s]", sourceVar->name.c_str(), dimName);
#endif
  // if(sourceVar->isDimension)return;
  CDFObject *sourceCDFObject = (CDFObject *)sourceVar->getParentCDFObject();
  std::vector<Dimension *> &srcDims = sourceVar->dimensionlinks;
  std::vector<Dimension *> &dstDims = dimensionlinks;
  // Concerning dimensions need to have data in order to get this to work
  if (dstDims.size() != srcDims.size()) {
    throw(CDF_E_NRDIMSNOTEQUAL);
  }

  // Check which dims are iterative
  for (size_t j = 0; j < dstDims.size(); j++) {
    // Test if the dimensions are the same
    if (!dstDims[j]->name.equals((srcDims)[j]->name.c_str())) {
      CDBError("setCDFReaderForDim: Dimension names are unequal: %s !=%s ", dstDims[j]->name.c_str(), srcDims[j]->name.c_str());
      throw(CDF_E_ERROR);
    }
    // TODO Also check dimension units
    // Check which dimension is not yet iterative.
    if (dstDims[j]->isIterative == false) {
      if (srcDims[j]->name.equals(dimName)) {
        dstDims[j]->isIterative = true;
      }
    }
  }

  Dimension *iterativeDim;
  Variable *iterativeVar;
  try {
    iterativeDim = getIterativeDim();
  } catch (int e) {
    return;
  }
  try {
    iterativeVar = ((CDFObject *)getParentCDFObject())->getVariable(iterativeDim->name.c_str());
  } catch (int e) {
    return;
  }

  // Read data from the source dim
  Variable *srcDimVar;
  //   int sourceType = currentType;
  try {
    srcDimVar = sourceCDFObject->getVariable(iterativeDim->name.c_str());
  } catch (int e) {
    CDBError("Variable [%s] not found in source CDFObject", iterativeDim->name.c_str());
    throw(e);
  }

  if (srcDimVar->data == NULL) {
    srcDimVar->readData(currentType);
  } /*else{
     sourceType = srcDimVar->getType();
   }*/
#ifdef CCDFDATAMODEL_DEBUG
  CDBDebug("=== Found %d steps in source ===", srcDimVar->getSize());
#endif

  //   if(sourceType != currentType){
  //     CDBError("%s == %s",CDF::getCDFDataTypeName(currentType).c_str(),CDF::getCDFDataTypeName(currentType).c_str());
  //   }

  if (iterativeVar->data == NULL) {
#ifdef CCDFDATAMODEL_DEBUG
    CDBDebug("READING FIRST ONE ONCE! Type = %s", CDF::getCDFDataTypeName(currentType).c_str());
#endif
    if (iterativeVar->readData(currentType) != 0) {
      throw(0);
    }
  }

  CTime *ccdftimesrc, *ccdftimedst;

  bool isTimeDim = false;

  try {
    if (srcDimVar->getAttribute("standard_name")->toString().equals("time")) {
      isTimeDim = true;
    }
  } catch (int e) {
  }

  if (isTimeDim) {
    ccdftimesrc = CTime::GetCTimeInstance(srcDimVar);
    if (ccdftimesrc == nullptr) {
      CDBError("Unable to initialize time library");
      throw(1);
    }
  }

  for (size_t indimsize = 0; indimsize < srcDimVar->getSize(); indimsize++) {
    CT::string srcDimValue;

    if (isTimeDim) {
      srcDimValue = ccdftimesrc->dateToString(ccdftimesrc->getDate(srcDimVar->getDataAt<double>(indimsize)));
    } else {
      if (srcDimVar->getType() == CDF_STRING) {
        srcDimValue.print("%s", ((const char **)srcDimVar->data)[indimsize]);
      } else {
        srcDimValue.print("%f", srcDimVar->getDataAt<double>(indimsize));
      }
    }

#ifdef CCDFDATAMODEL_DEBUG
    CDBDebug("srcDimValue = %s", srcDimValue.c_str());
    CDBDebug("Itereating %d/%d = %s", indimsize, srcDimVar->getSize(), srcDimValue.c_str());
#endif

    int foundDimValue = -1;
    size_t dimSize = iterativeDim->getSize();

    // CDBDebug("dimSize = %d",dimSize);
    for (size_t _j = 0; _j < dimSize; _j++) {
      size_t j = _j; //(dimSize-1)-_j;

      if (isTimeDim) {
        ccdftimedst = CTime::GetCTimeInstance(iterativeVar);
        if (ccdftimedst == nullptr) {
          CDBError("Unable to initialize time library");
          throw(1);
        }
      }
      CT::string dstDimValue;
      if (isTimeDim) {
        dstDimValue = ccdftimedst->dateToString(ccdftimedst->getDate(iterativeVar->getDataAt<double>(j)));
      } else {
        if (iterativeVar->getType() == CDF_STRING) {
          dstDimValue.print("%s", ((const char **)iterativeVar->data)[j]);
        } else {
          dstDimValue.print("%f", iterativeVar->getDataAt<double>(j));
        }
      }
#ifdef CCDFDATAMODEL_DEBUG
      // CDBDebug("dstDimValue = %s" ,dstDimValue.c_str());
#endif
      if (dstDimValue.equals(srcDimValue)) {
#ifdef CCDFDATAMODEL_DEBUG
        CDBDebug("Found %s == %s", dstDimValue.c_str(), srcDimValue.c_str());
#endif
        foundDimValue = j;
        break;
      }
    }
    //     if(foundDimValue == -1){
    //       CDBDebug("Unable to find srcDimValue %f",srcDimValue);
    //     }

    // Check wether we already have this cdfobject dimension combo in our list
    int foundCDFObject = -1;
    for (size_t j = 0; j < cdfObjectList.size(); j++) {
      //      CDBDebug("%s==%s",cdfObjectList[j]->dimValue.c_str(),srcDimValue.c_str());
      if (cdfObjectList[j]->dimValue.equals(srcDimValue)) {
        foundCDFObject = j;
        break;
      }
    }
    if (foundCDFObject != -1) {
#ifdef CCDFDATAMODEL_DEBUG
      CDBDebug("Found existing cdfObject %d", foundCDFObject);
#endif
    } else {
#ifdef CCDFDATAMODEL_DEBUG
      CDBDebug("cdfObjectList.push_back(new CDFObjectClass()) for variable %s size= %d", name.c_str(), cdfObjectList.size());
#endif
      CDFObjectClass *c = new CDFObjectClass();
      c->dimValue = srcDimValue;
      c->dimIndex = indimsize;
      c->cdfObjectPointer = sourceVar->getParentCDFObject();
      cdfObjectList.push_back(c);
    }

    if (sourceVar->name.equals(dimName) == true) {
      if (foundDimValue == -1) {
#ifdef CCDFDATAMODEL_DEBUG
        CDBDebug("ADding value %s", srcDimValue.c_str());
#endif

        // Extend the concerning dimension
        size_t currentDimSize = iterativeDim->getSize();
        // CDBDebug("Currentdimsize = %d",currentDimSize);
        void *dstData = NULL;
        int status = 0;
        status = CDF::allocateData(currentType, &dstData, currentDimSize + 1);
        if (status != 0) {
          CDBError("Unable to allocate data");
          throw("__LINE__");
        }
        // CDBDebug("try adding %f",srcDimVar->getDataAt<double>(indimsize));
        status = DataCopier::copy(dstData, currentType, iterativeVar->data, currentType, 0, 0, currentDimSize);
        if (status != 0) {
          CDBError("Unable to copy data");
          throw("__LINE__");
        }

        //         CDBDebug("indimsize %d %d",indimsize,((int*)srcDimVar->data)[0]);
        //         CDBDebug("srcDimVar units = %s",srcDimVar->getAttribute("units")->toString().c_str());
        double destValue = 0;
        try {
          if (isTimeDim) {
            destValue = ccdftimedst->dateToOffset(ccdftimedst->stringToDate(srcDimValue.c_str()));
          } else {
            destValue = srcDimValue.toDouble();
          }
        } catch (int e) {
          CDBError("Error converting %s date", srcDimValue.c_str());
          throw e;
        }
        //         CDBDebug("srcDimVar value = %s == [%d]=%f",srcDimValue.c_str(),currentDimSize,destValue);

        if (currentType == CDF_DOUBLE) ((double *)dstData)[currentDimSize] = destValue;
        if (currentType == CDF_FLOAT) ((float *)dstData)[currentDimSize] = (float)destValue;
        if (currentType == CDF_STRING) {
          // CDBDebug("Appending %s",srcDimValue.c_str());
          ((char **)dstData)[currentDimSize] = (char *)malloc(srcDimValue.length() + 1);
          strncpy(((char **)dstData)[currentDimSize], srcDimValue.c_str(), srcDimValue.length());
          ((char **)dstData)[currentDimSize][srcDimValue.length()] = 0;
        }

        // //         status = DataCopier::copy(dstData,//destdata
        // //                          currentType,     //thistype
        // //                          srcDimVar->data, //sourcedata
        // //                          sourceType,      //sourcetype
        // //                          currentDimSize,  //destinationOffset
        // //                          indimsize,       //sourceOffset
        // //                          1);              //Nr. Elements
        // //         if(status!=0){
        // //           CDBError("Unable to copy timestep ");
        // //           throw("__LINE__");
        // //         }
        iterativeVar->freeData();
        iterativeVar->data = dstData;

        iterativeDim->setSize(currentDimSize + 1);
        iterativeVar->setSize(currentDimSize + 1);

        //        size_t dimSize = iterativeDim->getSize();
        //         for(size_t j=0;j<dimSize;j++){
        //           CDBDebug("%d == %f",j,(iterativeVar->getDataAt<double>(j)));
        //         }
#ifdef CCDFDATAMODEL_DEBUG
        CDBDebug("New iterativeDim size %d", iterativeDim->getSize());
#endif
      } /*else{
         CDBError("For dimension %s, time value %f is already defined, skipping!",dimName,srcDimValue);
       }*/
    }
  }
}

CDF::Variable *CDF::Variable::clone(CDFType newType, CT::string newName) {
  CDF::Variable *newVariable = new CDF::Variable(newName.c_str(), newType, this->dimensionlinks, this->isDimension);

  for (auto attribute : attributes) {
    newVariable->addAttribute(new CDF::Attribute(attribute));
  }
  newVariable->parentCDFObject = parentCDFObject;
  newVariable->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);
  newVariable->data = nullptr;
  return newVariable;
}
void CDF::Variable::copy(CDF::Variable *sourceVariable) {
  size_t size = sourceVariable->getSize();
  this->allocateData(size);
  DataCopier::copy(this->data, this->currentType, sourceVariable->data, sourceVariable->currentType, 0, 0, size);
}