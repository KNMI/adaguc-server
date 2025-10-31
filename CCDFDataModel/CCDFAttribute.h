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

#ifndef CCDFATTRIBUTE_H
#define CCDFATTRIBUTE_H

#include "CCDFTypes.h"
namespace CDF {
  class Attribute {
  public:
    void setName(const char *value);
    Attribute();
    Attribute(Attribute *att);
    Attribute(const char *attrName, const char *attrString);
    Attribute(const char *attrName, CDFType type, const void *dataToSet, size_t dataLength);
    ~Attribute();
    CDFType type;
    CT::string name;
    size_t length;
    void *data;
    CDFType getType();

    int setData(Attribute *attribute);
    int setData(CDFType type, const void *dataToSet, size_t dataLength);

    /**
     * Sets one element of data
     */
    template <class T> int setData(CDFType type, T data) {
      if (type == CDF_CHAR || type == CDF_BYTE) {
        char d = data;
        setData(type, &d, 1);
      }
      if (type == CDF_UBYTE) {
        unsigned char d = data;
        setData(type, &d, 1);
      }
      if (type == CDF_SHORT) {
        short d = data;
        setData(type, &d, 1);
      }
      if (type == CDF_USHORT) {
        unsigned short d = data;
        setData(type, &d, 1);
      }
      if (type == CDF_INT) {
        int d = data;
        setData(type, &d, 1);
      }
      if (type == CDF_UINT) {
        unsigned int d = data;
        setData(type, &d, 1);
      }
      if (type == CDF_INT64) {
        long d = data;
        setData(type, &d, 1);
      }
      if (type == CDF_UINT64) {
        unsigned long d = data;
        setData(type, &d, 1);
      }
      if (type == CDF_FLOAT) {
        float d = data;
        setData(type, &d, 1);
      }
      if (type == CDF_DOUBLE) {
        double d = data;
        setData(type, &d, 1);
      }
      return 0;
    }

    int setData(const char *dataToSet);
    template <class T> int getData(T *dataToGet, size_t getlength) {
      if (data == NULL) return 0;
      if (getlength > length) getlength = length;
      CDF::DataCopier::copy(dataToGet, data, type, getlength);
      return getlength;
    }

    template <class T> T getDataAt(int index) {
      if (data == NULL) {
        throw(CDF_E_VARHASNODATA);
      }
      T dataElement = 0;
      if (type == CDF_CHAR) dataElement = (T)((char *)data)[index];
      if (type == CDF_BYTE) dataElement = (T)((char *)data)[index];
      if (type == CDF_UBYTE) dataElement = (T)((unsigned char *)data)[index];
      if (type == CDF_SHORT) dataElement = (T)((short *)data)[index];
      if (type == CDF_USHORT) dataElement = (T)((ushort *)data)[index];
      if (type == CDF_INT) dataElement = (T)((int *)data)[index];
      if (type == CDF_UINT) dataElement = (T)((unsigned int *)data)[index];
      if (type == CDF_INT64) dataElement = (T)((long *)data)[index];
      if (type == CDF_UINT64) dataElement = (T)((unsigned long *)data)[index];
      if (type == CDF_FLOAT) dataElement = (T)((float *)data)[index];
      if (type == CDF_DOUBLE) dataElement = (T)((double *)data)[index];

      return dataElement;
    }

    int getDataAsString(CT::string *out);

    CT::string toString();

    CT::string getDataAsString();

    size_t size();
  };
} // namespace CDF
#endif
