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

#ifndef CXMLPARSER_H
#define CXMLPARSER_H
#include <iostream>
#include <vector>
#include <cstdio>

#include "CTString.h"

#define CXMLPARSER_ATTR_NOT_FOUND 1
#define CXMLPARSER_ELEMENT_NOT_FOUND 2
#define CXMLPARSER_ATTRIBUTE_OUT_OF_BOUNDS 3
#define CXMLPARSER_ELEMENT_OUT_OF_BOUNDS 4
#define CXMLPARSER_INVALID_XML 6

#define CXMLPARSER_JSONMODE_STANDARD 0
#define CXMLPARSER_JSONMODE_CLASSIC 1

/*Example Usage:

#include "CXMLParser.h"

int main(){

  std::string xmlData=
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<playlist name=\"mylist\" xml:lang=\"en\">\n"
  "  <song>\n"
  "    <title>Little Fluffy Clouds</title>\n"
  "    <artist>the Orb</artist>\n"
  "  </song>\n"
  "  <song>\n"
  "    <title name=\"mylist\" xml:lang=\"en\">Goodbye mother Earth</title>\n"
  "    <artist A=\"B\">Underworld</artist>\n"
  "  </song>\n"
  "  <test>ok</test>\n"
  "</playlist>\n";

  CXMLParserElement element;

  try{
    element.parse(xmlData);
    printf("xml:\n%s\n",element.getFirst()->toString().c_str());
    printf("%s\n",element.get("playlist")->getList("song").get(1)->toString().c_str());
  }catch(int e){
    std::string message=CXMLParser::getErrorMessage(e);
    printf("%s\n",message.c_str());
  }

  return 0;
}


*/

/**
 * CXMLParser parses a XML file or XML data to a nested list of objects of type XMLElement.
 * Various functions are available to find elements+attributes and to walk through the tree.
 */
class CXMLParser {
public:
  /**
   * Static function which converts an exception into a readable message
   * @param int The value of catched exception
   * @return string with the readable message
   */
  static std::string getErrorMessage(int CXMLParserException);

  /**
   * XML Attribute
   */
  struct XMLAttribute {
    std::string value;
    std::string name;
  };

  /**
   * XML Element
   */
  class XMLElement {
  public:
    void copy(XMLElement const &f);
    XMLElement();
    XMLElement(const std::string &name) { this->name = name; }
    XMLElement(const std::string &name, const std::string &value) {
      this->name = name;
      this->value = value;
    }

  public:
    std::vector<XMLElement> xmlElements;
    std::vector<XMLAttribute> xmlAttributes;
    std::string value;
    std::string name;

  private:
    /**
     * Constructor which parses libXmlNode
     * @param xmlNode The libXML node to parse
     * @param depth Current recursive depth of the node
     */
    XMLElement(void *_a_node, int depth);

  private:
    /**
     * Parses the attributes of the libXML attribute and adds them to the XMLelement
     * @param xmlAttr the libXML attribute to parse
     */
    void parse_element_attributes(void *a_node);

    /**
     * Parses the lib xmlElements to XMLElement recursively
     * @param xmlNode the libXML node to parse
     * @param depth the current recursive depth of the node
     */
    void parse_element_names(void *_a_node, int depth);

  private:
    /**
     * Converts XMLElements and attributes to a string recursively
     * @param el The XMLElement to convert
     * @param depth the current recursive depth
     */
    std::string toXML(XMLElement el, int depth);

    /**
     * Converts XMLElements and attributes to a jsonstring recursively
     * @param el The XMLElement to convert
     * @param depth the current recursive depth
     */
    std::string toJSON(const XMLElement &el, int depth, int mode) const;

  public:
    /**
     * toString converts the current XMLElement to string
     */
    std::string toString();

    /**
     * toJSON converts the current XMLElement to json
     */
    std::string toJSON(int mode) const;

    /**
     * toString converts the current XMLElement to string
     */
    std::string toStringNoHeader();

    /**
     * getAttrValue Returns the value of the attribute with the specified name
     * Throws CXMLPARSER_ATTR_NOT_FOUND if attribute was not found.
     * @param name the name of the attribute to search for
     */
    std::string getAttrValue(const std::string &name);

    /**
     * getFirst returns the first XMLElement
     */
    XMLElement *getFirst();

    /**
     * getFirst returns the last XMLElement
     */
    XMLElement *getLast();

    /**
     * getList returns all elements with the specified name
     * @param name The name of the elements to return
     */
    std::vector<CXMLParser::XMLElement> getList(const std::string &name) const;
    /**
     * get returns all elements with the specified name
     * @param name The name of the elements to return or null if noit found
     */
    XMLElement *get(const std::string &name);
    /**
     * get returns all elements with the specified name
     * @throws exception if not found
     * @param name The name of the elements to return
     */
    XMLElement *getThrows(const std::string &name);

    /**
     * set Name and Value of XML element
     */
    void setNameValue(const std::string &name, const std::string &value) {
      this->name = name;
      this->value = value;
    }

    /**
     * Set the name of the XML element
     */
    void setName(const std::string &name) { this->name = name; }

    /**
     * Set the value of the xml element
     */
    void setValue(const std::string &value) { this->value = value; }

    /**
     * Add XMLElement
     */
    XMLElement &add(const XMLElement &el) {
      xmlElements.push_back(el);
      return xmlElements.back();
    }

    XMLElement &add(const std::string &name) {
      xmlElements.push_back(XMLElement(name));
      return xmlElements.back();
    }

    void add(std::string name, std::string value) { xmlElements.push_back(XMLElement(name.c_str(), value.c_str())); }
    /**
     * Add xmlAttibute
     */
    void add(const XMLAttribute &at) { xmlAttributes.push_back(at); }

    /**
     * Parses a string to XMLElement structure
     * throws integer CXMLPARSER_INVALID_XML if invalid
     * @param data the string
     * @return Zero means succesfully parsed
     */
    int parseData(const std::string &data);

    /**
     * Parses a file to XMLElement structure
     * throws integer CXMLPARSER_INVALID_XML if invalid
     * @param xmlFile The XML file location
     * @return Zero means succesfully parsed
     */
    int parseFile(const std::string &filename);
  };
};

#define CXMLParserElements CXMLParser::XMLElement::XMLElements
#define CXMLParserElement CXMLParser::XMLElement
#define CXMLParserAttribute CXMLParser::XMLAttribute

std::string xmlListToJSON(const std::vector<CXMLParser::XMLElement> &list, int mode);
#endif
