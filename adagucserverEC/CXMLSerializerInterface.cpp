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
const char *CXMLSerializerInterface::className = "CXMLSerializerInterface";

CXMLObjectInterface::CXMLObjectInterface() { pt2Class = NULL; }

void CXMLObjectInterface::addElement(CXMLObjectInterface *baseClass, int rc, const char *, const char *value) {
  CXMLSerializerInterface *base = (CXMLSerializerInterface *)baseClass;
  base->currentNode = (CXMLObjectInterface *)this;
  if (rc == 0)
    if (value != NULL) {
      this->value.copy(value);
      this->value.trimWhiteSpacesAndLinesSelf();
    }
}

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

CXMLSerializerInterface::CXMLSerializerInterface() {
  pt2Class = this;
  baseClass = this;
  currentNode = NULL;
}

void CXMLSerializerInterface::parse_element_attributes(void *_a_node) {
  xmlAttr *a_node = (xmlAttr *)_a_node;
  char *content = NULL;
  char *name = NULL;
  name = (char *)a_node->name;
  if (a_node->children != NULL) content = (char *)a_node->children->content;
  if (content != NULL) addAttributeEntry(name, content);
  a_node = a_node->next;
  if (a_node != NULL) parse_element_attributes(a_node);
}

void CXMLSerializerInterface::parse_element_names(void *_a_node) {
  xmlNode *a_node = (xmlNode *)_a_node;
  xmlNode *cur_node = NULL;
  for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      char *content = NULL;
      if (cur_node->children != NULL)
        if (cur_node->children->content != NULL)
          if (cur_node->children->type == XML_TEXT_NODE) content = (char *)cur_node->children->content;
      addElementEntry(recursiveDepth, (char *)cur_node->name, content);
      if (cur_node->properties != NULL) parse_element_attributes(cur_node->properties);
    }
    recursiveDepth++;
    parse_element_names(cur_node->children);
    recursiveDepth--;
  }
}

int CXMLSerializerInterface::parse(const char *xmlData, size_t xmlSize) {
  recursiveDepth = 0;
  LIBXML_TEST_VERSION
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;
  doc = xmlParseMemory(xmlData, xmlSize);
  if (doc == NULL) {
    CDBError("error: could not parse xmldata %s", xmlData);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return 1;
  }
  root_element = xmlDocGetRootElement(doc);
  parse_element_names(root_element);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  return 0;
}
int CXMLSerializerInterface::parseFile(const char *xmlFile) {
  recursiveDepth = 0;
  LIBXML_TEST_VERSION
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;
  doc = xmlParseFile(xmlFile);
  if (doc == NULL) {
    CDBError("error: could not parse xmlFile %s", xmlFile);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return 1;
  }
  root_element = xmlDocGetRootElement(doc);
  parse_element_names(root_element);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  return 0;
}

bool CXMLSerializerInterface::equals(const char *val1, const char *val2) { return strcmp(val1, val2) == 0; }