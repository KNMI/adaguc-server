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

#include "CXMLSerializerInterface.h"
const char * CXMLSerializerInterface::className="CXMLSerializerInterface";
void CXMLObjectInterface::addElement(CXMLObjectInterface *baseClass,int rc, const char *name,const char *value){
  CXMLSerializerInterface * base = (CXMLSerializerInterface*)baseClass;
  base->currentNode=(CXMLObjectInterface*)this;
  if(rc==0)if(value!=NULL)this->value.copy(value);
}

int parseInt(const char *pszValue){
  if(pszValue==NULL)return 0;
  int dValue=atoi(pszValue);
  return dValue;  
}

float parseFloat(const char *pszValue){
  if(pszValue==NULL)return 0;
  float fValue=(float)atof(pszValue);
  return fValue;  
}

double parseDouble(const char *pszValue){
  if(pszValue==NULL)return 0;
  double fValue=(double)atof(pszValue);
  return fValue;  
}

bool parseBool(const char *pszValue){
  if(pszValue==NULL)return false;
  return CT::string(pszValue).equalsIgnoreCase("true");
}

