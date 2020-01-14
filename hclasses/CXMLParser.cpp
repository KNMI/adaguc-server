
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
/**
 * Static function which converts an exception into a readable message
 * @param int The value of catched exception 
 * @return CT::string with the readable message
 */
CT::string CXMLParser::getErrorMessage(int CXMLParserException){
  CT::string message = "Unknown error";
  if(CXMLParserException == CXMLPARSER_ATTR_NOT_FOUND)message="CXMLPARSER_ATTR_NOT_FOUND";
  if(CXMLParserException == CXMLPARSER_ELEMENT_NOT_FOUND)message="CXMLPARSER_ELEMENT_NOT_FOUND";
  if(CXMLParserException == CXMLPARSER_ATTRIBUTE_OUT_OF_BOUNDS)message="CXMLPARSER_ATTRIBUTE_OUT_OF_BOUNDS";
  if(CXMLParserException == CXMLPARSER_ELEMENT_OUT_OF_BOUNDS)message="CXMLPARSER_ELEMENT_OUT_OF_BOUNDS";
  if(CXMLParserException == CXMLPARSER_INVALID_XML)message="CXMLPARSER_INVALID_XML";
  
  return message;
}

CXMLParser::XMLAttribute::XMLAttribute(){}

CXMLParser::XMLAttribute::XMLAttribute(const char *name,const char *value){
  this->name=name;
  this->value=value;
}

CXMLParser::XMLAttribute CXMLParser::XMLAttributes::get(size_t nr){
  if(nr>size()||nr+1>size())throw CXMLPARSER_ATTRIBUTE_OUT_OF_BOUNDS;
  return (*this)[nr];
}
void CXMLParser::XMLAttributes::add(XMLAttribute attribute){
  this->push_back(attribute);
}

void CXMLParser::XMLElement::copy(XMLElement const &f){
  this->name=f.name;
  this->value=f.value;
  for(size_t j=0;j<f.xmlElements.size();j++)xmlElements.push_back(XMLElement(f.xmlElements[j]));
  for(size_t j=0;j<f.xmlAttributes.size();j++)xmlAttributes.add(XMLAttribute(f.xmlAttributes[j]));
}



CXMLParser::XMLElement::XMLElement(){
}

// CXMLParser::XMLElement CXMLParser::XMLElement::XMLElements::get(size_t nr){
//   if(nr>size()||nr+1>size())throw CXMLPARSER_ELEMENT_OUT_OF_BOUNDS;
//   return (*this)[nr];
// }
// 
// 
// void CXMLParser::XMLElement::XMLElements::add(XMLElement element){
//   this->push_back(element);
// }


CXMLParser::XMLElement *CXMLParser::XMLElement::XMLElementPointerList::get(size_t nr){
  if(nr>size()||nr+1>size())throw CXMLPARSER_ELEMENT_OUT_OF_BOUNDS;
  return (*this)[nr];
}
void CXMLParser::XMLElement::XMLElementPointerList::add(XMLElement *element){
  this->push_back(element);
}


/**
 * getFirst returns the first XMLElement
 */
// CXMLParser::XMLElement CXMLParser::XMLElement::XMLElements::getFirst(){
//   if(size()==0)throw CXMLPARSER_ELEMENT_OUT_OF_BOUNDS;
//   return this->get(0);
// }

/**
 * getFirst returns the last XMLElement
 */
// CXMLParser::XMLElement CXMLParser::XMLElement::XMLElements::getLast(){
//   if(size()==0)throw CXMLPARSER_ELEMENT_OUT_OF_BOUNDS;
//   return this->get(this->size()-1);
// }

/**
 * Constructor which parses libXmlNode
 * @param xmlNode The libXML node to parse
 * @param depth Current recursive depth of the node
 */
CXMLParser::XMLElement::XMLElement(void* _a_node,int depth){
  xmlNode * a_node = (xmlNode*)_a_node;
  parse_element_names( a_node, depth);
}

/**
 * Parses the attributes of the libXML attribute and adds them to the XMLelement
 * @param xmlAttr the libXML attribute to parse
 */
void CXMLParser::XMLElement::parse_element_attributes(void *_a_node){
  xmlAttr *a_node=(xmlAttr*)_a_node;
  char *content=NULL;
  char *name = NULL;
  name=(char*)a_node->name;
  if(a_node->children!=NULL)content=(char*)a_node->children->content;
  if(content!=NULL){
    xmlAttributes.push_back(XMLAttribute(name,content));
  }
  a_node=a_node->next;
  if(a_node!=NULL)this->parse_element_attributes(a_node);
}

/**
 * Parses the lib xmlElements to XMLElement recursively
 * @param xmlNode the libXML node to parse
 * @param depth the current recursive depth of the node
 */
void CXMLParser::XMLElement::parse_element_names(void *_a_node,int depth){
  xmlNode * a_node=(xmlNode*)_a_node;
  xmlNode *cur_node = NULL;
  for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      char *content=NULL;
      if(cur_node->children!=NULL && cur_node->children->content!=NULL && cur_node->children->type==XML_TEXT_NODE){
        content=(char*)cur_node->children->content;
      }
          
      XMLElement child;
      if(cur_node->name){
        child.name=(char*)cur_node->name;
      }
      child.value=(char*)content;
      child.parse_element_names(cur_node->children,depth+1);
      if(cur_node->properties!=NULL){
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
CT::string CXMLParser::XMLElement::toXML(XMLElement el,int depth){
  CT::string data = "";
  bool hasValue=false;
  if(el.getValue().replace("\n","").trim().length()>0){
    hasValue=true;
  }

  for(int i=0;i<depth;i++)data+="  ";
  data+=CT::string("<")+el.name;
  for(size_t j=0;j<el.getAttributes().size();j++){
    data+=CT::string(" ")+el.getAttributes().get(j).name+CT::string("=\"")+el.getAttributes().get(j).value.encodeXML()+"\"";
  }
  if(!hasValue){
    data+=">\n";
  }else{
    data+=">";
  }
  
  for(size_t j=0;j<el.xmlElements.size();j++){
    data+=toXML(el.xmlElements[j],depth+1);
  }
  
  if(hasValue){
    data+=el.getValue();
    data+= CT::string("</")+el.name+">\n";
  }else{
    for(int i=0;i<depth;i++){data+="  ";};data+= CT::string("</")+el.name+">\n";
  }
  return data;
}

/**
 * toString converts the current XMLElement to string
 */
CT::string CXMLParser::XMLElement::toString(){
  CT::string data ="";
  data="<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  data+=toXML((*this),0);
  return data;
}


CT::string CXMLParser::XMLElement::toJSON(XMLElement el,int depth,int mode){
  CT::string data;
  std::vector<CT::string>done;

  if(el.xmlAttributes.size()>0){
    CXMLParser::XMLElement xmlattr("xmlattr");
    for(size_t j=0;j<el.xmlAttributes.size();j++){
        //xmlattr.add(CXMLParser::XMLElement(el.xmlAttributes[j].name.c_str(),el.xmlAttributes[j].value.c_str()));
      xmlattr.add(CXMLParser::XMLElement(el.xmlAttributes[j].name.c_str(),el.xmlAttributes[j].value.c_str()));
      
    }
    el.add(xmlattr);
  }

  for(size_t j=0;j<el.xmlElements.size();j++){
    const char *name = el.xmlElements[j].name.c_str();
    bool alreadyDone = false;
    for(size_t i=0;i<done.size();i++){
      if(done[i].equals(name)){alreadyDone = true;break;}
    }
    if(alreadyDone == false){
      done.push_back(name);
      CXMLParser::XMLElement::XMLElementPointerList els = el.getList(name);
      if(els.size()>1){
        if(j>0)data+=",";
        data+= "\"";data+=el.xmlElements[j].name.c_str();data+= "\"";
        data+=":[";
        for(size_t i=0;i<els.size();i++){
          CT::string value = els[i]->getValue().replace("\n","").trim();
          CT::string subdata = toJSON(*(els[i]),depth++,mode);;
          if(subdata.length()>0){
            if(i>0)data+=",";
            data+="{";
            data+=&subdata;
            data+="}";
          }
          if(value.length()>0){
            if(i>0)data+=",";else{
              if(subdata.length()>0){
                data+=",";
              }
            }
            data+= "\"";
            data+= value.c_str();
            data+= "\"";
          }
        }
        data+="]";
      }else{
        bool hasValues = false;
        CT::string value = el.xmlElements[j].value;
        if(value.length()>0){
          value=value.replace("\n","").trim();
          if(value.length()>0){
            hasValues = true;
          }
        }
        if(j>0)data+=",";
        data+="\"";
        data+=name;
        data+="\":";
        if(hasValues){
          data+="\"";
          data+=value.c_str();
         data+="\"";
        }else{
          data+="{";
        }
        CT::string subdata = toJSON(el.xmlElements[j],depth++,mode);
        if(hasValues&&subdata.length()>0)data+=",";
        if(subdata.length()>0){
          data+=subdata;
        }
        if(!hasValues){
          data+="}";
        }
      }
    }
  }
  return data;
}

CT::string CXMLParser::XMLElement::toJSON(int mode){
  CT::string data ="";
  data="[{";
  data+=toJSON((*this),0,mode);
  data+="}]\n";
  return data;
}

/**
 * toString converts the current XMLElement to string
 */
CT::string CXMLParser::XMLElement::toStringNoHeader(){
  CT::string data ="";
  data+=toXML((*this),0);
  return data;
}

/**
 * getAttributes returns the xmlAttibute list of this element
 * @return the xmlAttibute list of this element
 */
CXMLParser::XMLAttributes CXMLParser::XMLElement::getAttributes(){
  return xmlAttributes;
}

/**
 * getElements returns the XMLElement list of this element
 * @return the XMLElement list of this element
 */
CXMLParser::XMLElement::XMLElementList* CXMLParser::XMLElement::getElements(){
  return &xmlElements;
}

/**
 * getAttrValue Returns the value of the attribute with the specified name
 * Throws CXMLPARSER_ATTR_NOT_FOUND if attribute was not found.
 * @param name the name of the attribute to search for
 */
CT::string CXMLParser::XMLElement::getAttrValue(const char *name){
  for(size_t j=0;j<xmlAttributes.size();j++){
    if(xmlAttributes[j].name.equals(name))return xmlAttributes[j].value;
  }
  throw CXMLPARSER_ATTR_NOT_FOUND;
}

/**
 * getFirst returns the first XMLElement
 */
CXMLParser::XMLElement* CXMLParser::XMLElement::getFirst(){
  if(xmlElements.size()==0)throw CXMLPARSER_ELEMENT_OUT_OF_BOUNDS;
  return &xmlElements[0];
}

/**
 * getFirst returns the last XMLElement
 */
CXMLParser::XMLElement* CXMLParser::XMLElement::getLast(){
  if(xmlElements.size()==0)throw CXMLPARSER_ELEMENT_OUT_OF_BOUNDS;
  return &xmlElements[(xmlElements.size()-1)];
}

/**
 * getList returns all elements with the specified name
 * @param name The name of the elements to return
 */
CXMLParser::XMLElement::XMLElementPointerList CXMLParser::XMLElement::getList(const char *name){
  XMLElementPointerList elements;
  for(size_t j=0;j<xmlElements.size();j++){
    if(xmlElements[j].name.equals(name)){
      //printf("Found %s\n",name);
      elements.add(&xmlElements[j]);
    }
  }
  if(elements.size()==0){
    throw CXMLPARSER_ELEMENT_NOT_FOUND;
  }
  return elements;
}
/**
 * get returns the elements with the specified name
 * @param name The name of the elements to return
 */
CXMLParser::XMLElement *CXMLParser::XMLElement::get(const char *name){
  for(size_t j=0;j<xmlElements.size();j++){
    if(xmlElements[j].name.equals(name)){
      //printf("Found %s\n",name);
      return &xmlElements[j];
    }
  }
  throw CXMLPARSER_ELEMENT_NOT_FOUND;
}

/**
 * getName returns the name of this XML element
 */
CT::string CXMLParser::XMLElement::getName(){
  return name;
}

/**
 * getValue returns the value of this XML element
 */
CT::string CXMLParser::XMLElement::getValue(){
  if(value.empty()){
    return "";
  }
  return value;
}


/**
 * Parses a string to XMLElement structure
 * throws integer CXMLPARSER_INVALID_XML if invalid
 * @param data the CT::string
 * @return Zero means succesfully parsed
 */
int CXMLParser::XMLElement::parse(CT::string data){
  return parse(data.c_str(),data.length());
}

/**
 * Parses a string to XMLElement structure
 * throws integer CXMLPARSER_INVALID_XML if invalid
 * @param *data pointer to the CT::string
 * @return Zero means succesfully parsed
 */
int CXMLParser::XMLElement::parse(CT::string *data){
  return parse(data->c_str(),data->length());
}

/**
 * Parses a string of given size to XMLElement structure
 * throws integer CXMLPARSER_INVALID_XML if invalid
 * @param xmlData The XML data as string to parse
 * @param xmlSize The size of the XML data
 * @return Zero means succesfully parsed
 */
int CXMLParser::XMLElement::parse(const char *xmlData,size_t xmlSize){
  LIBXML_TEST_VERSION
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;
  doc = xmlParseMemory(xmlData,xmlSize);
  if (doc == NULL) {
    xmlCleanupParser();
    throw(CXMLPARSER_INVALID_XML);
    return 1;
  }
  root_element = xmlDocGetRootElement(doc);
  name="root";
  value="";
  parse_element_names(root_element,0);
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
int CXMLParser::XMLElement::parse(const char *xmlFile){
  LIBXML_TEST_VERSION
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;
  doc = xmlParseFile(xmlFile);
  if (doc == NULL) {
    xmlCleanupParser();
    throw(CXMLPARSER_INVALID_XML);
    return 1;
  }
  root_element = xmlDocGetRootElement(doc);
  name="root";
  value="";
  parse_element_names(root_element,0);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  return 0;
}
