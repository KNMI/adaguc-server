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
#include "CCDFDataModel.h"
#include "CCDFHDF5IO.h"
#include "utils.h"

DEF_ERRORMAIN();

int testCTimeInit(CDF::Variable *testVar, const char *testDate) {
  CTime *testTime = CTime::GetCTimeInstance(testVar);
  if (testTime == NULL) {
    CDBError("[FAILED] testTime is NULL");
    return 1;
  } else {
    CDBDebug("[OK] testTime is returned");
  }

  CTime::Date date;
  try {
    date = testTime->getDate(6600000000);
    CDBDebug("[OK] testTime->getDate(6600000000)");
  } catch (int e) {
    CDBError("[FAILED] testTime->getDate(6600000000)");
    return 1;
  }
  CT::string dateString = testTime->dateToString(date);
  CDBDebug("dateString = [%s]", dateString.c_str());
  if (dateString.equals(testDate)) {
    CDBDebug("[OK] dateToString");
  } else {
    CDBError("[FAILED] dateToString expected [%s]: [%s]", testDate, dateString.c_str());
    return 1;
  }
  return 0;
}

int testCTime2Init(CDF::Variable *testVar, double valueToCheck) {
  CTime *adagucTime = CTime::GetCTimeInstance(testVar);

  try {
    double startGraphTime = adagucTime->dateToOffset(adagucTime->freeDateStringToDate("2014-08-08T11:22:33Z"));
    if (startGraphTime != valueToCheck) {
      CDBError("[FAILED] startGraphTime != %f, %f", valueToCheck, startGraphTime);
      return 1;
    } else {
      CDBDebug("[OK] startGraphTime == %f", valueToCheck);
    }
    adagucTime->getDate(0);
    CDBDebug("[OK] adagucTime->getDate(0) %f", startGraphTime);
  } catch (int e) {
    CDBError("[FAILED] testTime->getDate(0)");
    return 1;
  }
  return 0;
}

int testCTimeEpochTimeConversion() {
  bool failed = false;
  try {
    if (CTime::getEpochTimeFromDateString("Sun Sep 22 13:23:18 2019") != 1569158598) {
      CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"Sun Sep 22 13:23:18 2019\") != 1569158598");
      failed = true;
    } else {
      CDBDebug("[OK] CTime::getEpochTimeFromDateString(\"Sun Sep 22 13:23:18 2019\") == 1569158598");
    }
  } catch (int e) {
    failed = true;
    CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"Sun Sep 22 13:23:18 2019\") throws an exception");
  }

  try {
    if (CTime::getEpochTimeFromDateString("2019-09-22T13:23:18") != 1569158598) {
      CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18\") != 1569158598");
      failed = true;
    } else {
      CDBDebug("[OK] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18\") == 1569158598");
    }
  } catch (int e) {
    failed = true;
    CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18\") throws an exception");
  }

  try {
    if (CTime::getEpochTimeFromDateString("2019-09-22T13:23:18Z") != 1569158598) {
      CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18Z\") != 1569158598");
      failed = true;
    } else {
      CDBDebug("[OK] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18Z\") == 1569158598");
    }
  } catch (int e) {
    failed = true;
    CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18Z\") throws an exception");
  }

  try {
    CTime::getEpochTimeFromDateString("SHOULDTHROWEXCEPTION");
    failed = true;
    CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"SHOULDTHROWEXCEPTION\") did not throw an exception");
  } catch (int e) {
    CDBDebug("[OK] CTime::getEpochTimeFromDateString(\"SHOULDTHROWEXCEPTION\") throws an exception");
  }

  try {
    CTime::Date date = CTime::periodToDate("P3Y4M5DT6H7M13S");
    if (!CTime::dateToPeriod(date).equals("P3Y4M5DT6H7M13S")) {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P3Y4M5DT6H7M13S\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"P3Y4M5DT6H7M13S\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P3Y4M5DT6H7M13S\") throws an exception");
    failed = true;
  }

  try {
    CTime::Date date = CTime::periodToDate("P1234Y");
    if (!CTime::dateToPeriod(date).equals("P1234Y")) {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P1234Y\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"P1234Y\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P1234Y\") throws an exception");
    failed = true;
  }

  try {
    CTime::Date date = CTime::periodToDate("P15M");
    if (!CTime::dateToPeriod(date).equals("P15M")) {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P15M\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"P15M\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P15M\") throws an exception");
    failed = true;
  }

  try {
    CTime::Date date = CTime::periodToDate("P7D");
    if (!CTime::dateToPeriod(date).equals("P7D")) {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P7D\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"P7D\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P7D\") throws an exception");
    failed = true;
  }

  try {
    CTime::Date date = CTime::periodToDate("PT12H");
    if (!CTime::dateToPeriod(date).equals("PT12H")) {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"PT12H\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"PT12H\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"PT12H\") throws an exception");
    failed = true;
  }

  try {
    CTime::Date date = CTime::periodToDate("PT1M");
    if (!CTime::dateToPeriod(date).equals("PT1M")) {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"PT1M\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"PT1M\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"PT1M\") throws an exception");
    failed = true;
  }

  try {
    CTime::Date date = CTime::periodToDate("PT30S");
    if (!CTime::dateToPeriod(date).equals("PT30S")) {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"PT30S\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"PT30S\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"PT30S\") throws an exception");
    failed = true;
  }

  CT::string f[] = {// 0                                                                                 //
                    "2019-09-22T13:23:18Z", "PT1H", "2019-09-22T12:23:18Z",
                    // 1                                                                                 //
                    "2019-09-22T13:23:18Z", "PT60M", "2019-09-22T12:23:18Z",
                    // 2                                                                                 //
                    "2019-09-22T13:23:18Z", "P1M", "2019-08-22T13:23:18Z",
                    // 3                                                                                 //
                    "2019-09-22T13:23:18Z", "P19Y08M21DT13H23M18S", "2000-01-01T00:00:00Z"};

  for (int j = 0; j < 4; j++) {
    CT::string in = f[j * 3 + 0];
    CT::string op = f[j * 3 + 1];
    CT::string out = f[j * 3 + 2];
    try {
      CTime ctime;
      ctime.init("seconds since 1970-01-01", "none");
      CTime::Date date = ctime.ISOStringToDate(in.c_str());
      if (!ctime.dateToISOString(ctime.subtractPeriodFromDate(date, op.c_str())).equals(out.c_str())) {
        CDBError("[FAILED]!ctime.dateToISOString(ctime.subtractPeriodFromDate(date, \"%s\")) returns %s and not %s", op.c_str(),
                 ctime.dateToISOString(ctime.subtractPeriodFromDate(date, op.c_str())).c_str(), out.c_str());
        failed = true;
        break;
      } else {
        CDBDebug("[OK] CTime::subtractPeriodFromDate(\"%s\",(\"%s\" ) == (\"%s\")", in.c_str(), op.c_str(), out.c_str());
      }
    } catch (int e) {
      failed = true;
      CDBError("[FAILED]  CTime::subtractPeriodFromDate(\"%s\",(\"%s\" ) throws an exception", in.c_str(), op.c_str());
      break;
    }
  }

  return failed;
}

int testHDF5Reader() {
  CDBDebug("testHDF5Reader");
  CT::string testFile = "./testdata/variable_string.h5";
  CDFReader *cdfReader = findReaderByFileName(testFile.c_str());
  CDFObject *cdfObject = new CDFObject();
  cdfObject->attachCDFReader(cdfReader);
  if (cdfReader->open(testFile.c_str()) != 0) {
    CDBError("[FAILED] testHDF5Reader was unable to open file %s", testFile.c_str());
    return 1;
  }
  CT::string dumpString = CDF::dump(cdfObject);

  // throw (dumpString.c_str());
  CT::string expectedString = cdfObject->getVariable("overview")->getAttribute("product_datetime_start")->getDataAsString();

  if (expectedString.equals("22-NOV-2021;08:00:00.000") == false) {
    CDBDebug("[FAILED] testHDF5Reader: expectedString 22-NOV-2021;08:00:00.000 is different then ");
    return 1;
  } else {
    CDBDebug("[OK] testHDF5Reader: expectedString is equal to dumpstring");
  }

  delete cdfObject;
  delete cdfReader;
  CTime::cleanInstances();

  return 0;
}

int main(int, char **) {
  bool failed = false;
  CDBDebug("Testing CTime");
  CDF::Variable *testVarA = new CDF::Variable();
  testVarA->setAttributeText("units", "seconds since 1904-01-01 00:00:00.000 00:00");
  CDF::Variable *testVarB = new CDF::Variable();
  testVarB->setAttributeText("units", "seconds since 2004-01-01 00:00:00.000 00:00");

  if (testCTimeInit(testVarA, "21130221T212000") != 0) failed = true;
  if (testCTime2Init(testVarA, 3490341753.000000) != 0) failed = true;
  if (testCTimeInit(testVarB, "22130222T212000") != 0) failed = true;
  if (testCTime2Init(testVarB, 0334581753.000000) != 0) failed = true;
  if (testCTimeInit(testVarA, "21130221T212000") != 0) failed = true;
  if (testCTimeInit(testVarB, "22130222T212000") != 0) failed = true;

  if (testCTimeEpochTimeConversion() != 0) failed = true;

  if (testHDF5Reader() != 0) failed = true;

  CTime::cleanInstances();
  delete testVarA;
  delete testVarB;
  if (failed) {
    CDBError("[FAILED] Some tests failed.");
  }
  return failed;
}
