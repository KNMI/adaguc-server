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

#ifndef CCDFTYPES_DEBUG
#define CCDFTYPES_DEBUG

#include <cstdint>
#ifndef ubyte
#define ubyte uint8_t
#endif

#include <cstdio>
#include <cstddef>
#include <vector>
#include <iostream>
#include <cstdlib>

#include "CTString.h"

// #define CCDFDATAMODEL_DEBUG
//  CDF: Common Data Format

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

// This is an X-Macro to automatically enumerate all template type mappings used in CDF
#define ENUMERATE_OVER_CDFTYPES(DO)                                                                                                                                                                    \
  DO(CDF_CHAR, char)                                                                                                                                                                                   \
  DO(CDF_BYTE, int8_t)                                                                                                                                                                                 \
  DO(CDF_UBYTE, ubyte)                                                                                                                                                                                 \
  DO(CDF_SHORT, short)                                                                                                                                                                                 \
  DO(CDF_USHORT, ushort)                                                                                                                                                                               \
  DO(CDF_INT, int)                                                                                                                                                                                     \
  DO(CDF_UINT, uint)                                                                                                                                                                                   \
  DO(CDF_INT64, long)                                                                                                                                                                                  \
  DO(CDF_UINT64, ulong)                                                                                                                                                                                \
  DO(CDF_FLOAT, float)                                                                                                                                                                                 \
  DO(CDF_DOUBLE, double)

/* Possible error codes, thrown by CDF */
typedef int CDFError;
#define CDF_E_NONE 1000        /* Unknown */
#define CDF_E_ERROR 1001       /* Unknown */
#define CDF_E_VARNOTFOUND 1002 /* Variable not found */
#define CDF_E_DIMNOTFOUND 1003 /* Dimension not found */
#define CDF_E_ATTNOTFOUND 1004 /* Attribute not found */
#define CDF_E_NRDIMSNOTEQUAL 1005
#define CDF_E_VARHASNOPARENT 1006 /*Variable has no parent CDFObject*/
#define CDF_E_VARHASNODATA 1007   /*Variable has no data*/

namespace CDF {
  // Allocates data for an array, provide type, the empty array and length
  // Data must be freed by using free()
  int allocateData(CDFType type, void **p, size_t length);
  int freeData(void **p);

  // Check if this is a string type or a numeric type
  bool isCDFNumeric(CDFType type);

  int fill(void *destdata, CDFType destType, double value, size_t size);

  /*Puts the CDF name of the type in the string array with name (CDF_FLOAT, CDF_INT, etc...)*/
  void getCDFDataTypeName(char *name, const size_t maxlen, const int type);

  /*Puts the C name of the type in the string array with name (float, int, etc...)*/
  void getCDataTypeName(char *name, const size_t maxlen, const int type);

  /**
   * Static function which converts an exception into a readable message
   * @param int The value of catched exception
   * @return string with the readable message
   */
  std::string getErrorMessage(const int errorCode);

  /**
   * returns the type name as string
   * @param type The CDF type
   * @return string with the name
   */
  CT::string getCDFDataTypeName(const int type);

  /*Returns the number of bytes needed for a single element of this datatype*/
  int getTypeSize(CDFType type);

  int fill(void *destdata, CDFType destType, double value, size_t size);

}; // namespace CDF

#endif
