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

#include "CTypes.h"

#include "CTStringRef.h"


namespace CT{
  
  CT::stringref::~stringref(){constdata=NULL;_length=0;if(voldata!=NULL){delete[]voldata;voldata=NULL;}}

CT::stringref::stringref(){init();}

CT::stringref::stringref(const char * data,size_t length){
  init();
  assign(data,length);
}

CT::stringref::stringref(CT::stringref const& f){
  init();
  if(&f==NULL){return;}
  _length=f._length;
  constdata=f.constdata;
}

CT::stringref& CT::stringref::operator= (stringref const& f){
  if(&f==NULL){init();return *this;}
  if (this == &f) return *this;  
  init();
  _length=f._length;
  constdata=f.constdata;
  return *this;
}

void CT::stringref::assign(const char * data,size_t length){
  this->constdata=data;
  this->_length=length;
}

void CT::stringref::init(){voldata=NULL;}
  

const char *CT::stringref::c_str(){
  delete[] voldata;
  voldata = new char[_length+1];
  memcpy ( voldata, constdata, _length );
  voldata[_length]='\0';
  return voldata;
}

int CT::stringref::indexOf(char const*search){
  if(_length==0)return -1;
  if(search[0]=='\0')return -1;
  size_t i=0;
  for(size_t j=0;j<_length;j++){
    if(constdata[j]==search[i]){i++;}else{i=0;}
    if(search[i]=='\0')return j-i+1;
  }
  return -1;
}

CT::stringref CT::stringref::trim(){
  stringref r;
  r.assign(constdata,_length);
  int s=0;
  for(size_t j=0;j<_length;j++){if(constdata[j]!=' '){s=j;break;}}
  r.constdata = r.constdata+s;
  int e=_length-s;
  for(size_t j=_length-1-s;j>=0;j--){if(r.constdata[j]!=' '){e=j;break;}}
  r._length=(e+1);
  return r;
}

size_t CT::stringref::length(){
  return _length;
}

CT::StackList<CT::stringref> CT::stringref::splitToStackReferences(const char * _value){
    StackList<CT::stringref> stringList;
    stringref str(this->constdata,this->_length);
    int a = this->indexOf(_value);
    size_t rel = 0;
    while(a!=-1){
      if(a>0){
        str._length = a;
        stringList.push_back(str);
      }
      a++;
      rel+=a;
      str.constdata+=a;
      str._length=this->_length-rel;
      a = str.indexOf(_value);
    }
    if(this->_length-rel>0){
      stringList.push_back(str);
    }
    return stringList;  
}

  
  CT::StackList<CT::stringref> string::splitToStackReferences(const char * _value){
    StackList<CT::stringref> stringList;
    const char *fo = strstr(useStack?stackValue:heapValue,_value);
    const char *prevFo=useStack?stackValue:heapValue;
    while(fo!=NULL){
      stringList.push_back(CT::stringref(prevFo,(fo-prevFo)-1));
      prevFo=fo+1;
      fo = strstr(fo+1,_value);
    }
    size_t prevFoLength = strlen(prevFo);
    if(prevFoLength>0){
      stringList.push_back(CT::stringref(prevFo,prevFoLength));
    }
    return stringList;
  }
    

/*    int32::int32(){
        value=0;
        init();
    }
    int32::int32(int _value){
        init();
        value=_value;
    }*/
    
   


}
