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

#ifndef CXMLSerializerInterface_H
#define CXMLSerializerInterface_H
#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CTypes.h"
#include "CDebugger.h"

#define XMLE_ADDOBJ(variableName)                                                                                                                                                                      \
  {                                                                                                                                                                                                    \
    variableName.push_back(new XMLE_##variableName());                                                                                                                                                 \
    return variableName.back();                                                                                                                                                                        \
  }

#define XMLE_SETOBJ(variableName)                                                                                                                                                                      \
  {                                                                                                                                                                                                    \
    if (variableName.size() == 0) {                                                                                                                                                                    \
      variableName.push_back(new XMLE_##variableName());                                                                                                                                               \
      return variableName.back();                                                                                                                                                                      \
    } else {                                                                                                                                                                                           \
      return variableName.back();                                                                                                                                                                      \
    }                                                                                                                                                                                                  \
  }

#define XMLE_DELOBJ(variableName)                                                                                                                                                                      \
  {{for (size_t j = 0; j < variableName.size(); j++){delete variableName[j];                                                                                                                           \
  }                                                                                                                                                                                                    \
  }                                                                                                                                                                                                    \
  }

struct attribute {
  std::string name;
  std::string value;
};

/**
 * Base objects
 */
class CXMLObjectInterface {
public:
  CT::string value;
  virtual ~CXMLObjectInterface() {}

  virtual CXMLObjectInterface *addElement(const char *) { return nullptr; };
  virtual void handleValue() {};
  virtual bool addAttribute(const char *, const char *) { return false; }
};

/**
 * Serializes XML according to a defined class structure to nested lists of objects
 * Inherits the CXMLObjectInterface base object
 */

int parseConfig(CXMLObjectInterface *object, CT::string &xmlData);

/**
 * parses a character string to int
 * @param pszValue The string to parse
 */
int parseInt(const char *pszValue);

/**
 * parses a character string to float
 * @param pszValue The string to parse
 */
float parseFloat(const char *pszValue);

/**
 * parses a character string to double
 * @param pszValue The string to parse
 */
double parseDouble(const char *pszValue);

/**
 * parses a character string to bool
 * @param pszValue The string to parse
 */
bool parseBool(const char *pszValue);

// Functions specfically for CXMLSerializer
bool equals(const char *val1, const char *val2);

#endif
