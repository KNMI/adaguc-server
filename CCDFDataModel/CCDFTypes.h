/******************************************************************************
 *
 * Project:  Generic adaguc common data format (ACDF)
 * Purpose:  Generic Data model to read netcdf and hdf5
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2021-12-31
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

#ifndef CCDFTYPES_DEBUG
#define CCDFTYPES_DEBUG

#include <stdio.h>
#include <stddef.h>
#include <vector>
#include <iostream>
#include <stdlib.h>

#include "CTypes.h"

/**
 * @brief CDF stands for Common Data Format (Also known as ADAGUC Common Data Format)
 *
 */

/* Types supported by CDF */
typedef int CDFType;
#define CDF_NONE 0     /* Unknown */
#define CDF_BYTE 1     /* signed 1 byte integer */
#define CDF_CHAR 2     /* ISO/ASCII character */
#define CDF_SHORT 3    /* signed 2 byte integer */
#define CDF_INT 4      /* signed 4 byte integer */
#define CDF_FLOAT 5    /* single precision floating point number */
#define CDF_DOUBLE 6   /* double precision floating point number */
#define CDF_UBYTE 7    /* unsigned 1 byte int */
#define CDF_USHORT 8   /* unsigned 2-byte int */
#define CDF_UINT 9     /* unsigned 4-byte int */
#define CDF_STRING 10  /* variable string */
#define CDF_UNKNOWN 11 /* Unknown type, using CDF_DOUBLE */
#define CDF_INT64 12   /* signed 8 byte integer */
#define CDF_UINT64 13  /* unsigned 8 byte integer */

/* Possible error codes, thrown by CDF */
typedef int CDFError;
#define CDF_E_NONE 0        /* Unknown */
#define CDF_E_ERROR 1       /* Unknown */
#define CDF_E_VARNOTFOUND 2 /* Variable not found */
#define CDF_E_DIMNOTFOUND 3 /* Dimension not found */
#define CDF_E_ATTNOTFOUND 4 /* Attribute not found */
#define CDF_E_NRDIMSNOTEQUAL 5
#define CDF_E_VARHASNOPARENT 6 /*Variable has no parent CDFObject*/
#define CDF_E_VARHASNODATA 7   /*Variable has no data*/
namespace CDF {

  /**
   * @brief Allocates data for an array, provide type, the empty array and length. Data must be freed by using CDF::freeData()
   *
   * @param type The CDFtype of the data
   * @param p pointer to the data
   * @param length The requested size
   * @return int Zero on success
   */
  int allocateData(CDFType type, void **p, size_t length);

  /**
   * @brief Free the data allocated with allocateData.
   *
   * @param p Pointer to the data pointer.
   * @return int Zero on success
   */
  int freeData(void **p);

  /**
   * @brief A template class to autogenerate the code for all the possible types used in ACDF.
   *
   * @tparam T
   */
  template <typename T> class DataCopierDestDataTemplated {
  public:
    /**
     * @brief Copies data from sourcedata to destdata. The destdata is the template. The type of the sourcedata should be given with sourceType.
     *
     * @param destdata Pointer to the destination. Type is given via the template.
     * @param sourcedata Pointer to the sourcedata
     * @param sourcetype CDFType type of the source
     * @param destinationOffset Offset of the destination
     * @param sourceOffset Offset for the source
     * @param length The number of elements to copy.
     * @return int Zero on success.
     */
    static int copy(T *destdata, void *sourcedata, CDFType sourcetype, size_t destinationOffset, size_t sourceOffset, size_t length);

    /**
     * @brief Copies data from sourcedata to destdata. The destdata is the template. The type of the sourcedata should be given with sourceType.
     *
     * @param destdata Pointer to the destination. Type is given via the template.
     * @param sourcedata Pointer to the sourcedata
     * @param sourcetype CDFType type of the source
     * @param length The number of elements to copy.
     * @return int Zero on success.
     */
    static int copy(T *destdata, void *sourcedata, CDFType sourcetype, size_t length);

    /**
     * @brief Fills the array templated with T with the given value
     *
     * @tparam T
     * @param data Pointer to the data to fill, should have type Template T
     * @param value The value to fill
     * @param size The number of elements to fill
     */
    static void fill(T *data, double value, size_t size);
  };

  /**
   * @brief Copies data from one array to another and performs type conversion. Destdata must be a pointer to an empty array with non-void type
   *
   * @param destdata Pointer to the destination data, type indicated with destType
   * @param destType Type of the destination data
   * @param sourcedata Pointer to the source data, type indicated with sourceType
   * @param sourcetype Type of the source data
   * @param destinationOffset Offset in number of elemenets for the destination data
   * @param sourceOffset Offset in number of elemenets for the source data
   * @param length The number of elements to copy
   * @return int Zero on success.
   */
  int copy(void *destdata, CDFType destType, void *sourcedata, CDFType sourcetype, size_t destinationOffset, size_t sourceOffset, size_t length);

  /**
   * @brief Fills the array using the provided value with type given via destType
   *
   * @param destdata Pointer to the data
   * @param destType Type of the data
   * @param value The value to set
   * @param size Number of elements to fill
   * @return int Zero on success.
   */
  int fill(void *destdata, CDFType destType, double value, size_t size);

  /**
   * @brief Returns the CDF name (CDF_CHAR / CDF_DOUBLE / ...) for given CDFType typedef int.
   *
   * @param type The int representing CDFType
   * @return CT::string (CDF_CHAR / CDF_DOUBLE / ...)
   */
  CT::string getCDFDataTypeName(CDFType type);

  /**
   * @brief Returns the C name of the given CDF type
   *
   * @param type  The int representing CDFType
   * @return CT::string
   */
  CT::string getCDataTypeName(const int CDFType);

  /**
   * Static function which converts an exception into a readable message
   * @param int The value of catched exception
   * @return CT::string with the readable message
   */
  CT::string getErrorMessage(int errorCode);

  /**
   * returns the type name as string
   * @param type The CDF type
   * @return string with the name
   */
  CT::string getCDFDataTypeName(const int type);

  /**
   * @brief Returns the number of bytes needed for a single element of this datatype
   *
   * @param type The CDFtype
   * @return int The size in bytes of the CDFType
   */
  int getTypeSize(CDFType type);

}; // namespace CDF

#endif
