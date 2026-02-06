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

#ifndef CCDFVARIABLE_H
#define CCDFVARIABLE_H

#include <algorithm>
#include "CCDFTypes.h"
#include "CCDFAttribute.h"
#include "CCDFDimension.h"
// #define CCDFDATAMODEL_DEBUG
#include "CDebugger.h"

class CDFObject;
namespace CDF {

  class Variable {
  private:
    int _readData(CDFType type, size_t *_start, size_t *_count, ptrdiff_t *_stride);

  public:
    CDFType nativeType;
    CDFType currentType;
    CT::string name;
    CT::string orgName;
    std::vector<Attribute *> attributes;
    std::vector<Dimension *> dimensionlinks;
    int id = -1;
    size_t currentSize = 0;
    void *data = nullptr;
    bool isDimension = false;
    bool enableCache = false;

    // Currently, aggregation along just 1 dimension is supported.
    struct CDFObjectClass {
      void *cdfObjectPointer;
      int dimIndex;
      CT::string dimValue;
    };

    CDF::Variable *clone(CDFType newType, CT::string newName);
    void copy(CDF::Variable *sourceVariable);

  private:
    std::vector<CDFObjectClass *> cdfObjectList;
    void *cdfReaderPointer;
    CDFObject *parentCDFObject;
    bool _hasCustomReader;

  public:
    class CustomReader {
    public:
      virtual ~CustomReader() {}
      virtual int readData(CDF::Variable *thisVar, size_t *start, size_t *count, ptrdiff_t *stride) = 0;
    };
    class CustomMemoryReader : public CDF::Variable::CustomReader {
    public:
      ~CustomMemoryReader() {}
      int readData(CDF::Variable *thisVar, size_t *, size_t *count, ptrdiff_t *stride) override {
        int size = 1;
        for (size_t j = 0; j < thisVar->dimensionlinks.size(); j++) {
          size *= int((float(count[j]) / float(stride[j])) + 0.5);
        }
        thisVar->setSize(size);
        CDF::allocateData(thisVar->getType(), &thisVar->data, size);
        // CDF::fill(thisVar->data, thisVar->getType(),12345,size);//Should be done by followup code.
        return 0;
      }
    };
    static CustomMemoryReader *CustomMemoryReaderInstance;

  private:
    CustomReader *customReader;
    bool _isString;

  public:
    void setCustomReader(CustomReader *customReader);
    CustomReader *getCustomReader();

    bool hasCustomReader();

    void setCDFReaderPointer(void *cdfReaderPointer);
    void setParentCDFObject(CDFObject *parentCDFObject);
    CDFObject *getParentCDFObject() const;

    void setCDFObjectDim(Variable *sourceVar, const char *dimName);

    void fill(double value);

    void allocateData(size_t size);

    void freeData();

    int readData(CDFType type);
    int readData(bool applyScaleOffset);
    int readData(CDFType type, bool applyScaleOffset);
    int readData(CDFType type, size_t *_start, size_t *_count, ptrdiff_t *stride);
    int readData(CDFType type, size_t *_start, size_t *_count, ptrdiff_t *stride, bool applyScaleOffset);

    template <class T> T getDataAt(int index) {
      if (data == NULL) {
        throw(CDF_E_VARHASNODATA);
      }
      T dataElement = 0;
      if (currentType == CDF_CHAR) dataElement = (T)((char *)data)[index];
      if (currentType == CDF_BYTE) dataElement = (T)((char *)data)[index];
      if (currentType == CDF_UBYTE) dataElement = (T)((unsigned char *)data)[index];
      if (currentType == CDF_SHORT) dataElement = (T)((short *)data)[index];
      if (currentType == CDF_USHORT) dataElement = (T)((ushort *)data)[index];
      if (currentType == CDF_INT) dataElement = (T)((int *)data)[index];
      if (currentType == CDF_UINT) dataElement = (T)((unsigned int *)data)[index];
      if (currentType == CDF_INT64) dataElement = (T)((long *)data)[index];
      if (currentType == CDF_UINT64) dataElement = (T)((unsigned long *)data)[index];
      if (currentType == CDF_FLOAT) dataElement = (T)((float *)data)[index];
      if (currentType == CDF_DOUBLE) dataElement = (T)((double *)data)[index];

      return dataElement;
    }

    int getIterativeDimIndex();

    Dimension *getIterativeDim();

    Variable(const char *name, CDFType type, CDF::Dimension *dims[], int numdims, bool isCoordinateVariable);
    Variable(const char *name, CDFType type, std::vector<CDF::Dimension *> idimensionlinks, bool isCoordinateVariable);
    Variable(const char *name, CDFType type);
    Variable();
    ~Variable();

    CDFType getType();
    CDFType getNativeType();
    void setType(CDFType type);

    bool isString();
    bool isString(bool isString);
    void setName(const char *value);

    void setSize(size_t size);
    size_t getSize();

    Attribute *getAttribute(const char *name) const;

    Attribute *getAttributeNE(const char *name) const;

    /**
     * Returns the dimension for given name. Throws error code  when something goes wrong
     * @param name The name of the dimension to look for
     * @return Pointer to the dimension
     */
    Dimension *getDimension(const char *name);

    Dimension *getDimensionIgnoreCase(const char *name);

    Dimension *getDimensionNE(const char *name);

    int getDimensionIndexNE(const char *name);

    int getDimensionIndex(const char *name);
    int addAttribute(Attribute *attr);
    int removeAttribute(const char *name);

    int removeAttributes();

    int setAttribute(const char *attrName, CDFType attrType, const void *attrData, size_t attrLen);

    template <class T> int setAttribute(const char *attrName, CDFType attrType, T data) {
      Attribute *attr;
      try {
        attr = getAttribute(attrName);
      } catch (...) {
        attr = new Attribute();
        attr->name.copy(attrName);
        addAttribute(attr);
      }
      attr->type = attrType;
      attr->setData(attrType, data);
      return 0;
    }
    int setAttributeText(const char *attrName, const char *attrString, size_t strLen);
    int setAttributeText(const char *attrName, const char *attrString);

    int setAttributeText(std::string attrName, std::string attrString);

    void *getCDFObjectClassPointer(size_t *start, size_t *count);
    int setData(CDFType type, const void *dataToSet, size_t dataLength);
  };
} // namespace CDF
#endif
