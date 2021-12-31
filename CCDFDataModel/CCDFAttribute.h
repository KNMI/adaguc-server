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
  private:
  public:
    CT::string name;
    CDFType type;
    void *data;
    size_t length;

    void setName(const char *value);

    Attribute();

    Attribute(Attribute *att);

    Attribute(const char *attrName, const char *attrString);

    Attribute(const char *attrName, CDFType type, const void *dataToSet, size_t dataLength);

    ~Attribute();

    /**
     * @brief Get the Type object
     *
     * @return CDFType
     */
    CDFType getType();

    /**
     * @brief Set the data based on the data from another CDF::Attribute
     *
     * @param attribute
     * @return int
     */

    int setData(Attribute *attribute);

    /**
     * @brief Set the attribute data.
     *
     * @param type
     * @param dataToSet
     * @param dataLength
     * @return int
     */
    int setData(CDFType type, const void *dataToSet, size_t dataLength);

    /**
     * @brief Sets one element in the attribute. The data type can be of anytype and will be converted to the type defined for the attribute.
     *
     * @tparam T
     * @param data The data value to set.
     * @return int
     */
    template <class T> int setData(T data) {
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

    /**
     * @brief Set a string value for this attribute
     *
     * @param The string to set.
     * @return int Zero on Success.
     */
    int setData(const char *dataToSet);

    /**
     * @brief Get the data from the attribute
     *
     * @tparam T
     * @param dataToGet
     * @param getlength
     * @return int
     */
    template <class T> int getData(T *dataToGet, size_t getlength) {
      if (data == NULL) return 0;
      if (getlength > length) getlength = length;
      CDF::DataCopierDestDataTemplated<T>::copy(dataToGet, data, type, getlength);
      return getlength;
    }

    /**
     * @brief Returns the attribute value as string using the pointer
     *
     * @param out Point to a CT::string
     * @return int
     */
    int getDataAsString(CT::string *out);

    /**
     * @brief Returns the attribute value as string
     *
     * @param out
     * @return int
     */
    CT::string toString();

    /**
     * @brief Returns the attribute value as string, same as toString
     *
     * @param out
     * @return int
     */
    CT::string getDataAsString();

    /**
     * @brief Data size in nr of elements for this attribute
     *
     * @return size_t
     */
    size_t size();
  };
} // namespace CDF
#endif
