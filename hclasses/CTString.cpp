#include "CTString.h"
#include <iostream>
#include <algorithm>
#ifdef CTYPES_DEBUG
const char *CT::string::className = "CT::string";
#endif

namespace CT{
  PointerList<CT::string*> *string::splitToPointer(const char * _value){
    PointerList<CT::string*> *stringList = new PointerList<CT::string*>();
    const char *fo = strstr(useStack?stackValue:heapValue,_value);
    const char *prevFo=useStack?stackValue:heapValue;
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
    const char *fo = strstr(useStack?stackValue:heapValue,_value);
    const char *prevFo=useStack?stackValue:heapValue;
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

  string::string(){
    #ifdef CTYPES_DEBUG
    printf("string();\n");
    #endif
    init();
  }

  string::string(const char * _value){
    #ifdef CTYPES_DEBUG
    printf("string(const char * _value == %s)\n",_value);
    #endif      
    init();if (_value!=NULL) copy(_value,strlen(_value));
  }
  
  string::string(string const &f){
    #ifdef CTYPES_DEBUG
    printf("string(string const &f);\n");
    #endif

    // if((&f)==NULL){init();return;}
    init();copy(f.useStack?f.stackValue:f.heapValue,f.privatelength);
  }
 
  string::string(CT::string*_string){init();copy(_string);}
  
  string::string(const char * _value,size_t _length){init();copy(_value,_length);}
  
  string& string::operator= (string const& f){
    #ifdef CTYPES_DEBUG
    printf("string::operator= (string const& f);\n");
    #endif
    // if((&f)==NULL){init();return *this;}
    if (this == &f) return *this;  
    _Free();init();copy(f.useStack?f.stackValue:f.heapValue,f.privatelength);
    return *this;
  }

  string& string::operator= (const char*const &f){
    #ifdef CTYPES_DEBUG
    printf("string::operator= (const char*const &f)\n");
    #endif
    // if((&f)==NULL){init();return *this;}
    _Free();init();this->copy(f);
    return *this;
  }      

  string& string::operator+= (string const& f){
    if (this == &f) return *this;  
    concat(f.useStack?f.stackValue:f.heapValue,f.privatelength);
    return *this;
  }
  string& string::operator+= (const char*const &f){
    this->concat(f);
    return *this;
  }

  string string::operator+ (string const& f){
    CT::string n(*this);
    n.concat(f);
    return n;
  }

  string string::operator+ (const char*const &f){
    CT::string n(*this);
    n.concat(f);
    return n;
  }

  string::operator const char* () const {
    return this->c_str();
  }

  string * string::splitToArray(const char * _value){
    string str(useStack?stackValue:heapValue,privatelength);
    void *temp[8000];
    char * pch;int n=0;
    pch = strtok (str.useStack?str.stackValue:str.heapValue,_value);
    while (pch != NULL) {
      string *token=new string(pch);
      temp[n]=token;
      pch = strtok (NULL,_value);
      n++;
    }
    string *strings=new string[n+1];
    for(int j=0;j<n;j++){
      string *token=(string*)temp[j];
      strings[j].copy(token->useStack?token->stackValue:token->heapValue,token->privatelength);
      strings[j].count=n;
      delete token;
    }
    CTlink(strings,n);
    return strings;
  };

  void string::_Free(){
    if(allocated!=0){
      delete[] heapValue;
    
    }
    heapValue = NULL;
    stackValue[0]=0;
    useStack = CTYPES_USESTACK;
    privatelength=0;
    bufferlength=CTSTRINGSTACKLENGTH;
    allocated=0;
  }

  void string::_Allocate(int _length){
    _Free();
    if(_length>CTSTRINGSTACKLENGTH-1){
      useStack = false;
      
      heapValue=new char[_length+1];
    }else{
      
      useStack = CTYPES_USESTACK;
    }
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
    return (useStack?stackValue:heapValue)[n];
  }

  int string::indexOf(const char* search,size_t _length){
    if(_length==0)return -1;
    if(privatelength==0)return -1;
    if(allocated==0)return -1;
    const char* value = useStack?stackValue:heapValue;
    const char * pi = strstr (value,search);
    if(pi == NULL)return -1;
    int c=pi-value;
    if(c<0)c=-1;
    return c;
  }

  int string::lastIndexOf(const char* search,size_t _length){
    if(_length==0)return -1;
    if(privatelength==0)return -1;
    if(allocated==0)return -1;
    const char * value = useStack?stackValue:heapValue;
    const char * pi=strrstr (value,search);
    if(pi == NULL)return -1;
    int c=pi-value;
    if(c<0)c=-1;
    return c;
  }

  void string::copy(const char * _value,size_t _length){
    if(_value==NULL){_Free();return;}
    _Allocate(_length);
    privatelength=_length;
    char * value = useStack?stackValue:heapValue;
    strncpy(value,_value,privatelength);
    value[privatelength]='\0';
  }
    
  void string::concat(const char*_value,size_t len){
    if(_value==NULL)return;
    
    if(len==0)return;
    //Destination is still clean, this is just a copy.
    if(allocated==0){
      copy(_value,len);
      return;
    }
    
    //Check if the source fits in the destination buffer.
    size_t cat_len=len,total_len=privatelength+cat_len;
    if(total_len<bufferlength){
      char * value = useStack?stackValue:heapValue;
      strncpy(value+privatelength,_value,cat_len);
      value[total_len]='\0';
      privatelength=total_len;
      return; 
    }
    
    //Source buffer is to small, reallocate and copy to bigger buffer.
    bufferlength=total_len+privatelength*2;//8192*4-1;

    char *temp=new char[bufferlength+1];

    strncpy(temp,useStack?stackValue:heapValue,privatelength);
    temp[privatelength]='\0';
    strncpy(temp+privatelength,_value,len);
    temp[total_len]='\0';
    privatelength=total_len;
    if(useStack == false){
      delete[] heapValue;
    }
    heapValue=temp;
    useStack = false;
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
          char * value = useStack?stackValue:heapValue;
        for(unsigned int j=0;j<privatelength;j++){
            szChar=value[j];
            if(szChar>='A'&&szChar<='Z')value[j]+=32;
        }
    }

    void string::toUpperCaseSelf(){
        char szChar;
        char * value = useStack?stackValue:heapValue;
        for(unsigned int j=0;j<privatelength;j++){
            szChar=value[j];
            if(szChar>='a'&&szChar<='z')value[j]-=32;
        }
    }
    void string::encodeURLSelf(){
      char *pszEncode=new char[privatelength*6+1];
      int p=0;
      unsigned char szChar;
      char * value = useStack?stackValue:heapValue;
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
      char * value = useStack?stackValue:heapValue;
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
        char * value = useStack?stackValue:heapValue;
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
    
    const char* string::c_str() const{
      if(useStack == true){
        if(allocated == 0)return "";
        return stackValue;
      }else{
        if(heapValue == NULL){
          return "";
        }
        return heapValue;
      }      
    }

  int CT::string::replaceSelf(const char *substr,size_t substrl,const char *newString,size_t newStringl){
    if(this->empty())return 0;
    CT::string thisString;
    const char * value = c_str();
    thisString.copy(value,privatelength);
    const char * thisStringValue = thisString.c_str();
    std::vector<int>occurences;
    
    const char * tempVal = value;
    const char *search = substr;
    int c=0;
    size_t oc=0;
    do{
      tempVal = value+oc;
      //printf("testing '%s'\n",tempVal);
      const char * pi = strstr (tempVal,search);
      if(pi!=NULL){
        c=pi-tempVal;
      }else{
        c=-1;
      }
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
    char * newvalue = getValuePointer();
    size_t pt=0,ps=0,j=0;
    do{
      if(j<occurences.size()){
        while(ps==(unsigned)occurences[j]&&j<occurences.size()){
          for(size_t i=0;i<newStringl;i++){
            newvalue[pt++]=newString[i];
          }
          ps+=substrl;
          j++;
          if (j>=occurences.size()) break;
        }
      }
      newvalue[pt++]=thisStringValue[ps++];
    }while(pt<newSize);
    newvalue[newSize]='\0';
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
    const char *value = useStack?stackValue:heapValue;
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
    CT::string temp((string->useStack?string->stackValue:string->heapValue)+start,end-start);
    copy(&temp);
    return  0;
  }

  bool string::testRegEx(const char *pattern){
    int status; 
    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0){
      return false;
    }
    status = regexec(&re, useStack?stackValue:heapValue, (size_t) 0, NULL, 0);
    regfree(&re);
    if (status != 0) {
      return false;
    }
    return true;
  }


  void string::setChar(size_t location,const char character) {
    if(location<privatelength){
      (useStack?stackValue:heapValue)[location]=character;
      if(character=='\0')privatelength=location;
    } 
  }

  bool string::equals(const char *_value,size_t _length) const{
    if(_value==NULL)return false;
    if(allocated == 0)return false;
    if(privatelength!=_length)return false;
    if(privatelength == 0)return true;
    if(strncmp(useStack?stackValue:heapValue,_value,_length)==0)return true;
    return false;
  }

  bool string::equals(const char *_value) const{
    if(_value==NULL)return false;
    return equals(_value,strlen(_value));
  }

  bool string::equals(CT::string* _string) const{
    if(_string==NULL)return false;
    return equals(_string->useStack?_string->stackValue:_string->heapValue,_string->privatelength);
  }
      
  bool string::equals(CT::string _string) const{
    if(allocated == 0)return false;
    return equals(_string.useStack?_string.stackValue:_string.heapValue,_string.privatelength);
  }

  bool string::equalsIgnoreCase(const char *_value,size_t _length){
    if(_value==NULL)return false;
    if(allocated == 0)return false;
    if(privatelength!=_length)return false;
    if(privatelength == 0)return true;
    CT::string selfLowerCase = useStack?stackValue:heapValue;
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
    return equalsIgnoreCase(_string->useStack?_string->stackValue:_string->heapValue,_string->privatelength);
  }

  bool string::equalsIgnoreCase(CT::string _string){
    if(allocated == 0)return false;
    return equalsIgnoreCase(_string.useStack?_string.stackValue:_string.heapValue,_string.privatelength);
  }
  
  void string::copy(const CT::string*_string){
    if(_string==NULL){_Free();return;}
    copy(_string->useStack?_string->stackValue:_string->heapValue,_string->privatelength);      
  };
  
  void string::copy(const CT::string _string){
    if(_string.privatelength == 0){_Free();return;}
    copy(_string.useStack?_string.stackValue:_string.heapValue,_string.privatelength);      
  };
  
  void string::copy(const char * _value){ if(_value==NULL){_Free();return;}copy(_value,strlen(_value));};
  
  CT::string string::toLowerCase(){
    CT::string t;
    t.copy(c_str(),privatelength);
    t.toLowerCaseSelf();
    return t;
  }
  
  const bool string::empty(){
    if(privatelength == 0){
      return true;
    }
    if(useStack == false){
      if(heapValue == NULL)return true;
    }
    return false;
  }
  
  void string::setSize(int size){
    if(size<0){
      copy("",0);
      return;
    }
    if(size<int(privatelength)){
      getValuePointer()[size]='\0';
      privatelength=size;
    }
  }
  
  void string::concat(const CT::string*_string){concat(_string->useStack?_string->stackValue:_string->heapValue,_string->privatelength);}
  
  void string::concat(const CT::string _string){concat(_string.useStack?_string.stackValue:_string.heapValue,_string.privatelength);}
  
  void string::concat(const char*_value){if(_value==NULL)return;concat(_value,strlen(_value));};
  
  int string::indexOf(const char* search){return indexOf(search,strlen(search));};
  
  int string::lastIndexOf(const char* search){return lastIndexOf(search,strlen(search));};
  
  int string::endsWith(const char* search){return (lastIndexOf(search)==int(privatelength-strlen(search)));};
  
  int string::startsWith(const char* search){return (indexOf(search)==0);};
  
  string string::trim(){CT::string r;r.copy(c_str(),privatelength);r.trimSelf();return r;}
  
  int string::replaceSelf(CT::string *substr,CT::string *newString){return replaceSelf(substr->c_str(),substr->privatelength,newString->c_str(),newString->privatelength);}
  
  int string::replaceSelf(const char *substr,CT::string *newString){return replaceSelf(substr,strlen(substr),newString->c_str(),newString->privatelength);}
  
  int string::replaceSelf(CT::string *substr,const char *newString){return replaceSelf(substr->c_str(),substr->privatelength,newString,strlen(newString));}
  
  int string::replaceSelf(const char *substr,const char *newString){return replaceSelf(substr,strlen(substr),newString,strlen(newString));}
  
  CT::string string::replace(const char * old,const char *newstr){string r;r.copy(c_str(),privatelength);r.replaceSelf(old,newstr);return r;}
  
  int string::substringSelf(size_t start,size_t end){substringSelf(this, start, end);return 0;}
  
  CT::string string::substring(size_t start,size_t end){CT::string r;r.substringSelf(this,start,end);return r;}
  
  float string::toFloat(){float fValue=(float)atof(c_str());return fValue;}
  
  double string::toDouble(){double fValue=(double)atof(c_str());return fValue;}
  
  int string::toInt(){int dValue=(int)atoi(c_str());return dValue;}
  
  CT::string string::basename() {
    const char *last=rindex(this->c_str(), '/');
    CT::string fileBaseName;
    if ((last!=NULL)&&(*last)) {
      fileBaseName.copy(last+1);
    } else {
      fileBaseName.copy(this);
    }
    return fileBaseName;
  }
  
  

  
  CT::StackList<CT::stringref> string::splitToStackReferences(const char * _value){
    StackList<CT::stringref> stringList;
    const char *fo = strstr(useStack?stackValue:heapValue,_value);
    const char *prevFo=useStack?stackValue:heapValue;
    while(fo!=NULL){
      stringList.push_back(CT::stringref(prevFo,(fo-prevFo)));
      prevFo=fo+1;
      fo = strstr(fo+1,_value);
    }
    size_t prevFoLength = strlen(prevFo);
    if(prevFoLength>0){
      stringList.push_back(CT::stringref(prevFo,prevFoLength));
    }
    return stringList;
  }
  
  
  bool is_digit(const char value) { return std::isdigit(value); }
    
  bool string::isNumeric(){
    const std::string value = this->c_str();
    // printf("Test %s", value.c_str());
    return std::all_of(value.begin(), value.end(), is_digit);
  }
  
  bool string::isFloat() {
    double a = this->toDouble();
    CT::string s;s.print("%g",a);
    // printf("%s == %s", s.c_str(), this->c_str());
    return (s.equals(this));
  }
}
