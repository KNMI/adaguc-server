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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "CXMLSerializerInterface.h"

int parseInt(const char *pszValue) {
  if (pszValue == NULL) return 0;
  int dValue = atoi(pszValue);
  return dValue;
}

float parseFloat(const char *pszValue) {
  if (pszValue == NULL) return 0;
  float fValue = (float)atof(pszValue);
  return fValue;
}

double parseDouble(const char *pszValue) {
  if (pszValue == NULL) return 0;
  double fValue = (double)atof(pszValue);
  return fValue;
}

bool parseBool(const char *pszValue) {
  if (pszValue == NULL) return false;
  return CT::string(pszValue).equalsIgnoreCase("true");
}

void parse_element_attributes(void *_a_node, std::vector<attribute> &attributes) {
  xmlAttr *a_node = (xmlAttr *)_a_node;
  char *content = NULL;
  char *name = NULL;
  name = (char *)a_node->name;
  if (a_node->children != NULL) content = (char *)a_node->children->content;
  if (content != NULL) {
    attributes.push_back({.name = name, .value = content});
  }
  a_node = a_node->next;
  if (a_node != NULL) parse_element_attributes(a_node, attributes);
}

void parse_element_names(void *_a_node, CXMLObjectInterface *object) {
  xmlNode *a_node = (xmlNode *)_a_node;
  xmlNode *cur_node = NULL;
  CXMLObjectInterface *addedElement = nullptr;
  for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      char *content = cur_node->children != NULL && cur_node->children->content != NULL && cur_node->children->type == XML_TEXT_NODE ? (char *)cur_node->children->content : nullptr;
      addedElement = object->addElement((char *)cur_node->name, content);
      if (addedElement != nullptr) {
        if (addedElement->value.empty()) {
          addedElement->value = content;
          addedElement->value.trimSelf(true);
        }
        if (cur_node->properties != NULL) {
          std::vector<attribute> attributes;
          parse_element_attributes(cur_node->properties, attributes);
          for (auto &attribute : attributes) {
            if (addedElement->addAttribute(attribute.name.c_str(), attribute.value.c_str()) == false) {
              CDBWarning("No matches for attribute [%s] in Element [%s]", attribute.name.c_str(), (char *)cur_node->name);
            }
          }
        }
      } else {
        CDBWarning("No matches for Element [%s]", (char *)cur_node->name);
      }
    }
    if (addedElement != nullptr) {
      parse_element_names(cur_node->children, addedElement);
    }
  }
}

int parseConfig(CXMLObjectInterface *object, CT::string &xmlData) {
  LIBXML_TEST_VERSION
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;
  doc = xmlParseMemory(xmlData.c_str(), xmlData.length());
  if (doc == NULL) {
    CDBError("error: could not parse xmldata %s", xmlData);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return 1;
  }
  root_element = xmlDocGetRootElement(doc);
  parse_element_names(root_element, object);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  return 0;
}

bool equals(const char *val1, const char *val2) { return strcmp(val1, val2) == 0; }