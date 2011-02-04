#ifndef CTypes_H
#define CTypes_H
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <iostream>
#include <typeinfo>
#include <exception>
#include <stdlib.h>
#include <regex.h>
#include "CDebugger.h"



#define END NULL

namespace CT{

  template <class T>
  void CTlink(T *object,int nr){
    for(int j=0;j<nr;j++){
      object[j].next=&object[j+1];
      object[j+1].prev=&object[j];
      object[j].start=&object[0];
      object[j].end=&object[nr];
    }
    object[0].prev=NULL;object[nr].next=NULL;
    object[nr].start=&object[0];
    object[nr].end=&object[nr];
  }

  class basetype{
    public:
         virtual void init() = 0;
         virtual ~basetype(){}
      int id;
      int count;
      basetype *next,*prev,*start,*end;//Linked list
  };

class string:public basetype{
  private:
    int allocated;
    size_t privatelength; // Length of string
    size_t bufferlength;  // Length of buffer
    void _Free();
    void _Allocate(int _length);
    const char* strrstr(const char *x, const char *y);
    char _tohex(char in);
    char _fromhex(char in);
    char *value;
    void init(){value=NULL;count=0;allocated=0;privatelength=0;bufferlength=0;}
    public:
    string& operator= (string const& f){
      if (this == &f) return *this;   // Gracefully handle self assignment
      init();copy(f.value,f.privatelength);
      return *this;
    }
    string& operator= (const char*const &f){
      this->copy(f);
      return *this;
    }      
    string(){init();}
    string(const char * _value,size_t _length){init();copy(_value,_length);}
    string(const char * _value){init();copy(_value,strlen(_value));}
    string(CT::string*_string){init();copy(_string);}
    virtual ~string(){_Free();  }

    size_t length(){return privatelength;}
    size_t getbufferlength(){return bufferlength;}
    void copy(const char * _value,size_t _length);
    void copy(const CT::string*_string){
      if(_string==NULL){_Free();return;}
        copy(_string->value,_string->privatelength);
        };
        void copy(const char * _value){ if(_value==NULL){_Free();return;}copy(_value,strlen(_value));};
    void concat(const CT::string*_string){
      concat(_string->value,_string->privatelength);
    }
    
    void concat(const char*_value,size_t len);
    void concat(const char*_value){concat(_value,strlen(_value));};
    char charAt(size_t n);
    void setChar(size_t location,const char character){
      if(location<privatelength){
        value[location]=character;
      } 
    }
    int indexOf(const char* search,size_t _length);
    int match(const char *_value,size_t _length){
      if(_value==NULL)return -1;
      if(privatelength!=_length)return 1;
      if(strncmp(value,_value,_length)==0)return 0;
      return 1;
    }
    int match(const char *_value){
      if(_value==NULL)return -1;
      return match(_value,strlen(_value));
    }
    int match(CT::string* _string){
      if(_string==NULL)return -1;
      return match(_string->value,_string->privatelength);
    }
    bool equals(const char *_value,size_t _length){
      if(match(_value,_length)==0)return true;
      return false;
    }
    int equals(const char *_value){
      if(match(_value)==0)return true;
      return false;
    }
    int equals(CT::string* _string){
      if(match(_string)==0)return true;
      return false;
    }
    bool testRegEx(const char *pattern){
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
    int indexOf(const char* search){return indexOf(search,strlen(search));};
    int lastIndexOf(const char* search,size_t _length);
    int lastIndexOf(const char* search){return lastIndexOf(search,strlen(search));};
    void toUnicode();
    void toUpperCase();
    void toLowerCase();
    void decodeURL();
    void encodeURL();
    string * split(const char * _value);
    void print(const char *a, ...);
    void printconcat(const char *a, ...);
    const char * c_str();
    int substring(size_t start,size_t end){
      substring(this, start, end);
      return 0;
    }
    int replace(const char *substr,size_t substrl,const char *newString,size_t newStringl){
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
    int replace(CT::string *substr,CT::string *newString){
      return replace(substr->value,substr->privatelength,newString->value,newString->privatelength);
    }
    int replace(const char *substr,CT::string *newString){
      return replace(substr,strlen(substr),newString->value,newString->privatelength);
    }
    int replace(CT::string *substr,const char *newString){
      return replace(substr->value,substr->privatelength,newString,strlen(newString));
    }
    int replace(const char *substr,const char *newString){
      return replace(substr,strlen(substr),newString,strlen(newString));
    }

    int substring(CT::string *string, size_t start,size_t end){
      if(start<0||start>=string->privatelength||end-start<=0){
        copy("");
        return 0;
      }
      if(end>string->privatelength)end=string->privatelength;
      CT::string temp(string->value+start,end-start);
      copy(&temp);
      //printf("templength: %d\n",temp.length());
      //privatelength=end-start;
      //value[privatelength]='\0';
      
      return  0;
    }
    float toFloat(){
      float fValue=(float)atof(value);
      return fValue;  
    }
    int toInt(){
      int dValue=(int)atoi(value);
      return dValue;  
    }

  };

  class int32:public basetype{
    public:
      int value;
        void init(){count=0;};
      int32();
      int32(int _value);
  };
}

#endif
