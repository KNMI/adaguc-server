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
  for(size_t j=0;j<f.xmlElements.size();j++)xmlElements.add(XMLElement(f.xmlElements[j]));
  for(size_t j=0;j<f.xmlAttributes.size();j++)xmlAttributes.add(XMLAttribute(f.xmlAttributes[j]));
}



CXMLParser::XMLElement::XMLElement(){
}

CXMLParser::XMLElement CXMLParser::XMLElement::XMLElements::get(size_t nr){
  if(nr>size()||nr+1>size())throw CXMLPARSER_ELEMENT_OUT_OF_BOUNDS;
  return (*this)[nr];
}


void CXMLParser::XMLElement::XMLElements::add(XMLElement element){
  this->push_back(element);
}

/**
 * getFirst returns the first XMLElement
 */
CXMLParser::XMLElement CXMLParser::XMLElement::XMLElements::getFirst(){
  if(size()==0)throw CXMLPARSER_ELEMENT_OUT_OF_BOUNDS;
  return this->get(0);
}

/**
 * Constructor which parses libXmlNode
 * @param xmlNode The libXML node to parse
 * @param depth Current recursive depth of the node
 */
CXMLParser::XMLElement::XMLElement(xmlNode * a_node,int depth){
  parse_element_names( a_node, depth);
}

/**
 * Parses the attributes of the libXML attribute and adds them to the XMLelement
 * @param xmlAttr the libXML attribute to parse
 */
void CXMLParser::XMLElement::parse_element_attributes(xmlAttr * a_node){
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
void CXMLParser::XMLElement::parse_element_names(xmlNode * a_node,int depth){
  xmlNode *cur_node = NULL;
  for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      char *content=NULL;
      if(cur_node->children!=NULL)
        if(cur_node->children->content!=NULL)
          if(cur_node->children->type==XML_TEXT_NODE)
            content=(char*)cur_node->children->content;
          {
            XMLElement child;
            child.name=(char*)cur_node->name;
            child.value=(char*)content;
            child.parse_element_names(cur_node->children,depth+1);
            if(cur_node->properties!=NULL){
              child.parse_element_attributes(cur_node->properties);
            }
            xmlElements.push_back(child);
          }
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
  for(int i=0;i<depth;i++)data+="  ";
  data+=CT::string("<")+el.name;
  for(size_t j=0;j<el.getAttributes().size();j++){
    data+=CT::string(" ")+el.getAttributes().get(j).name+CT::string("=\"")+el.getAttributes().get(j).value+"\"";
  }
  data+=">\n";
  for(size_t j=0;j<el.xmlElements.size();j++){
    data+=toXML(el.xmlElements.get(j),depth+1);
  }
  
  if(el.getValue().replacer("\n","").trimr().length()>0){
    for(int i=0;i<depth;i++)data+="  ";
    data+=CT::string("  ")+el.getValue()+"\n";
  }
  for(int i=0;i<depth;i++)data+="  ";data+= CT::string("</")+el.name+">\n";
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
CXMLParser::XMLElement::XMLElements CXMLParser::XMLElement::getElements(){
  return xmlElements;
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
CXMLParser::XMLElement CXMLParser::XMLElement::getFirst(){
  return xmlElements.get(0);
}

/**
 * getList returns all elements with the specified name
 * @param name The name of the elements to return
 */
CXMLParser::XMLElement::XMLElements CXMLParser::XMLElement::getList(const char *name){
  XMLElements elements;
  for(size_t j=0;j<xmlElements.size();j++){
    if(xmlElements[j].name.equals(name)){
      //printf("Found %s\n",name);
      elements.add(xmlElements[j]);
    }
  }
  if(elements.size()==0){
    throw CXMLPARSER_ELEMENT_NOT_FOUND;
  }
  return elements;
}
/**
 * getList returns all elements with the specified name
 * @param name The name of the elements to return
 */
CXMLParser::XMLElement CXMLParser::XMLElement::get(const char *name){
  XMLElements elements;
  for(size_t j=0;j<xmlElements.size();j++){
    if(xmlElements[j].name.equals(name)){
      //printf("Found %s\n",name);
      return xmlElements[j];
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
    xmlFreeDoc(doc);
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
    xmlFreeDoc(doc);
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
