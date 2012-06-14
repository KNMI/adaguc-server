#include "CTypes.h"
/* !!! TODO Currently missing in netcdf lib 4.1.2 */
//int nc_def_var_deflate(int ncid, int varid, int shuffle, int deflate,                        int deflate_level){return 0;};
namespace CT{

  stringlist *string::splitN(const char * _value){
    stringlist *stringList = new stringlist();
    const char *fo = strstr(value,_value);
    const char *prevFo=value;
   // if(fo==NULL)return stringList;
    while(fo!=NULL){
      CT::string * val = new CT::string();stringList->push_back(val);
      val->copy(prevFo,(fo-prevFo));
     // printf("pushing1 %s %d\n",val->c_str(),stringList.size());
      prevFo=fo+1;
      fo = strstr(fo+1,_value);
    }
    if(strlen(prevFo)>0){
      CT::string * val = new CT::string();stringList->push_back(val);val->copy(prevFo);
    }
    //printf("pushing2 %s %d\n",val->c_str(),stringList.size());
    return stringList;
  }
  
  stringlistS string::splitS(const char * _value){
    stringlistS stringList;
    const char *fo = strstr(value,_value);
    const char *prevFo=value;
    // if(fo==NULL)return stringList;
    while(fo!=NULL){
      stringList.push_back(CT::string(prevFo,(fo-prevFo)));
      prevFo=fo+1;
      fo = strstr(fo+1,_value);
    }
    if(strlen(prevFo)>0){
      stringList.push_back(CT::string(prevFo));
    }
    //printf("pushing2 %s %d\n",val->c_str(),stringList.size());
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
  string * string::split(const char * _value){
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
    CTlink<string>(strings,n);
    return strings;
  };
  void string::_Free(){
    if(allocated!=0){
      delete[] value;
      value = NULL;
      privatelength=0;
      bufferlength=0;
      allocated=0;
    }
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

    void string::toLowerCase(){
        char szChar;
        for(unsigned int j=0;j<privatelength;j++){
            szChar=value[j];
            if(szChar>='A'&&szChar<='Z')value[j]+=32;
        }
    }

    void string::toUpperCase(){
        char szChar;
        for(unsigned int j=0;j<privatelength;j++){
            szChar=value[j];
            if(szChar>='a'&&szChar<='z')value[j]-=32;
        }
    }
    void string::encodeURL(){
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
    void string::decodeURL(){
      char *pszDecode=new char[privatelength*6+1];
      int p=0;
      unsigned char szChar,d1,d2;
      replace("+"," ");
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
    void string::toUnicode(){
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
      return value;
    }
    int32::int32(){
        value=0;
        init();
    }
    int32::int32(int _value){
        init();
        value=_value;
    }


}
