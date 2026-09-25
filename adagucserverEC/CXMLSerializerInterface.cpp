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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "Definitions.h"
#include "CStopWatch.h"
#include "CXMLSerializerInterface.h"
#include "CTString.h"
#include "CDebugger.h"
int numXMLAttributesNotRecognized = 0;

int parseInt(const attribute &attrCfg) { return atoi(attrCfg.value.c_str()); }

bool parseBool(const attribute &attrCfg) {
  if (attrCfg.value.empty()) return false;
  return CT::toLowerCase(attrCfg.value) == "true";
}

double parseDouble(const attribute &attrCfg) {
  if (attrCfg.value.empty()) return 0;
  return (double)atof(attrCfg.value.c_str());
}

void parse_element_names(void *_a_node, CXMLObjectInterface *object, const std::string &datasetName) {
  xmlNode *a_node = (xmlNode *)_a_node;
  xmlNode *cur_node = NULL;
  CXMLObjectInterface *addedElement = nullptr;
  attribute attr; // Reused for every attribute, so its strings keep their capacity
  for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      char *content = cur_node->children != NULL && cur_node->children->content != NULL && cur_node->children->type == XML_TEXT_NODE ? (char *)cur_node->children->content : nullptr;
      addedElement = object->addElement((const char *)cur_node->name);
      if (addedElement != nullptr) {
        if (content != nullptr) {
          addedElement->elementValue = CT::trim(content);
        }
        addedElement->handleValue();
        for (xmlAttr *xmlAttribute = cur_node->properties; xmlAttribute != NULL; xmlAttribute = xmlAttribute->next) {
          if (xmlAttribute->children == NULL || xmlAttribute->children->content == NULL) continue;
          attr.name = (const char *)xmlAttribute->name;
          attr.value = (const char *)xmlAttribute->children->content;
          if (addedElement->addAttribute(attr) == false) {
            CDBWarning("[LINT]: In [%s]: no matches for attribute [%s] in Element [%s]", datasetName.c_str(), attr.name.c_str(), (char *)cur_node->name);
            numXMLAttributesNotRecognized++;
          }
        }
      } else {
        CDBWarning("In [%s]: no matches for Element [%s]", datasetName.c_str(), (char *)cur_node->name);
      }
    }
    if (addedElement != nullptr) {
      parse_element_names(cur_node->children, addedElement, datasetName);
    }
  }
}

int parseConfig(CXMLObjectInterface *object, const std::string &xmlData, const std::string &datasetName) {
  LIBXML_TEST_VERSION
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;

  if (adagucMeasureTime) {
    StopWatch_Stop("Start xmlReadMemory");
  }
  doc = xmlReadMemory(xmlData.c_str(), xmlData.length(), nullptr, nullptr, 0);
  if (adagucMeasureTime) {
    StopWatch_Stop("Done xmlReadMemory");
  }
  if (doc == NULL) {
    CDBError("error: could not parse xmldata %s", xmlData.c_str());
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return 1;
  }
  root_element = xmlDocGetRootElement(doc);
  if (adagucMeasureTime) {
    StopWatch_Stop("start parse_element_names");
  }
  parse_element_names(root_element, object, datasetName);
  if (adagucMeasureTime) {
    StopWatch_Stop("done parse_element_names");
  }
  xmlFreeDoc(doc);
  xmlCleanupParser();
  return 0;
}
