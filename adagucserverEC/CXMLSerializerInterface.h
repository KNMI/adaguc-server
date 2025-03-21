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
    pt2Class = new XMLE_##variableName();                                                                                                                                                              \
    pt2Class->level = rc;                                                                                                                                                                              \
    pt2Class->parentName = #variableName;                                                                                                                                                              \
    variableName.push_back(((XMLE_##variableName *)pt2Class));                                                                                                                                         \
  }
#define XMLE_SETOBJ(variableName)                                                                                                                                                                      \
  {                                                                                                                                                                                                    \
    if (variableName.size() == 0) {                                                                                                                                                                    \
      pt2Class = new XMLE_##variableName();                                                                                                                                                            \
      pt2Class->level = rc;                                                                                                                                                                            \
      variableName.push_back(((XMLE_##variableName *)pt2Class));                                                                                                                                       \
    } else {                                                                                                                                                                                           \
      pt2Class = variableName[0];                                                                                                                                                                      \
    }                                                                                                                                                                                                  \
  }
#define XMLE_DELOBJ(variableName)                                                                                                                                                                      \
  {{for (size_t j = 0; j < variableName.size(); j++){delete variableName[j];                                                                                                                           \
  }                                                                                                                                                                                                    \
  }                                                                                                                                                                                                    \
  }

/**
 * Base objects
 */
class CXMLObjectInterface {
public:
  CXMLObjectInterface();
  virtual ~CXMLObjectInterface() {}
  int level;
  CT::string value;
  CT::string parentName;
  CXMLObjectInterface *pt2Class;
  virtual void addElement(CXMLObjectInterface *baseClass, int rc, const char *name, const char *value);
  virtual void addAttribute(const char *, const char *) {}
};

/**
 * Serializes XML according to a defined class structure to nested lists of objects
 * Inherits the CXMLObjectInterface base object
 */
class CXMLSerializerInterface : public CXMLObjectInterface {
private:
  int recursiveDepth;
  void parse_element_attributes(void *a_node);
  void parse_element_names(void *a_node);
  DEF_ERRORFUNCTION();

public:
  // Functions specfically for CXMLSerializer
  static bool equals(const char *val1, const char *val2);
  CXMLObjectInterface *currentNode;
  CXMLSerializerInterface *baseClass;
  virtual void addElementEntry(int rc, const char *name, const char *value) = 0;
  virtual void addAttributeEntry(const char *parentName, const char *name, const char *value) = 0;

  int parse(const char *xmlData, size_t xmlSize);
  int parseFile(const char *xmlFile);

  CXMLSerializerInterface();
};

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

#endif
