/* 
 * Copyright (C) 2012, Royal Netherlands Meteorological Institute (KNMI)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or any 
 * later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Project    : ADAGUC
 *
 * initial programmer :  M.Plieger
 * initial date       :  20120610
 */

#ifndef CXMLPARSER_H
#define CXMLPARSER_H
#include <iostream>
#include <vector>
#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <CTypes.h>

#define CXMLPARSER_ATTR_NOT_FOUND 1
#define CXMLPARSER_ELEMENT_NOT_FOUND 2
#define CXMLPARSER_ATTRIBUTE_OUT_OF_BOUNDS 3
#define CXMLPARSER_ELEMENT_OUT_OF_BOUNDS 4
#define CXMLPARSER_INVALID_XML 6


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
    printf("xml:\n%s\n",element.getFirst().toString().c_str());
    printf("%s\n",element.get("playlist").getList("song").get(1).toString().c_str());
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
class CXMLParser{
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
      XMLAttribute(const char *name,const char *value);
      CT::string value;
      CT::string name;
  };
  
  /**
   *  List/Vector of XML Attributes
   */
  class XMLAttributes:public std::vector<XMLAttribute> {
  public:
    XMLAttribute get(size_t nr);
    void add(XMLAttribute attribute);
  };
  
  /**
   * XML Element 
   */
  class XMLElement{
    public:
     void copy(XMLElement const &f);
      
     XMLElement& operator= (XMLElement const& f){
       if (this == &f) return *this;   /* Gracefully handle self assignment*/
      copy(f);
      return *this;
     }
     
      XMLElement();
      
    /**
     *  List/Vector of XML Elements
     */
    class XMLElements:public std::vector<XMLElement> {
      public:
        XMLElement get(size_t nr);
        void add(XMLElement element);
        /**
         * getFirst returns the first XMLElement
         */
        XMLElement getFirst();
    };
    
    private:
    XMLElements xmlElements;
    XMLAttributes xmlAttributes;

    CT::string value;
    CT::string name;
    
    public:
    /**
     * Constructor which parses libXmlNode
     * @param xmlNode The libXML node to parse
     * @param depth Current recursive depth of the node
     */
    XMLElement(xmlNode * a_node,int depth);
 
  
    private:
    /**
     * Parses the attributes of the libXML attribute and adds them to the XMLelement
     * @param xmlAttr the libXML attribute to parse
     */
    void parse_element_attributes(xmlAttr * a_node);
     
    /**
     * Parses the lib xmlElements to XMLElement recursively
     * @param xmlNode the libXML node to parse
     * @param depth the current recursive depth of the node
     */
    void parse_element_names(xmlNode * a_node,int depth);
      
    private: 
    /**
     * Converts XMLElements and attributes to a string recursively
     * @param el The XMLElement to convert
     * @param depth the current recursive depth
     */
    CT::string toXML(XMLElement el,int depth);
          
    public: 
    /**
     * toString converts the current XMLElement to string
     */
    CT::string toString();
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
    XMLElements getElements();
    
    /**
     * getAttrValue Returns the value of the attribute with the specified name
     * Throws CXMLPARSER_ATTR_NOT_FOUND if attribute was not found.
     * @param name the name of the attribute to search for
     */
    CT::string getAttrValue(const char *name);
    
    /**
     * getFirst returns the first XMLElement
     */
    XMLElement getFirst();
    
    /**
     * getList returns all elements with the specified name
     * @param name The name of the elements to return
     */
    XMLElements getList(const char *name);
    /**
     * getList returns all elements with the specified name
     * @param name The name of the elements to return
     */
    XMLElement get(const char *name);
    
    /**
     * getName returns the name of this XML element
     */
    CT::string getName();
    
    /**
     * getValue returns the value of this XML element
     */
    CT::string getValue();
    
  
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
    int parse(const char *xmlData,size_t xmlSize);
    
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
