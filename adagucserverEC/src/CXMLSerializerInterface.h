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
#define XMLE_DELOBJ(variableName){ {for(size_t j=0;j<variableName.size();j++){delete variableName[j];}}}

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
      //if(this->p!=NULL){free(this->p);}
      this->p=strdup(p);
    }
    const char *c_str(){
      return (const char*)p;
    }
    bool equals(const char *val2){
      if(p==NULL)return false;
      size_t lenval1=strlen(p);
      size_t lenval2=strlen(val2);
      if(lenval1!=lenval2)return false;
      for(size_t j=0;j<lenval1;j++)if(p[j]!=val2[j])return false;
      return true;
    }
};
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

int parseInt(const char *pszValue);
float parseFloat(const char *pszValue);

#endif

