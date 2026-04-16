
/******************************************************************************
 *
 * Project:  Helper classes
 * Purpose:  Generic functions
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

#include "CXMLParser.h"
#include <algorithm>
/**
 * Static function which converts an exception into a readable message
 * @param int The value of catched exception
 * @return std::string with the readable message
 */
std::string CXMLParser::getErrorMessage(int CXMLParserException) {
  std::string message = "Unknown error";
  if (CXMLParserException == CXMLPARSER_ATTR_NOT_FOUND) message = "CXMLPARSER_ATTR_NOT_FOUND";
  if (CXMLParserException == CXMLPARSER_ELEMENT_NOT_FOUND) message = "CXMLPARSER_ELEMENT_NOT_FOUND";
  if (CXMLParserException == CXMLPARSER_ATTRIBUTE_OUT_OF_BOUNDS) message = "CXMLPARSER_ATTRIBUTE_OUT_OF_BOUNDS";
  if (CXMLParserException == CXMLPARSER_ELEMENT_OUT_OF_BOUNDS) message = "CXMLPARSER_ELEMENT_OUT_OF_BOUNDS";
  if (CXMLParserException == CXMLPARSER_INVALID_XML) message = "CXMLPARSER_INVALID_XML";

  return message;
}

CXMLParser::XMLElement::XMLElement() {}

/**
 * Constructor which parses libXmlNode
 * @param xmlNode The libXML node to parse
 * @param depth Current recursive depth of the node
 */
CXMLParser::XMLElement::XMLElement(void *_a_node, int depth) {
  xmlNode *a_node = (xmlNode *)_a_node;
  parse_element_names(a_node, depth);
}

/**
 * Parses the attributes of the libXML attribute and adds them to the XMLelement
 * @param xmlAttr the libXML attribute to parse
 */
void CXMLParser::XMLElement::parse_element_attributes(void *_a_node) {
  xmlAttr *a_node = (xmlAttr *)_a_node;
  char *content = NULL;
  char *name = NULL;
  name = (char *)a_node->name;
  if (a_node->children != NULL) content = (char *)a_node->children->content;
  if (content != NULL) {
    xmlAttributes.push_back(XMLAttribute(name, content));
  }
  a_node = a_node->next;
  if (a_node != NULL) this->parse_element_attributes(a_node);
}

/**
 * Parses the lib xmlElements to XMLElement recursively
 * @param xmlNode the libXML node to parse
 * @param depth the current recursive depth of the node
 */
void CXMLParser::XMLElement::parse_element_names(void *_a_node, int depth) {
  xmlNode *a_node = (xmlNode *)_a_node;
  xmlNode *cur_node = NULL;
  for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      char *content = NULL;
      if (cur_node->children != NULL && cur_node->children->content != NULL && cur_node->children->type == XML_TEXT_NODE) {
        content = (char *)cur_node->children->content;
      }

      XMLElement child;
      if (cur_node->name) {
        child.name = (char *)cur_node->name;
      }
      child.value = (char *)content;
      child.parse_element_names(cur_node->children, depth + 1);
      if (cur_node->properties != NULL) {
        child.parse_element_attributes(cur_node->properties);
      }
      xmlElements.push_back(child);
    }
  }
}

/**
 * Converts XMLElements and attributes to a string recursively
 * @param el The XMLElement to convert
 * @param depth the current recursive depth
 */
std::string CXMLParser::XMLElement::toXML(XMLElement el, int depth) {
  std::string data = "";
  bool hasValue = false;
  if (CT::trim(CT::replace(el.value, "\n", "")).length() > 0) {
    hasValue = true;
  }

  for (int i = 0; i < depth; i++) data += "  ";
  data += "<" + el.name;
  for (size_t j = 0; j < el.xmlAttributes.size(); j++) {
    data += " " + el.xmlAttributes.at(j).name + "=\"" + CT::encodeXml(el.xmlAttributes.at(j).value) + "\"";
  }
  if (!hasValue) {
    data += ">\n";
  } else {
    data += ">";
  }

  for (size_t j = 0; j < el.xmlElements.size(); j++) {
    data += toXML(el.xmlElements[j], depth + 1);
  }

  if (hasValue) {
    data += el.value;
    data += "</" + el.name + ">\n";
  } else {
    for (int i = 0; i < depth; i++) {
      data += "  ";
    };
    data += "</" + el.name + ">\n";
  }
  return data;
}

/**
 * toString converts the current XMLElement to string
 */
std::string CXMLParser::XMLElement::toString() {
  std::string data = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  data += toXML((*this), 0);
  return data;
}

std::string CXMLParser::XMLElement::toJSON(const XMLElement &el, int depth, int mode) const {
  std::string data;
  std::vector<std::string> done;
  for (size_t j = 0; j < el.xmlElements.size(); j++) {
    const auto &name = el.xmlElements[j].name;
    if (std::find(done.begin(), done.end(), name) != done.end()) continue;
    done.push_back(name);
    const auto &els = el.getList(name);
    if (els.size() > 1) {
      if (j > 0) data += ",";
      data += "\"" + el.xmlElements[j].name + "\":[";
      for (size_t i = 0; i < els.size(); i++) {
        std::string value = CT::trim(CT::replace(els[i].value, "\n", ""));
        std::string subdata = toJSON((els[i]), depth++, mode);
        if (subdata.length() > 0) {
          if (i > 0) data += ",";
          data += "{" + subdata + "}";
        }
        if (value.length() > 0) {
          if (i > 0)
            data += ",";
          else {
            if (subdata.length() > 0) {
              data += ",";
            }
          }
          data += "\"" + value + "\"";
        }
      }
      data += "]";
    } else {
      bool hasValues = false;
      std::string value = el.xmlElements[j].value;
      if (value.length() > 0) {
        value = CT::trim(CT::replace(value, "\n", ""));
        if (value.length() > 0) {
          hasValues = true;
        }
      }
      if (j > 0) data += ",";
      data += "\"" + name + "\":";
      if (hasValues) {
        data += "\"" + value + "\"";
      } else {
        data += "{";
      }
      std::string subdata = toJSON(el.xmlElements[j], depth++, mode);
      if (hasValues && subdata.length() > 0) data += ",";
      if (subdata.length() > 0) {
        data += subdata;
      }
      if (!hasValues) {
        data += "}";
      }
    }
  }
  return data;
}

std::string CXMLParser::XMLElement::toJSON(int mode) const { return "[{" + toJSON((*this), 0, mode) + "}]\n"; }

/**
 * toString converts the current XMLElement to string
 */
std::string CXMLParser::XMLElement::toStringNoHeader() { return toXML((*this), 0); }

/**
 * getAttrValue Returns the value of the attribute with the specified name
 * Throws CXMLPARSER_ATTR_NOT_FOUND if attribute was not found.
 * @param name the name of the attribute to search for
 */
std::string CXMLParser::XMLElement::getAttrValue(const std::string &name) {
  auto it = std::find_if(xmlAttributes.begin(), xmlAttributes.end(), [&name](const auto &a) { return name == a.name; });
  if (it != xmlAttributes.end()) {
    return (*it).value;
  }

  throw CXMLPARSER_ATTR_NOT_FOUND;
}

/**
 * getFirst returns the first XMLElement
 */
CXMLParser::XMLElement *CXMLParser::XMLElement::getFirst() {
  if (xmlElements.size() == 0) throw CXMLPARSER_ELEMENT_OUT_OF_BOUNDS;
  return &xmlElements[0];
}

/**
 * getFirst returns the last XMLElement
 */
CXMLParser::XMLElement *CXMLParser::XMLElement::getLast() {
  if (xmlElements.size() == 0) throw CXMLPARSER_ELEMENT_OUT_OF_BOUNDS;
  return &xmlElements[(xmlElements.size() - 1)];
}

/**
 * getList returns all elements with the specified name
 * @param name The name of the elements to return
 */
std::vector<CXMLParser::XMLElement> CXMLParser::XMLElement::getList(const std::string &name) const {
  std::vector<CXMLParser::XMLElement> elements;
  for (size_t j = 0; j < xmlElements.size(); j++) {
    if (xmlElements[j].name == name) {
      elements.push_back(xmlElements[j]);
    }
  }
  if (elements.size() == 0) {
    throw CXMLPARSER_ELEMENT_NOT_FOUND;
  }
  return elements;
}
/**
 * get returns the elements with the specified name
 * @param name The name of the elements to return
 */

CXMLParser::XMLElement *CXMLParser::XMLElement::get(const std::string &name) {
  auto it = std::find_if(xmlElements.begin(), xmlElements.end(), [&name](const auto &a) { return name == a.name; });
  if (it != xmlElements.end()) {
    return &(*it);
  }
  return nullptr;
}

CXMLParser::XMLElement *CXMLParser::XMLElement::getThrows(const std::string &name) {
  auto el = get(name);
  if (el != nullptr) {
    return el;
  }
  throw CXMLPARSER_ELEMENT_NOT_FOUND;
}

/**
 * Parses a string to XMLElement structure
 * throws integer CXMLPARSER_INVALID_XML if invalid
 * @param xmlData The XML data as string to parse
 * @return Zero means succesfully parsed
 */
int CXMLParser::XMLElement::parseData(const std::string &xmlData) {
  LIBXML_TEST_VERSION
  xmlElements.clear();
  xmlAttributes.clear();
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;
  doc = xmlParseMemory(xmlData.c_str(), xmlData.length());
  if (doc == NULL) {
    xmlCleanupParser();
    throw(CXMLPARSER_INVALID_XML);
    return 1;
  }
  root_element = xmlDocGetRootElement(doc);
  name = "root";
  value = "";
  parse_element_names(root_element, 0);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  return 0;
}

/**
 * Parses a file to XMLElement structure
 * throws integer CXMLPARSER_INVALID_XML if invalid
 * @param xmlFile The XML file location
 * @return Zero means succesfully parsed
 */
int CXMLParser::XMLElement::parseFile(const std::string &filename) {
  LIBXML_TEST_VERSION
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;
  doc = xmlParseFile(filename.c_str());
  if (doc == NULL) {
    xmlCleanupParser();
    throw(CXMLPARSER_INVALID_XML);
    return 1;
  }
  root_element = xmlDocGetRootElement(doc);
  name = "root";
  value = "";
  parse_element_names(root_element, 0);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  return 0;
}

std::string xmlListToJSON(const std::vector<CXMLParser::XMLElement> &list, int mode) {
  std::string json = "[";
  for (size_t j = 0; j < list.size(); j++) {
    if (j > 0) json += ",";
    std::string subdata = list.at(j).toJSON(mode);
    json += CT::substring(subdata, 1, subdata.length() - 2);
  }
  json += "]";
  return json;
}