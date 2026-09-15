/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl, GST - GeoSpatialTeam KNMI
 * Date:     2026-09-10
 *
 ******************************************************************************
 *
 * Copyright 2026, Royal Netherlands Meteorological Institute (KNMI)
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
#include <cstdio>
#include <cstdlib>
#include <cstring>


extern int numXMLAttributesNotRecognized;

/**
 * Appends a newly constructed element to an XML object vector and returns it.
 */
template <typename T> T *addXmlObj(std::vector<T> &elements) {
  elements.emplace_back();
  return &elements.back();
}

/**
 * Returns the (single) element in an XML object vector, constructing it first if not yet present.
 */
template <typename T> T *setXmlObj(std::vector<T> &elements) {
  if (elements.empty()) {
    elements.emplace_back();
  }
  return &elements.back();
}

struct attribute {
  std::string name;
  std::string value;
};

int parseInt(const attribute &attrCfg);
/**
 * Base objects
 */
struct CXMLObjectInterface {
  std::string elementValue;
  virtual ~CXMLObjectInterface() {}
  virtual CXMLObjectInterface *addElement(const std::string &) { return nullptr; };
  virtual void handleValue() {};
  virtual bool addAttribute(const attribute &) { return false; }
};

/**
 * Serializes XML according to a defined class structure to nested lists of objects
 * Inherits the CXMLObjectInterface base object
 */

int parseConfig(CXMLObjectInterface *object, const std::string &xmlData, std::string datasetName);

/**
 * parses a character string to int
 * @param attribute to parse
 */
int parseInt(const attribute &attrCfg);

/**
 * parses a character string to float
 * @param attribute to parse
 */
float parseFloat(const attribute &attrCfg);

/**
 * parses a character string to double
 * @param attribute to parse
 */
double parseDouble(const attribute &attrCfg);

/**
 * parses a character string to bool
 * @param attribute to parse
 */
bool parseBool(const attribute &attrCfg);

#endif
