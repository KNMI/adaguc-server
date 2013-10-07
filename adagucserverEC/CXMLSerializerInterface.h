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

#ifndef CXMLSerializerInterface_H
#define CXMLSerializerInterface_H
#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "CDebugger.h"


#define XMLE_ADDOBJ(variableName){ pt2Class=new XMLE_##variableName();pt2Class->level=rc;variableName.push_back(((XMLE_##variableName*)pt2Class));}
#define XMLE_SETOBJ(variableName){ pt2Class=variableName[0];}
#define XMLE_DELOBJ(variableName){ {for(size_t j=0;j<variableName.size();j++){delete variableName[j];}}}

/**
 * Simple string element with limited functionality. All string values in CXMLObjectInterface will have this type.
 */
class CXMLString{
  private:
	char *p;
  public:

    CXMLString(){
      p=NULL;
    }
    ~CXMLString(){
      if(p!=NULL){free(p);p=NULL;}
    }
    void copy(const char *p){
      //if(this->p!=NULL){free(this->p);}//TODO
      this->p=strdup(p);
    }
    const char *c_str(){
      return (const char*)p;
    }
    bool empty(){
      if(p==NULL)return true;
      return false;
    }
    bool equals(const char *val2){
      if(p==NULL||val2==NULL)return false;
      size_t lenval1=strlen(p);
      size_t lenval2=strlen(val2);
      if(lenval1!=lenval2)return false;
      for(size_t j=0;j<lenval1;j++)if(p[j]!=val2[j])return false;
      return true;
    }
};

/**
 * Base objects
 */
class CXMLObjectInterface{
  public:
    CXMLObjectInterface(){
      pt2Class=NULL;
    }
    virtual ~CXMLObjectInterface(){}
    int level;
    CXMLString value;
    CXMLObjectInterface* pt2Class;
    virtual void addElement(CXMLObjectInterface *baseClass,int rc, const char *name,const char *value);
    virtual void addAttribute(const char *name,const char *value){}
};

/**
 * Serializes XML according to a defined class structure to nested lists of objects 
 * Inherits the CXMLObjectInterface base object
 */
class CXMLSerializerInterface:public CXMLObjectInterface{
  private:
    int recursiveDepth;
    void parse_element_attributes(xmlAttr * a_node){
      char *content=NULL;
      char *name = NULL;
      name=(char*)a_node->name;
      if(a_node->children!=NULL)content=(char*)a_node->children->content;
      if(content!=NULL)addAttributeEntry(name,content);
      a_node=a_node->next;
      if(a_node!=NULL)parse_element_attributes(a_node);
    }
    void parse_element_names(xmlNode * a_node){
      xmlNode *cur_node = NULL;
      for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
          char *content=NULL;
          if(cur_node->children!=NULL)
            if(cur_node->children->content!=NULL)
              if(cur_node->children->type==XML_TEXT_NODE)
                content=(char*)cur_node->children->content;
          addElementEntry(recursiveDepth,(char*)cur_node->name,content);
          if(cur_node->properties!=NULL)
            parse_element_attributes(cur_node->properties);
        }
        recursiveDepth++;
        parse_element_names(cur_node->children);
        recursiveDepth--;
      }
    }
    DEF_ERRORFUNCTION();
  public:
  //Functions specfically for CXMLSerializer
  static bool equals(const char *val1,size_t lenval1,const char *val2){
    size_t lenval2=strlen(val2);
    if(lenval1!=lenval2)return false;
    for(size_t j=0;j<lenval1;j++)if(val1[j]!=val2[j])return false;
    return true;
  }
  CXMLObjectInterface *currentNode;
  CXMLSerializerInterface *baseClass;
  virtual void addElementEntry(int rc,const char *name,const char *value) = 0;
  virtual void addAttributeEntry(const char *name,const char *value) = 0;

  int parse(const char *xmlData,size_t xmlSize){
    recursiveDepth=0;
    LIBXML_TEST_VERSION
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    doc = xmlParseMemory(xmlData,xmlSize);
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
  int parseFile(const char *xmlFile){
    recursiveDepth=0;
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

  CXMLSerializerInterface(){
    pt2Class=this;
    baseClass=this;
    currentNode = NULL;
  }
};

/**
 * parses a character string to int
 * @param pszValue The string to parse
 */
int parseInt(const char *pszValue);


/**
 * parses a character string to float
 * @param pszValue The string to parse
 */
float parseFloat(const char *pszValue);

/**
 * parses a character string to double
 * @param pszValue The string to parse
 */
double parseDouble(const char *pszValue);

#endif

