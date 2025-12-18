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
#include <stdio.h>

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

  CT::string xmlData=
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
    CT::string message=CXMLParser::getErrorMessage(e);
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
   * @return CT::string with the readable message
   */
  static CT::string getErrorMessage(int CXMLParserException);

  /**
   * XML Attribute
   */
  class XMLAttribute {
  public:
    XMLAttribute();
    XMLAttribute(const char *name, const char *value);
    CT::string value;
    CT::string name;
  };

  /**
   *  List/Vector of XML Attributes
   */
  class XMLAttributes : public std::vector<XMLAttribute> {
  public:
    XMLAttribute get(size_t nr);
    void add(XMLAttribute attribute);
  };

  /**
   * XML Element
   */
  class XMLElement {
  public:
    void copy(XMLElement const &f);

    XMLElement &operator=(XMLElement const &f) {
      if (this == &f) return *this; /* Gracefully handle self assignment*/
      copy(f);
      return *this;
    }

    XMLElement(XMLElement const &f) {
      if (this == &f) return;
      copy(f);
    }

    XMLElement();

    XMLElement(const char *name) { this->name = name; }

    XMLElement(const char *name, const char *value) {
      this->name = name;
      this->value = value;
    }

    /**
     *  List/Vector of XML Elements
     */
    class XMLElementList : public std::vector<XMLElement> {};

    /**
     *  List/Vector of XML Element pointers
     */
    class XMLElementPointerList : public std::vector<XMLElement *> {
    public:
      XMLElement *get(size_t nr);
      void add(XMLElement *element);
      CT::string toJSON(int mode) {
        CT::string json = "[";
        for (size_t j = 0; j < size(); j++) {
          if (j > 0) json += ",";
          CT::string subdata = get(j)->toJSON(mode);
          json.concatlength((subdata.c_str() + 1), subdata.length() - 3);
        }
        json += "]";
        return json;
      }
    };

  private:
    XMLElementList xmlElements;
    XMLAttributes xmlAttributes;

    CT::string value;
    CT::string name;

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
    CT::string toXML(XMLElement el, int depth);

    /**
     * Converts XMLElements and attributes to a jsonstring recursively
     * @param el The XMLElement to convert
     * @param depth the current recursive depth
     */
    CT::string toJSON(XMLElement el, int depth, int mode);

  public:
    /**
     * toString converts the current XMLElement to string
     */
    CT::string toString();

    /**
     * toJSON converts the current XMLElement to json
     */
    CT::string toJSON(int mode);

    /**
     * toString converts the current XMLElement to string
     */
    CT::string toStringNoHeader();

    /**
     * getAttributes returns the xmlAttibute list of this element
     * @return the xmlAttibute list of this element
     */
    XMLAttributes getAttributes();

    /**
     * getElements returns the XMLElement list of this element
     * @return the XMLElement list of this element
     */
    XMLElementList *getElements();

    /**
     * getAttrValue Returns the value of the attribute with the specified name
     * Throws CXMLPARSER_ATTR_NOT_FOUND if attribute was not found.
     * @param name the name of the attribute to search for
     */
    CT::string getAttrValue(const char *name);

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
    XMLElementPointerList getList(const char *name);
    /**
     * getList returns all elements with the specified name
     * @param name The name of the elements to return
     */
    XMLElement *get(const char *name);

    /**
     * getName returns the name of this XML element
     */
    CT::string getName();

    /**
     * getValue returns the value of this XML element
     */
    CT::string getValue();

    /**
     * set Name and Value of XML element
     */
    void setNameValue(const char *name, const char *value) {
      this->name = name;
      this->value = value;
    }

    /**
     * Set the name of the XML element
     */
    void setName(const char *name) { this->name = name; }

    /**
     * Set the value of the xml element
     */
    void setValue(const char *value) { this->value = value; }

    /**
     * Add XMLElement
     */
    XMLElement *add(XMLElement el) {
      xmlElements.push_back(el);
      return &xmlElements[xmlElements.size() - 1];
    }

    /**
     * Add XMLElement with name and value
     */
    XMLElement *add(const char *name, const char *value) {
      XMLElement el;
      el.setNameValue(name, value);
      xmlElements.push_back(el);
      return &xmlElements[xmlElements.size() - 1];
    }

    /**
     * Add xmlAttibute
     */
    void add(XMLAttribute at) { xmlAttributes.add(at); }

    /**
     * Parses a string to XMLElement structure
     * throws integer CXMLPARSER_INVALID_XML if invalid
     * @param data the CT::string
     * @return Zero means succesfully parsed
     */
    int parse(CT::string data);

    /**
     * Parses a string to XMLElement structure
     * throws integer CXMLPARSER_INVALID_XML if invalid
     * @param *data pointer to the CT::string
     * @return Zero means succesfully parsed
     */
    int parse(CT::string *data);

    /**
     * Parses a string of given size to XMLElement structure
     * throws integer CXMLPARSER_INVALID_XML if invalid
     * @param xmlData The XML data as string to parse
     * @param xmlSize The size of the XML data
     * @return Zero means succesfully parsed
     */
    int parse(const char *xmlData, size_t xmlSize);

    /**
     * Parses a file to XMLElement structure
     * throws integer CXMLPARSER_INVALID_XML if invalid
     * @param xmlFile The XML file location
     * @return Zero means succesfully parsed
     */
    int parse(const char *xmlFile);
  };
};

#define CXMLParserElements CXMLParser::XMLElement::XMLElements
#define CXMLParserElement CXMLParser::XMLElement
#define CXMLParserAttribute CXMLParser::XMLAttribute

#endif
