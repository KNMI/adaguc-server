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
#include "CTString.h"
#include "CTStringRef.h"
const char *CT::string::className = "CT::string";

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
    const char *fo = strstr(value,_value);
    const char *prevFo=value;
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
    

  PointerList<CT::string*> *string::splitToPointer(const char * _value){
    PointerList<CT::string*> *stringList = new PointerList<CT::string*>();
    const char *fo = strstr(value,_value);
    const char *prevFo=value;
    while(fo!=NULL){
      CT::string * val = new CT::string();stringList->push_back(val);
      val->copy(prevFo,(fo-prevFo));
      prevFo=fo+1;
      fo = strstr(fo+1,_value);
    }
    if(strlen(prevFo)>0){
      CT::string * val = new CT::string();stringList->push_back(val);val->copy(prevFo);
    }
    return stringList;
  }
  
  StackList<CT::string>  string::splitToStack(const char * _value){
    StackList<CT::string> stringList;
    const char *fo = strstr(value,_value);
    const char *prevFo=value;
    while(fo!=NULL){
      stringList.push_back(CT::string(prevFo,(fo-prevFo)));
      prevFo=fo+1;
      fo = strstr(fo+1,_value);
    }
    if(strlen(prevFo)>0){
      stringList.push_back(CT::string(prevFo));
    }
    return stringList;
  }
  
  
  
  
  
 /*void string::splitN( stringlist stringList,const char * _value){
    stringList.free();
    const char *fo = strstr(value,_value);
    const char *prevFo=value;
    if(fo==NULL)return;
    while(fo!=NULL){
      CT::string * val = new CT::string();stringList.push_back(val);
      val->copy(prevFo,(fo-prevFo));
     // printf("pushing1 %s %d\n",val->c_str(),stringList.size());
      prevFo=fo+1;
      fo = strstr(fo+1,_value);
    }
    CT::string * val = new CT::string();stringList.push_back(val);val->copy(prevFo);
    //printf("pushing2 %s %d\n",val->c_str(),stringList.size());
    //return stringList;
  }*/
 
  string::string(){
    #ifdef CTYPES_DEBUG
    printf("string();\n");
    #endif
    init();
  }
 
  string::string(string const &f){
    #ifdef CTYPES_DEBUG
    printf("string(string const &f);\n");
    #endif

    if(&f==NULL){init();return;}
    init();copy(f.value,f.privatelength);
  }
  
  string& string::operator= (string const& f){
    #ifdef CTYPES_DEBUG
    printf("string::operator= (string const& f);\n");
    #endif
    if(&f==NULL){init();return *this;}
    if (this == &f) return *this;  
    _Free();init();copy(f.value,f.privatelength);
    return *this;
  }
  
  string& string::operator= (const char*const &f){
    #ifdef CTYPES_DEBUG
    printf("string::operator= (const char*const &f)\n");
    #endif
    if(&f==NULL){init();return *this;}
    _Free();init();this->copy(f);
    return *this;
  }      
  
  string& string::operator+= (string const& f){
    if (this == &f) return *this;  
    concat(f.value,f.privatelength);
    return *this;
  }
  string& string::operator+= (const char*const &f){
    this->concat(f);
    return *this;
  }
  
  string& string::operator+ (string const& f){
    if (this == &f) return *this;  
    concat(f.value,f.privatelength);
    return *this;
  }
  
  string& string::operator+ (const char*const &f){
    this->concat(f);return *this;
    
  }
  
 
  string * string::splitToArray(const char * _value){
    string str(value,privatelength);
    void *temp[8000];
    char * pch;int n=0;
    pch = strtok (str.value,_value);
    while (pch != NULL) {
      string *token=new string(pch);
      temp[n]=token;
      pch = strtok (NULL,_value);
      n++;
    }
    string *strings=new string[n+1];
    for(int j=0;j<n;j++){
      string *token=(string*)temp[j];
      strings[j].copy(token->value,token->privatelength);
      strings[j].count=n;
      delete token;
    }
    CTlink(strings,n);
    return strings;
  };
  
  void string::_Free(){
    if(allocated!=0){
      delete[] value;
   
    }
    value = NULL;
    privatelength=0;
    bufferlength=0;
    allocated=0;
  }
  void string::_Allocate(int _length){
    _Free();
    value=new char[_length+1];
    allocated=1;
  }
  const char *string::strrstr(const char *x, const char *y) {
    const char *prev = NULL;
    const char *next;
    if (*y == '\0')return strchr(x, '\0');
    while ((next = strstr(x, y)) != NULL) {
      prev = next;
      x = next + 1;
    }
    return prev;
  }

  char string::charAt(size_t n){
    if(n<0||n>privatelength)return 0;
    return value[n];
  }
  int string::indexOf(const char* search,size_t _length){
    if(_length==0)return -1;
    if(privatelength==0)return -1;
    int c=strstr (value,search)-value;
    if(c<0)c=-1;
    return c;
  }

  int string::lastIndexOf(const char* search,size_t _length){
    if(_length==0)return -1;
    if(privatelength==0)return -1;
    int c=strrstr (value,search)-value;
    if(c<0)c=-1;
    return c;
  }
  void string::copy(const char * _value,size_t _length){
    if(_value==NULL){_Free();return;}
        _Allocate(_length);
        privatelength=_length;
        strncpy(value,_value,privatelength);
        value[privatelength]='\0';
      }

  /*void string::concat(const char*_value,size_t len){
    int cat_len=len,total_len=privatelength+cat_len;
    if(len==0)return;
    char *temp=new char[total_len+1];
    strncpy(temp,value,privatelength);
    temp[privatelength]='\0';
    strncat(temp,_value,total_len);
    temp[total_len]='\0';
    copy(temp,total_len);
    delete[] temp;
}*/
   
  void string::concat(const char*_value,size_t len){
    if(_value==NULL)return;
    size_t cat_len=len,total_len=privatelength+cat_len;
    if(len==0)return;
    if(total_len<bufferlength){
      strncpy(value+privatelength,_value,cat_len);
      value[total_len]='\0';
      privatelength=total_len;
      return; 
    }
    if(allocated==0){
      copy(_value,len);
      return;
    }
    bufferlength=total_len+privatelength*2;//8192*4-1;
    char *temp=new char[bufferlength+1];

    strncpy(temp,value,privatelength);
    temp[privatelength]='\0';
    strncpy(temp+privatelength,_value,len);
    temp[total_len]='\0';
    privatelength=total_len;
    char *todelete = value;
    value=temp;
    delete[] todelete;
  }
    char string::_tohex(char in){
        in+=48;
        if(in>57)in+=7;
        return in;
    }
    char string::_fromhex(char in){
      //From lowercase to uppercase
      if(in>96)in-=32;
      //From number character to numeric value
      in-=48;
      //When numeric value is more than 16 (eg ABCDEF) substract 7 to get numeric value 10,11,12,etc...
      if(in>16)in-=7;
      return in;
    }

    void string::toLowerCaseSelf(){
        char szChar;
        for(unsigned int j=0;j<privatelength;j++){
            szChar=value[j];
            if(szChar>='A'&&szChar<='Z')value[j]+=32;
        }
    }

    void string::toUpperCaseSelf(){
        char szChar;
        for(unsigned int j=0;j<privatelength;j++){
            szChar=value[j];
            if(szChar>='a'&&szChar<='z')value[j]-=32;
        }
    }
    void string::encodeURLSelf(){
      char *pszEncode=new char[privatelength*6+1];
      int p=0;
      unsigned char szChar;
      for(unsigned int j=0;j<privatelength;j++){
        szChar=value[j];
        if(szChar<48||(szChar>59&&szChar<63)){
          pszEncode[p++]='%';
          pszEncode[p++]=_tohex(szChar/16);
          pszEncode[p++]=_tohex(szChar%16);
        }else{
          pszEncode[p++]=szChar;
        }
      }
      copy(pszEncode,p);
      delete [] pszEncode;
    }
    void string::decodeURLSelf(){
      char *pszDecode=new char[privatelength*6+1];
      int p=0;
      unsigned char szChar,d1,d2;
      replaceSelf("+"," ");
      for(unsigned int j=0;j<privatelength;j++){
        szChar=value[j];
        if(szChar=='%'){
          d1=_fromhex(value[j+1]);
          d2=_fromhex(value[j+2]);
          pszDecode[p++]=d1*16+d2;
          j=j+2;
        }else{
          pszDecode[p++]=szChar;
        }
      }
      copy(pszDecode,p);
      
      delete [] pszDecode;
    }
    void string::toUnicodeSelf(){
        char *pszUnicode=new char[privatelength*6+1];

        int p=0;
        unsigned char szChar;
        for(unsigned int j=0;j<privatelength;j++){
            szChar=value[j];
            if(szChar>127){
                pszUnicode[p++]='\\';
                pszUnicode[p++]='u';
                pszUnicode[p++]='0';
                pszUnicode[p++]='0';
                pszUnicode[p++]=_tohex(szChar/16);
                pszUnicode[p++]=_tohex(szChar%16);
            }else{
                pszUnicode[p++]=szChar;
            }
        }
        copy(pszUnicode,p);
        delete[] pszUnicode;
    }
    void string::print(const char *a, ...){
      va_list ap;
      char szTemp[8192+1];
      va_start (ap, a);
      vsnprintf (szTemp, 8192, a, ap);
      va_end (ap);
      szTemp[8192]='\0';
      copy(szTemp);
    }
    void string::printconcat(const char *a, ...){
      va_list ap;
      char szTemp[8192+1];
      va_start (ap, a);
      vsnprintf (szTemp, 8192, a, ap);
      va_end (ap);
      szTemp[8192]='\0';
      concat(szTemp);
    }
    const char* string::c_str(){
      if(value == NULL){
        return "";
      }
      return value;
    }
    
    int CT::string::replaceSelf(const char *substr,size_t substrl,const char *newString,size_t newStringl){
      if(this->privatelength==0)return 0;
      if(this->value==NULL)return 0;
      CT::string thisString;
      thisString.copy(value,privatelength);
      std::vector<int>occurences;
      char * tempVal = value;
      const char *search = substr;
      int c=0;
      size_t oc=0;
      do{
        tempVal = value+oc;
        //printf("testing '%s'\n",tempVal);
        c=strstr (tempVal,search)-tempVal;
        if(c>=0){
          oc+=c;
          //printf("!%d\n",oc);
          occurences.push_back(oc);
          oc+=substrl;
        }
      }while(c>=0&&oc<thisString.privatelength);
      //for(size_t j=0;j<occurences.size();j++){
        //        printf("%d\n",occurences[j]);
      //}
      size_t newSize = privatelength+occurences.size()*(newStringl-substrl);
      _Allocate(newSize);
      size_t pt=0,ps=0,j=0;
      do{
        if(j<occurences.size()){
          while(ps==(unsigned)occurences[j]&&j<occurences.size()){
            for(size_t i=0;i<newStringl;i++){
              value[pt++]=newString[i];
            }
            ps+=substrl;
            j++;
            if (j>=occurences.size()) break;
          }
        }
        value[pt++]=thisString.value[ps++];
      }while(pt<newSize);
      value[newSize]='\0';
      //privatelength
      privatelength=newSize;
      //printf("newSize %d\n",privatelength);
      return 0;      
    }
    
       
    CT::string string::encodeXML(CT::string stringToEncode){
      stringToEncode.encodeXMLSelf();
      return stringToEncode;
    }
    
    CT::string string::encodeXML(){
      CT::string str = this->c_str();
      str.encodeXMLSelf();
      return str;
    }

    void string::encodeXMLSelf(){
      replaceSelf("&amp;","&");
      replaceSelf("&","&amp;");
      
      replaceSelf("&lt;","<");
      replaceSelf("<","&lt;");
      
      replaceSelf("&gt;","<");
      replaceSelf("<","&gt;");
    }

    
    void string::trimSelf(){
      int s=-1,e=privatelength;
      for(size_t j=0;j<privatelength;j++){if(value[j]!=' '){s=j;break;}}
      for(size_t j=privatelength-1;j>=0;j--){if(value[j]!=' '){e=j;break;}}
      substringSelf(s,e+1);
    }
    
    int string::substringSelf(CT::string *string, size_t start,size_t end){
      if(start<0||start>=string->privatelength||end-start<=0){
        copy("");
        return 0;
      }
      if(end>string->privatelength)end=string->privatelength;
      CT::string temp(string->value+start,end-start);
      copy(&temp);
      return  0;
    }
    
    bool string::testRegEx(const char *pattern){
      int status; 
      regex_t re;
      if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0){
        return false;
      }
      status = regexec(&re, value, (size_t) 0, NULL, 0);
      regfree(&re);
      if (status != 0) {
        return false;
      }
      return true;
    }
    
    
    void string::setChar(size_t location,const char character){
      if(location<privatelength){
        value[location]=character;
        if(character=='\0')privatelength=location;
      } 
    }
    
    
    
    bool string::equals(const char *_value,size_t _length){
      if(_value==NULL)return false;
      if(value==NULL)return false;
      if(privatelength!=_length)return false;
      if(privatelength == 0)return true;
      if(strncmp(value,_value,_length)==0)return true;
      return false;
    }
    
    bool string::equals(const char *_value){
      if(_value==NULL)return false;
      return equals(_value,strlen(_value));
    }
    
    bool string::equals(CT::string* _string){
      if(_string==NULL)return false;
      return equals(_string->value,_string->privatelength);
    }
        
    bool string::equals(CT::string _string){
      if(_string.value==NULL)return false;
      return equals(_string.value,_string.privatelength);
    }
    
    bool string::equalsIgnoreCase(const char *_value,size_t _length){
      if(_value==NULL)return false;
      if(value==NULL)return false;
      if(privatelength!=_length)return false;
      if(privatelength == 0)return true;
      CT::string selfLowerCase = value;
      CT::string testValueLowerCase = _value;
      selfLowerCase.toLowerCaseSelf();
      testValueLowerCase.toLowerCaseSelf();
      if(strncmp(selfLowerCase.c_str(),testValueLowerCase.c_str(),testValueLowerCase.length())==0)return true;
      return false;
    }
    
    bool string::equalsIgnoreCase(const char *_value){
      if(_value==NULL)return false;
      return equalsIgnoreCase(_value,strlen(_value));
    }
    
    bool string::equalsIgnoreCase(CT::string* _string){
      if(_string==NULL)return false;
      return equalsIgnoreCase(_string->value,_string->privatelength);
    }
    
    bool string::equalsIgnoreCase(CT::string _string){
      if(_string.value==NULL)return false;
      return equalsIgnoreCase(_string.value,_string.privatelength);
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
