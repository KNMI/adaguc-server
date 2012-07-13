/* 
 * Copyright (C) 2012, Royal Netherlands Meteorological Institute (KNMI)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or any 
 * later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Project    : ADAGUC
 *
 * initial programmer :  M.Plieger
 * initial date       :  20120610
 */

#include "CTime.h"
const char *CTime::className="CTime";
utUnit CTime::dataunits;
bool CTime::isInitialized;

void CTime::safestrcpy(char *s1,const char*s2,size_t size_s1){
  strncpy(s1,s2,size_s1);
  s1[size_s1]='\0';
}

CTime::CTime(){
  isInitialized = false;
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
      CDBError("UDUNITS library already initialized with %s",currentUnit.c_str());
      return 1;
    }
    return 0;
  }
  currentUnit=units;
  
  if (utInit("") != 0) {
    CDBError("Couldn't initialize Unidata units library, try setting UDUNITS_PATH to udunits.dat or try setting UDUNITS2_XML_PATH to udunits2.xml");
    return 1;
  }
  
  size_t l=strlen(units);
  char szUnits[l+1];szUnits[l]='\0';
    for(size_t j=0;j<l;j++){
      szUnits[j]=units[j];
      if(szUnits[j]=='T')szUnits[j]=32;
             if(szUnits[j]=='Z')szUnits[j]=32;
    }
    
    if(utScan(szUnits,&dataunits) != 0)  {
      CDBError("internal error: udu_fmt_time can't parse data unit string: %s",szUnits);
      return 1;
    }
    isInitialized=true;
    return 0;
}



CTime::Date CTime::getDate(double offset){
  Date date;
  date.offset=offset;
  if(utCalendar(date.offset,&dataunits,&date.year,&date.month,&date.day,&date.hour,&date.minute,&date.second)!=0) {
    CDBError("OffsetToAdaguc: Internal error: utCalendar");throw CTIME_CONVERSION_ERROR;
    
  }
  return date;
}


double CTime::dateToOffset( Date date){
  double offset;
  if(utInvCalendar(date.year,date.month,date.day,date.hour,date.minute,(int)date.second,&dataunits,&offset) != 0){
    CDBError("DateToOffset: Internal error: utInvCalendar");throw CTIME_CONVERSION_ERROR;
  }
  return offset;
}

CTime::Date CTime::stringToDate(const char*szTime){
  
  Date date;
  char szTemp[64];
  safestrcpy(szTemp,szTime,4)  ;date.year=atoi(szTemp);
  safestrcpy(szTemp,szTime+4,2);date.month=atoi(szTemp);
  safestrcpy(szTemp,szTime+6,2);date.day=atoi(szTemp);
  safestrcpy(szTemp,szTime+9,2);date.hour=atoi(szTemp);
  safestrcpy(szTemp,szTime+11,2);date.minute=atoi(szTemp);
  safestrcpy(szTemp,szTime+13,2);date.second=(float)atoi(szTemp);
  
  date.offset=dateToOffset(date);
  Date checkDate=getDate(date.offset);
  CT::string checkStr = dateToString(checkDate);
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
  s.print("%04d%02d%02dT%02d%02d%02d",date.year,date.month,date.day,date.hour,date.minute,(int)date.second);
  return s;
}


CT::string CTime::dateToISOString(Date date){
  CT::string s;
  s.print("%04d-%02d-%02dT%02d:%02d:%09f",date.year,date.month,date.day,date.hour,date.minute,(int)date.second);
  return s;
}

