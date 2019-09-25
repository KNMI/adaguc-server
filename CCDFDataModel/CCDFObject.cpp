/******************************************************************************
 * 
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
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

#include "CCDFObject.h"
#include "CCDFReader.h"
#include "CTime.h"

const char *CDFObject::className="CDFObject";

CDFObject::~CDFObject(){
  clear();
}

int CDFObject::attachCDFReader(void *reader){
  CDFReader *r=(CDFReader*)reader;
  r->cdfObject=this;
  this->reader=r;
  return 0;
}
void CDFObject::clear(){
  for(size_t j=0;j<dimensions.size();j++){delete dimensions[j];dimensions[j]=NULL;}
  for(size_t j=0;j<variables.size();j++){delete variables[j];variables[j]=NULL;}
}

int CDFObject::open(const char *fileName){
  //CDBDebug("Opening file %s (current =%s)",fileName,currentFile.c_str());
  if(currentFile.equals(fileName)){
    //CDBDebug("OK: Current file is already open");
    return 0;
  }
  CDFReader *r=(CDFReader*)reader;
   if(r==NULL){
    CDBError("No reader attached");return 1;
  }
  clear();
  currentFile.copy(fileName);
  return r->open(fileName);
}

int CDFObject::close(){
  if(reader==NULL){
    CDBError("No reader attached");return 1;
  }
  //CDBDebug("Closing reader");
  CDFReader *r=(CDFReader*)reader;
  return r->close();
  reader = NULL;
}


/**
 * Fill in the data variable based on the value of a global attribute 
 * 2019-09-24: for now it parses timestamps of Sun Sep 22 13:23:18 2019 to epoch time.
 */
void ncmlHandletimeValueFromGlobalAttribute (xmlNode *cur_node,CT:: string NCMLVarName, CDFObject* cdfObject){
   if (NCMLVarName.empty()) return;
   if(cur_node->properties->name!=NULL){
    xmlAttr*node=cur_node->properties;
    CT::string timeValueFromGlobalAttribute;
    CT::string attr_attribute;
    while(node!=NULL){
      CT::string nodeName = (char*)node->name;
      if (nodeName.equals("attribute")) attr_attribute = (char*)node->children->content;
      node=node->next;
    }
    if(!attr_attribute.empty()){
      try{
        CDF::Variable *variable = cdfObject->getVariable(NCMLVarName.c_str());
        CDF::Attribute *attribute = cdfObject->getAttribute(attr_attribute.c_str());
        CT::string attributeValue = attribute->getDataAsString();
        variable->allocateData(1);
        ((double *)variable->data)[0] = CTime::getEpochTimeFromDateString(attributeValue);
      }catch(...){}
    }
  }
}


void ncmlHandleAttribute (xmlNode *cur_node,CT:: string NCMLVarName, CDFObject* cdfObject){
  if(cur_node->properties->name!=NULL){
    xmlAttr*node=cur_node->properties;
    char * pszAttributeType=NULL,*pszAttributeName=NULL,*pszAttributeValue=NULL;
    CT::string timeValueFromGlobalAttribute;
    char * pszOrgName=NULL;
    while(node!=NULL){
      if(strncmp("name",(char*)node->name,4)==0)
        pszAttributeName=(char*)node->children->content;
      if(strncmp("type",(char*)node->name,4)==0)
        pszAttributeType=(char*)node->children->content;
      if(strncmp("value",(char*)node->name,5)==0)
        pszAttributeValue=(char*)node->children->content;
      if(strncmp("orgName",(char*)node->name,7)==0)
        pszOrgName=(char*)node->children->content;
      node=node->next;
    }
    if(pszAttributeName!=NULL){
      //Rename an attribute
      if(pszOrgName!=NULL){
        try{
          cdfObject->getVariable(NCMLVarName.c_str())->getAttribute(pszOrgName)->name.copy(pszAttributeName);
        }catch(...){}
      }else{
        //Add an attribute
        if(pszAttributeType!=NULL&&pszAttributeValue!=NULL){
          CDF::Variable *var = NULL;
          try{
            var = cdfObject->getVariable(NCMLVarName.c_str());
          }catch(...){
            var = new CDF::Variable();
            var->name.copy(NCMLVarName.c_str());
            cdfObject->addVariable(var);
          }
          CDFType attrType = cdfObject->ncmlTypeToCDFType(pszAttributeType);
          if(strncmp("String",pszAttributeType,6)==0){
            var->setAttribute(pszAttributeName,
                              attrType,
                              pszAttributeValue,
                              strlen(pszAttributeValue));
          }else{
            size_t attrLen=0;
            CT::string t=pszAttributeValue;
            CT::string *t2=t.splitToArray(",");
            attrLen=t2->count;
            double values[attrLen];
            for(size_t attrN=0;attrN<attrLen;attrN++){
              values[attrN]=atof(t2[attrN].c_str());
              //CDBDebug("%f",values[attrN]);
            }
            delete[] t2;
            //if(attrLen==3)exit(2);
            
            //double value=atof(pszAttributeValue);
            CDF::Attribute *attr = new CDF::Attribute();
            attr->name.copy(pszAttributeName);
            var->addAttribute(attr);
            attr->type=attrType;
            CDF::allocateData(attrType,&attr->data,attrLen);
            for(size_t attrN=0;attrN<attrLen;attrN++){
              if(attrType==CDF_BYTE)((char*)attr->data)[attrN]=(char)values[attrN];
              if(attrType==CDF_UBYTE)((unsigned char*)attr->data)[attrN]=(unsigned char)values[attrN];
              if(attrType==CDF_CHAR)((char*)attr->data)[attrN]=(char)values[attrN];
              if(attrType==CDF_SHORT)((short*)attr->data)[attrN]=(short)values[attrN];
              if(attrType==CDF_USHORT)((unsigned short*)attr->data)[attrN]=(unsigned short)values[attrN];
              if(attrType==CDF_INT)((int*)attr->data)[attrN]=(int)values[attrN];
              if(attrType==CDF_UINT)((unsigned int*)attr->data)[attrN]=(unsigned int)values[attrN];
              if(attrType==CDF_FLOAT)((float*)attr->data)[attrN]=(float)values[attrN];
              if(attrType==CDF_DOUBLE)((double*)attr->data)[attrN]=(double)values[attrN];
            }
            attr->length=attrLen;
          }
        }
      }
    }
  }
}


CT::string ncmlHandleVariable (xmlNode *cur_node, CDFObject* cdfObject) {
  CT::string NCMLVarName = "";
  if(cur_node->properties->name!=NULL){
    if(cur_node->properties->children->content!=NULL){
      xmlAttr*node=cur_node->properties;
      char * pszOrgName=NULL,*pszName=NULL,*pszType=NULL;
      CT::string shape;
      while(node!=NULL){
        if(strncmp("name",(char*)node->name,4)==0) {
          pszName=(char*)node->children->content;
          NCMLVarName = pszName;
        }
        if(strncmp("orgName",(char*)node->name,7)==0)
          pszOrgName=(char*)node->children->content;
        if(strncmp("type",(char*)node->name,4)==0)
          pszType=(char*)node->children->content;
        if(strncmp("shape",(char*)node->name,5)==0)
          shape=(char*)node->children->content;
        node=node->next;
      }
      //Rename a variable
      if(pszOrgName!=NULL&&pszName!=NULL){
        try{
          CDF::Variable *var= cdfObject->getVariable(pszOrgName);
          var->name.copy(pszName);
        }catch(...){}
      }
      if(pszName!=NULL){
        CDF::Variable *var=NULL;
        try{
          var= cdfObject->getVariable(pszName);
        }catch(...){
          if(pszOrgName==NULL){
            var = new CDF::Variable();
            var->currentType=CDF_CHAR;
            var->name.copy(pszName);
            cdfObject->addVariable(var);
          }
        }
        /* Set type */
        if(pszType!=NULL){
          var->setType(cdfObject->ncmlTypeToCDFType(pszType));
        }
        /* set shape */     
        if(!shape.empty()) {
          CT::StackList<CT::string> dims = shape.splitToStack(" ");
          var->dimensionlinks.clear();
          for(size_t d=0;d< dims.size();d++){
            try {
              CDF::Dimension *dimension = cdfObject->getDimension(dims[d].c_str());
              var->dimensionlinks.push_back(dimension);
            }catch(int e){
            }
          }
        }
      }
      /* Loop through the attributes in the variable */
      if (cur_node->type == XML_ELEMENT_NODE&&cur_node->name!=NULL){
        for (xmlNode * attributeNode = cur_node->children; attributeNode; attributeNode = attributeNode->next) {
          if (attributeNode->type == XML_ELEMENT_NODE&&attributeNode->name!=NULL){
            CT::string nodeName = (char*)attributeNode->name;
            if(strncmp("attribute",(char*)attributeNode->name,9)==0){ 
              ncmlHandleAttribute (attributeNode, NCMLVarName, cdfObject);
            }
            /* timeValueFromGlobalAttribute */
            if(nodeName.equals("timeValueFromGlobalAttribute")) {
              ncmlHandletimeValueFromGlobalAttribute (attributeNode, NCMLVarName, cdfObject);
            }
          }
        }
      }
    }
  }
  return CT::string(NCMLVarName);
}

void ncmlHandleDimension (xmlNode *cur_node, CDFObject* cdfObject) {
  if(cur_node->properties->name!=NULL){
    if(cur_node->properties->children->content!=NULL){
      xmlAttr*node=cur_node->properties;
      char * pszOrgName=NULL,*pszName=NULL, *pszLength = NULL;
      while(node!=NULL){
        if(strncmp("name",(char*)node->name,4)==0)
          pszName=(char*)node->children->content;
        if(strncmp("orgName",(char*)node->name,7)==0)
          pszOrgName=(char*)node->children->content;
        if(strncmp("length",(char*)node->name,6)==0)
          pszLength=(char*)node->children->content;
        node=node->next;
      }
      //Rename a dimension
      if(pszOrgName!=NULL&&pszName!=NULL){
        try{
          CDF::Dimension *dim= cdfObject->getDimension(pszOrgName);
          dim->name.copy(pszName);
        }catch(...){}
      }
      if(pszName!=NULL){
        CDF::Dimension *dim=NULL;
        try{
          dim= cdfObject->getDimension(pszName);
        }catch(...){
          if(pszOrgName==NULL){
            dim = new CDF::Dimension();
            dim->name.copy(pszName);
            cdfObject->addDimension(dim);
            if (pszLength!=NULL) {
              dim->setSize(atoi(pszLength));
            }
          }
        }                    
      }
    }
  }
}

void ncmlHandleRemoveElement (xmlNode *cur_node, CDFObject* cdfObject) {
  if(cur_node->properties->name!=NULL){
    xmlAttr*node=cur_node->properties;
    char * pszType=NULL,*pszName=NULL;
    while(node!=NULL){
      if(strncmp("name",(char*)node->name,4)==0)
        pszName=(char*)node->children->content;
      if(strncmp("type",(char*)node->name,4)==0)
        pszType=(char*)node->children->content;
      node=node->next;
    }
    //Check what the parentname of this attribute is:
    const char *attributeParentVarName = NULL;
    if(cur_node->parent){
      if(cur_node->parent->properties){
        xmlAttr *tempnode = cur_node->parent->properties;
        while(tempnode!=NULL){
          if(strncmp("name",(char*)tempnode->name,4)==0){
            attributeParentVarName=(char*)tempnode->children->content;
            break;
          }
          tempnode=tempnode->next;
        }
      }
    }
    if(pszType!=NULL&&pszName!=NULL){
      if(strncmp(pszType,"variable",8)==0){
        cdfObject->removeVariable(pszName);
      }
      //Check wether we want to remove an attribute
      if(strncmp(pszType,"attribute",9)==0){
        if(attributeParentVarName!=NULL){
          try{
            CDF::Variable *var= cdfObject->getVariable(attributeParentVarName);
            var->removeAttribute(pszName);
          }catch(...){}
        }else {
          //Remove a global attribute
          cdfObject->removeAttribute(pszName);
        }
      }
    }
  }
}

void recurNodes (xmlNode * a_node, CDFObject * cdfObject) {
  for (xmlNode * cur_node = a_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE&&cur_node->name!=NULL){
        /* Find variable elements */
        if(strncmp("variable",(char*)cur_node->name,8)==0) ncmlHandleVariable (cur_node, cdfObject);
        /* Dimension elements */
        if(strncmp("dimension",(char*)cur_node->name,8)==0) ncmlHandleDimension(cur_node, cdfObject);
        /* Remove elements */
        if(strncmp("remove",(char*)cur_node->name,6)==0) ncmlHandleRemoveElement (cur_node, cdfObject);
        /* Attribute elements */
        if(strncmp("attribute",(char*)cur_node->name,9)==0) ncmlHandleAttribute (cur_node, "NC_GLOBAL", cdfObject);
    }
  }
}
void CDFObject::putNCMLAttributes(void* _a_node){
  xmlNode * a_node = (xmlNode*)_a_node;
  for (xmlNode * cur_node = a_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE&&cur_node->name!=NULL){
        /* Find the 'netcdf' element in the XML file ' */
        if(strncmp("netcdf",(char*)cur_node->name,6)==0) {
          recurNodes(cur_node->children, this); 
        }
    }
  }
  
}


int CDFObject::applyNCMLFile(const char * ncmlFileName){
  //The following ncml features have been implemented:
  // add a variable with <variable name=... type=.../>
  // add a attribute with <attribute name=... type=.../>
  // remove a variable with <remove name="..." type="variable"/>
  // remove a attribute with <remove name="..." type="attribute"/>
  // rename a variable with <variable name="LavaFlow" orgName="TDCSO2" />
  // rename a dimension with <dimension name="time" orgName="TIME" />
  // rename a attribute with <attribute name="LavaFlow" orgName="TDCSO2" />
  int errorRaised=0;
  // Read the XML file and put the attributes into the data model
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;
  LIBXML_TEST_VERSION;
  doc = xmlReadFile(ncmlFileName, NULL, 0);
  if (doc == NULL) {
    CDBError("Could not parse file \"%s\"", ncmlFileName);
    return 1;
  }
  root_element = xmlDocGetRootElement(doc);
  putNCMLAttributes(root_element);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  if(errorRaised==1)return 1;
  return 0;
}

CDFType CDFObject::ncmlTypeToCDFType(const char *type){
  if(strncmp("String",type,6)==0)return CDF_CHAR;
  if(strncmp("byte",type,4)==0)return CDF_BYTE;
  if(strncmp("char",type,4)==0)return CDF_CHAR;
  if(strncmp("short",type,5)==0)return CDF_SHORT;
  if(strncmp("int",type,3)==0)return CDF_INT;
  if(strncmp("float",type,5)==0)return CDF_FLOAT;
  if(strncmp("double",type,6)==0)return CDF_DOUBLE;
  return CDF_DOUBLE;
}