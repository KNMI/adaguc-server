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

#include <stdio.h>
#include <vector>
#include <iostream>
#include <CTypes.h>
#include "CDebugger.h" 
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFHDF5IO.h"
#include "CTime.h"
DEF_ERRORMAIN();

int testCTimeInit(CDF::Variable*testVar, const char *testDate){
  CTime *testTime = CTime::GetCTimeInstance(testVar);
  if (testTime == NULL){
    CDBError("[FAILED] testTime is NULL");
    return 1;
  } else {
    CDBDebug("[OK] testTime is returned");
  }
  
  CTime::Date date;
  try {
    date = testTime->getDate(6600000000);
     CDBDebug("[OK] testTime->getDate(6600000000)");
  } catch(int e){
    CDBError("[FAILED] testTime->getDate(6600000000)");
    return 1;
  }
  CT::string dateString = testTime->dateToString(date);
  CDBDebug("dateString = [%s]", dateString.c_str());
  if (dateString.equals(testDate)){
    CDBDebug("[OK] dateToString");
  }else{
    CDBError("[FAILED] dateToString expected [%s]: [%s]", testDate,  dateString.c_str());
    return 1;
  }
  return 0;
}

int testCTime2Init (CDF::Variable*testVar, double valueToCheck) {
  CTime adagucTime;
  adagucTime.init(testVar);
  
  try {
    double startGraphTime = adagucTime.dateToOffset(adagucTime.freeDateStringToDate("2014-08-08T11:22:33Z"));
    if (startGraphTime != valueToCheck){
      CDBError("[FAILED] startGraphTime != %f, %f", valueToCheck, startGraphTime);
      return 1;
    } else {
      CDBDebug("[OK] startGraphTime == %f", valueToCheck);
    }
    adagucTime.getDate(0);
    CDBDebug("[OK] adagucTime->getDate(0) %f",startGraphTime);
  } catch(int e){
    CDBError("[FAILED] testTime->getDate(0)");
    return 1;
  }
  return 0;
}

int testCTimeEpochTimeConversion() {
  bool failed = false;
  try{
    if (CTime::getEpochTimeFromDateString("Sun Sep 22 13:23:18 2019") != 1569158598) {
      CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"Sun Sep 22 13:23:18 2019\") != 1569158598");
      failed = true;
    } else {
      CDBDebug("[OK] CTime::getEpochTimeFromDateString(\"Sun Sep 22 13:23:18 2019\") == 1569158598");
    }
  }catch(int e) {
    failed = true;
    CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"Sun Sep 22 13:23:18 2019\") throws an exception");
  }

  try{
    if (CTime::getEpochTimeFromDateString("2019-09-22T13:23:18") != 1569158598) {
      CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18\") != 1569158598");
      failed = true;
    } else {
      CDBDebug("[OK] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18\") == 1569158598");
    }
  }catch(int e) {
    failed = true;
    CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18\") throws an exception");
  }

   try{
    if (CTime::getEpochTimeFromDateString("2019-09-22T13:23:18Z") != 1569158598) {
      CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18Z\") != 1569158598");
      failed = true;
    } else {
      CDBDebug("[OK] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18Z\") == 1569158598");
    }
  }catch(int e) {
    failed = true;
    CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18Z\") throws an exception");
  }

  try{
    CTime::getEpochTimeFromDateString("SHOULDTHROWEXCEPTION");
    failed = true;
    CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"SHOULDTHROWEXCEPTION\") did not throw an exception");
  }catch(int e) {
    CDBDebug("[OK] CTime::getEpochTimeFromDateString(\"SHOULDTHROWEXCEPTION\") throws an exception");
  }
  return failed;
}

int main(int argCount,char **argVars){
  bool failed = false;
  CDBDebug("Testing CTime");
  CDF::Variable * testVarA = new CDF::Variable();testVarA->setAttributeText("units", "seconds since 1904-01-01 00:00:00.000 00:00");
  CDF::Variable * testVarB = new CDF::Variable();testVarB->setAttributeText("units", "seconds since 2004-01-01 00:00:00.000 00:00");
  

  if (testCTimeInit (testVarA, "21130221T212000")!=0) failed= true;
  if (testCTime2Init(testVarA, 3490341753.000000)!=0) failed= true;
  if (testCTimeInit (testVarB, "22130222T212000")!=0) failed= true;
  if (testCTime2Init(testVarB, 0334581753.000000)!=0) failed= true;
  if (testCTimeInit (testVarA, "21130221T212000")!=0) failed= true;
  if (testCTimeInit (testVarB, "22130222T212000")!=0) failed= true;
  
  if (testCTimeEpochTimeConversion() != 0) failed = true;

  CTime::cleanInstances();
  delete testVarA;
  delete testVarB;
  return failed;
}
