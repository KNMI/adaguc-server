/******************************************************************************
 * 
 * Project:  CTime
 * Purpose:  Date Time functions
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


#include "CTime.h"
#include <ctime>
#include <sys/time.h>


const char *CTime::className="CTime";
// utUnit CTime::dataunits;
// bool CTime::isInitialized;
// 
// int CTime::mode ;

void CTime::safestrcpy(char *s1,const char*s2,size_t size_s1){
  strncpy(s1,s2,size_s1);
  s1[size_s1]='\0';
}

CTime::CTime(){
  isInitialized = false;
  mode = CTIME_MODE_UTCALENDAR;
}
CTime::~CTime(){
  reset();
}

CT::string CTime::getErrorMessage(int CTimeParserException){
  CT::string message = "Unknown error";
  if(CTimeParserException == CTIME_CONVERSION_ERROR)message="CTIME_CONVERSION_ERROR";
  return message;
}

void CTime::reset(){
  if(isInitialized){
    utTerm();
  }
  currentUnit="";
  isInitialized=false;
}

int CTime::init(const char *units){
  if(isInitialized){
    if(!currentUnit.equals(units)){
      if(mode == CTIME_MODE_UTCALENDAR){
        CDBError("UDUNITS library already initialized with %s",currentUnit.c_str());
      }
      if(mode == CTIME_MODE_YYYYMM){
        CDBError("CTIME_MODE_YYYYMM: Already initialized with %s",currentUnit.c_str());
      }
      return 1;
    }
    return 0;
  }
  currentUnit=units;
  
  //Mode is in YYYYMM format
  if(currentUnit.indexOf("YYYYMM")>=0){
    mode = CTIME_MODE_YYYYMM;
    isInitialized=true;
    return 0;
  }
  
  //Try udunits
  if (utInit("") != 0) {
    CDBError("Couldn't initialize Unidata units library, try setting UDUNITS_PATH to udunits.dat or try setting UDUNITS2_XML_PATH to udunits2.xml");
    return 1;
  }
  
  size_t l=strlen(units);
  char szUnits[l+1];szUnits[l]='\0';
  for(size_t j=0;j<l;j++){
    szUnits[j]=units[j];
    if(szUnits[j]=='U')szUnits[j]=32;
    if(szUnits[j]=='T')szUnits[j]=32;
    if(szUnits[j]=='C')szUnits[j]=32;
    if(szUnits[j]=='Z')szUnits[j]=32;
  }
  
  if(utScan(szUnits,&dataunits) != 0)  {
    CDBError("internal error: udu_fmt_time can't parse data unit string: %s",szUnits);
    return 1;
  }
  mode = CTIME_MODE_UTCALENDAR;
  
  isInitialized=true;
  return 0;
}



CTime::Date CTime::getDate(double offset){
  Date date;
  date.offset=offset;

  if(mode == CTIME_MODE_YYYYMM){
    int yyyymm = int(offset);
    date.year = int(yyyymm/100);
    date.month = yyyymm-(int(yyyymm/100)*100);
    date.day = 1;
    date.hour = 0;
    date.minute = 0;
    date.second = 0;
  }
  
  if(mode == CTIME_MODE_UTCALENDAR){
    float s;
    if(utCalendar(date.offset,&dataunits,&date.year,&date.month,&date.day,&date.hour,&date.minute,&s)!=0) {
      CDBError("OffsetToAdaguc: Internal error: utCalendar");throw CTIME_CONVERSION_ERROR;
    }
    date.second = s;
  }
  return date;
}


double CTime::dateToOffset( Date date){
  double offset;
  
  if(mode == CTIME_MODE_YYYYMM){
    return int(date.year)*100+int(date.month);
  }
  
  if(mode == CTIME_MODE_UTCALENDAR){
    if(utInvCalendar(date.year,date.month,date.day,date.hour,date.minute,(int)date.second,&dataunits,&offset) != 0){
      CDBError("dateToOffset: Internal error: utInvCalendar with args %s",dateToString(date).c_str());throw CTIME_CONVERSION_ERROR;
    }
  }
  return offset;
}


CTime::Date CTime::stringToDate(const char*szTime){
  size_t timeLength = strlen(szTime);
  if(timeLength<15){
     CDBError("stringToDate internal error: invalid time format:",szTime);
     throw CTIME_CONVERSION_ERROR;
  }
  Date date;
  char szTemp[64];
  safestrcpy(szTemp,szTime,4)  ;date.year=atoi(szTemp);
  safestrcpy(szTemp,szTime+4,2);date.month=atoi(szTemp);
  safestrcpy(szTemp,szTime+6,2);date.day=atoi(szTemp);
  safestrcpy(szTemp,szTime+9,2);date.hour=atoi(szTemp);
  safestrcpy(szTemp,szTime+11,2);date.minute=atoi(szTemp);
  safestrcpy(szTemp,szTime+13,2);date.second=(float)atoi(szTemp);
  if(timeLength>18){
    safestrcpy(szTemp,szTime+16,3);date.second+=((float)atoi(szTemp))/1000;
  }
//  CDBDebug("szTime %s %s %f",szTime,szTemp,date.second);
  try{
    date.offset=dateToOffset(date);
  }catch(int e){
    CDBError("Exception in stringToDate with input %s",szTime);
    throw e;
  }
  Date checkDate=getDate(date.offset);
  CT::string checkStr = dateToString(checkDate);
  if(!checkStr.equals(szTime,15)){
    CDBError("stringToDate internal error: intime is different from outtime:  \"%s\" != \"%s\"",szTime,checkStr.c_str());
    throw CTIME_CONVERSION_ERROR;
  }
  if(checkDate.offset!=date.offset){
    CDBError("stringToDate internal error: intime is different from outtime:  \"%f\" != \"%f\"",date.offset,checkDate.offset);
    throw CTIME_CONVERSION_ERROR;
  }
  return date;
  
}

CTime::Date CTime::ISOStringToDate(const char*szTime){
  Date date;
  char szTemp[64];
  safestrcpy(szTemp,szTime,4)  ;date.year=atoi(szTemp);
  safestrcpy(szTemp,szTime+5,2);date.month=atoi(szTemp);
  safestrcpy(szTemp,szTime+8,2);date.day=atoi(szTemp);
  safestrcpy(szTemp,szTime+11,2);date.hour=atoi(szTemp);
  safestrcpy(szTemp,szTime+14,2);date.minute=atoi(szTemp);
  safestrcpy(szTemp,szTime+17,2);date.second=(float)atoi(szTemp);
  try{
    date.offset=dateToOffset(date);
  }catch(int e){
    CDBError("ISOStringToDate: input %s",szTime);
    throw e;
  }
  Date checkDate=getDate(date.offset);
  CT::string checkStr = dateToISOString(checkDate);
  checkStr.setChar(19,'Z');
  checkStr.setSize(20);
  if(!checkStr.equals(szTime)){
    CDBError("stringToDate internal error: intime is different from outtime:  \"%s\" != \"%s\"",szTime,checkStr.c_str());
    throw CTIME_CONVERSION_ERROR;
  }
  if(checkDate.offset!=date.offset){
    CDBError("stringToDate internal error: intime is different from outtime:  \"%f\" != \"%f\"",date.offset,checkDate.offset);
    throw CTIME_CONVERSION_ERROR;
  }
  return date;
  
}


CT::string CTime::dateToString(Date date){
  CT::string s;
  int second=date.second;

  int minute = date.minute;
//   if(date.second>=60.){
//     second-=60;minute+=1;
//   }
  s.print("%04d%02d%02dT%02d%02d%02d",date.year,date.month,date.day,date.hour,minute,second);
  return s;
}


CT::string CTime::dateToISOString(Date date){
  CT::string s;
  float second=date.second;
  //int minute = date.minute;
//   if(date.second>=60.){
//     second-=60;minute+=1;
//   }
  //s.print("%04d-%02d-%02dT%02d:%02d:%09f",date.year,date.month,date.day,date.hour,minute,second);
  
  int seconds = int(second);
  int milliseconds = int((second-seconds)*1000)  ;
  if(milliseconds!=0){
    s.print("%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",date.year,date.month,date.day,date.hour,date.minute,seconds,milliseconds);
  }else{
    s.print("%04d-%02d-%02dT%02d:%02d:%02dZ",date.year,date.month,date.day,date.hour,date.minute,seconds);
  }

  return s;
}

CTime::Date CTime::freeDateStringToDate(const char*szTime){
  size_t len = strlen(szTime);
  //201002010900
  if(len==12){
    CT::string date="";
    date.concat(szTime+0,8);
    date.concat("T");
    date.concat(szTime+8,4);
    date.concat("00Z");
    return stringToDate(date.c_str());
  }
  
  if(len<14){
    CDBError("datestring %s has invalid length %d",szTime,len);
    throw CTIME_CONVERSION_ERROR;
  }
  //2010-01-01T00:00:00.000000
  //2010-01-01T00:00:00.000000
  //012345678901234567890
  if(szTime[4]=='-'&&szTime[7]=='-'&&szTime[13]==':'&&szTime[16]==':'){
    CT::string date="";
    date.concat(szTime+0,4);
    date.concat("-");
    date.concat(szTime+5,2);
    date.concat("-");
    date.concat(szTime+8,2);
    date.concat("T");
    date.concat(szTime+11,2);
    date.concat(":");
    date.concat(szTime+14,2);
    date.concat(":");
    date.concat(szTime+17,2);
    date.concat("Z");
    try{
      return ISOStringToDate(date.c_str());
    }catch(int e){
      CDBError("freeDateStringToDate exception");
      throw e;
    }
  }
  
  //20041201T00:00:00.000000
  //012345678901234567890
  if(szTime[8]=='T'&&szTime[11]==':'&&szTime[14]==':'){
    CT::string date="";
    date.concat(szTime+0,8);
    date.concat("T");
    date.concat(szTime+9,2);
   
    date.concat(szTime+12,2);
 
    date.concat(szTime+15,2);
    date.concat("Z");
    return stringToDate(date.c_str());
  }
  
  //20100101T000000
  //012345678901234567890
  if(szTime[8]=='T'){
    CT::string date="";
    date.concat(szTime+0,8);
    date.concat("T");
    date.concat(szTime+9,6);
    date.concat("Z");
    return stringToDate(date.c_str());
  }
  
  //2008-05-13T12:10Z
  //012345678901234567890
  if(szTime[4]=='-'&&szTime[7]=='-'&&szTime[10]=='T'&&szTime[13]==':'&&szTime[16]=='Z'){
    CT::string date="";
    date.concat(szTime+0,4);
    date.concat("-");
    date.concat(szTime+5,2);
    date.concat("-");
    date.concat(szTime+8,2);
    date.concat("T");
    date.concat(szTime+11,2);
    date.concat(":");
    date.concat(szTime+14,2);
    date.concat(":");
    date.concat("00",2);
    date.concat("Z");
    try{
      return ISOStringToDate(date.c_str());
    }catch(int e){
      CDBError("freeDateStringToDate exception");
      throw e;
    }
  }
  
  //CDBError("Format for date string \"%s\" not recognised",szTime);
  
  throw CTIME_CONVERSION_ERROR;
  return CTime::Date();
}


CT::string CTime::currentDateTime() {
    timeval curTime;
    gettimeofday(&curTime, NULL);
    int milli = curTime.tv_usec / 1000;

    char buffer [80];
    strftime(buffer, 80, "%Y-%m-%dT%H:%M:%S", gmtime(&curTime.tv_sec));

    char currentTime[84] = "";
    sprintf(currentTime, "%s:%03dZ", buffer, milli);
   

    return currentTime;
}

