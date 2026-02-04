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
#include <cmath>
#include <algorithm>

std::map<CT::string, CTime *> CTime::CTimeInstances;

CTime *CTime::GetCTimeInstance(CDF::Variable *timeVariable) {
  if (timeVariable == nullptr) {
    CDBError("CTime::init: Given timeVariable == nullptr");
    return nullptr;
  }
  CDF::Attribute *unitsAttr = timeVariable->getAttributeNE("units");
  if (unitsAttr == nullptr || unitsAttr->data == nullptr) {
    CDBError("No time units available for dimension %s", timeVariable->name.c_str());
    return nullptr;
  }
  CT::string units = unitsAttr->getDataAsString();
  if (units.length() == 0) {
    CDBError("No units data available for dimension %s", timeVariable->name.c_str());
    return NULL;
  }
  CT::string calendar;
  auto calendarAttr = timeVariable->getAttributeNE("calendar");
  if (calendarAttr != nullptr && calendarAttr->data != nullptr) {
    calendar = calendarAttr->getDataAsString();
  }
  CT::string key = units + CT::string("_") + calendar;

  std::map<CT::string, CTime *>::iterator it = CTimeInstances.find(key);
  if (it != CTimeInstances.end()) {
    return it->second;
  } else {
    auto ctime = new CTime();
    if (ctime->init(timeVariable) != 0) {
      CDBError("Unable to initialize CTime");
      return NULL;
    }
    CTimeInstances.insert(std::pair<CT::string, CTime *>(key, ctime));
    return ctime;
  }
}

CTime *CTime::GetCTimeEpochInstance() {
  CT::string timeUnits = CTIME_EPOCH_UNITS;
  std::map<CT::string, CTime *>::iterator it = CTimeInstances.find(timeUnits);
  if (it != CTimeInstances.end()) {
    return it->second;
  }
  auto ctime = new CTime();
  if (ctime->init(timeUnits.c_str(), "") != 0) {
    CDBError("Unable to initialize CTime");
    return NULL;
  }
  CTimeInstances.insert(std::pair<CT::string, CTime *>(timeUnits, ctime));
  return ctime;
}

void CTime::cleanInstances() {
  for (std::map<CT::string, CTime *>::iterator it = CTimeInstances.begin(); it != CTimeInstances.end(); ++it) {
    delete it->second;
  }
  CTime::CTimeInstances.clear();
}

int CTime::CTIME_CALENDARTYPE_360day_Months[] = {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30};
int CTime::CTIME_CALENDARTYPE_360day_MonthsCumul[] = {0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330, 360};

int CTime::CTIME_CALENDARTYPE_365day_Months[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
int CTime::CTIME_CALENDARTYPE_365day_MonthsCumul[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

void CTime::safestrcpy(char *s1, const char *s2, size_t size_s1) {
  strncpy(s1, s2, size_s1);
  s1[size_s1] = '\0';
}

CTime::CTime() {
  isInitialized = false;
  currentUnit = "";
  scanUnits = "";
  mode = CTIME_MODE_UTCALENDAR;
}
CTime::~CTime() { reset(); }

CT::string CTime::getErrorMessage(int CTimeParserException) {
  CT::string message = "Unknown error";
  if (CTimeParserException == CTIME_CONVERSION_ERROR) message = "CTIME_CONVERSION_ERROR";
  return message;
}

void CTime::reset() {
  if (isInitialized) {
    // utTerm(); // TODO: Deleting this instance will kill al other udunit instances!
  }
  currentUnit = "";
  currentCalendar = "";
  isInitialized = false;
}

int CTime::init(CDF::Variable *timeVariable) {
  if (timeVariable == NULL) {
    CDBError("CTime::init: Given timeVariable == NULL");
    return 1;
  }
  CT::string units, calendar;

  CDF::Attribute *unitsAttr = timeVariable->getAttributeNE("units");
  CDF::Attribute *calendarAttr = timeVariable->getAttributeNE("calendar");

  if (unitsAttr == NULL) {
    CDBError("No time units available for dimension %s", timeVariable->name.c_str());
    return 1;
  }

  if (unitsAttr->data == NULL) {
    CDBError("No units data available for dimension %s", timeVariable->name.c_str());
    return 1;
  }

  units = unitsAttr->getDataAsString();

  if (units.length() == 0) {
    CDBError("No units data available for dimension %s", timeVariable->name.c_str());
    return 1;
  }

  if (calendarAttr != NULL) {
    if (calendarAttr->data != NULL) {
      calendar = calendarAttr->getDataAsString();
      // CDBDebug("Found calendar %s",calendar.c_str());
    }
  }

  return init(units.c_str(), calendar.c_str());
}

int CTime::init(std::string units, std::string calendar) {
  if (isInitialized) {
    if (!currentUnit.equals(units)) {
      if (mode == CTIME_MODE_360day) {
        CDBError("CTIME_MODE_360day: already initialized with %s", currentUnit.c_str());
      }
      if (mode == CTIME_MODE_365day) {
        CDBError("CTIME_MODE_365day: lready initialized with %s", currentUnit.c_str());
      }
      if (mode == CTIME_MODE_UTCALENDAR) {
        CDBError("UDUNITS library already initialized with %s", currentUnit.c_str());
      }
      if (mode == CTIME_MODE_YYYYMM) {
        CDBError("CTIME_MODE_YYYYMM: Already initialized with %s", currentUnit.c_str());
      }
      if (mode == CTIME_MODE_YYYYMMDD_NUMBER) {
        CDBError("CTIME_MODE_YYYYMMDD_NUMBER: Already initialized with %s", currentUnit.c_str());
      }
      return 1;
    }
    return 0;
  }
  currentUnit = units;

  currentCalendar = "";

  if (!calendar.empty()) {
    currentCalendar = calendar;
  }

  bool parseTimeUnitsMySelf = false;

  if (currentCalendar.length() > 0) {
    if (currentCalendar.equals("360days") || currentCalendar.equals("360_day")) {
      // Mode is 360day
      mode = CTIME_MODE_360day;
      parseTimeUnitsMySelf = true;
      CDBDebug("360day calendar with units %s", currentUnit.c_str());
    }
    if (currentCalendar.equals("365days") || currentCalendar.equals("365_day") || currentCalendar.equals("noleap") || currentCalendar.equals("no_leap")) {
      // Mode is 365day  || noleap
      mode = CTIME_MODE_365day;
      parseTimeUnitsMySelf = true;
      CDBDebug("365day calendar with units %s", currentUnit.c_str());
    }
  }
  if (parseTimeUnitsMySelf) {
    // Eg parse "days since 1949-12-01 00:00:00"
    CT::string YYYYMMDDPart;
    CT::string HHMMSSPart;

    auto timeItems = currentUnit.split(" ");

    bool hasError = false;
    try {
      if (timeItems.size() < 3) {
        CDBError("timeItems length <3 for %s", currentUnit.c_str());
        throw(__LINE__);
      }
      if (timeItems[1].equals("since") == false) {
        CDBError("timeItems is missing since for %s", currentUnit.c_str());
        throw(__LINE__);
      }

      int unitType = -1;
      // Determine unit type (days since, hours since)
      if (timeItems[0].equals("seconds")) {
        unitType = CTIME_UNITTYPE_SECONDS;
      }
      if (timeItems[0].equals("minutes")) {
        unitType = CTIME_UNITTYPE_MINUTES;
      }
      if (timeItems[0].equals("hours")) {
        unitType = CTIME_UNITTYPE_HOURS;
      }
      if (timeItems[0].equals("days")) {
        unitType = CTIME_UNITTYPE_DAYS;
      }
      if (timeItems[0].equals("months")) {
        unitType = CTIME_UNITTYPE_MONTHS;
      }
      if (timeItems[0].equals("years")) {
        unitType = CTIME_UNITTYPE_YEARS;
      }

      if (unitType == -1) {
        CDBError("Unable to detect type of units for %s", timeItems[0].c_str());
        throw(__LINE__);
      }
      timeUnits.unitType = unitType;

      timeUnits.date.year = 0;
      timeUnits.date.month = 0;
      timeUnits.date.day = 0;
      timeUnits.date.hour = 0;
      timeUnits.date.minute = 0;
      timeUnits.date.second = 0;

      // Determince the since part, e.g. 1949-12-01 00:00:00
      YYYYMMDDPart = timeItems[2].c_str();

      auto YYYYMMDDPartSplitted = YYYYMMDDPart.split("-");

      if (YYYYMMDDPartSplitted.size() != 3) {
        CDBError("YYYYMMDD part is incorrect [%s]", YYYYMMDDPart.c_str());
        hasError = true;
      } else {
        timeUnits.date.year = YYYYMMDDPartSplitted[0].toInt();
        timeUnits.date.month = YYYYMMDDPartSplitted[1].toInt();
        timeUnits.date.day = YYYYMMDDPartSplitted[2].toInt();
      }

      if (timeItems.size() > 3) {
        HHMMSSPart = timeItems[3].c_str();
        auto HHMMSSPartSplited = HHMMSSPart.split(":");
        if (HHMMSSPartSplited.size() != 3) {
          CDBError("HHMMSS part is incorrect [%s]", HHMMSSPart.c_str());
          hasError = true;
        } else {
          timeUnits.date.hour = HHMMSSPartSplited[0].toInt();
          timeUnits.date.minute = HHMMSSPartSplited[1].toInt();
          timeUnits.date.second = (double)HHMMSSPartSplited[2].toInt();
        }
      }
    } catch (int e) {
      hasError = true;
    }

    // Check limits
    if (!(timeUnits.date.year >= 0 && timeUnits.date.year < 10000)) {
      CDBError("Year out of range");
      return 1;
    }
    if (!(timeUnits.date.month >= 1 && timeUnits.date.month < 13)) {
      CDBError("Month out of range");
      return 1;
    }
    if (!(timeUnits.date.day >= 1 && timeUnits.date.day < 32)) {
      CDBError("Day out of range");
      return 1;
    }
    if (!(timeUnits.date.hour >= 0 && timeUnits.date.hour < 24)) {
      CDBError("Hour out of range");
      return 1;
    }
    if (!(timeUnits.date.minute >= 0 && timeUnits.date.minute < 60)) {
      CDBError("Minute out of range");
      return 1;
    }
    if (!(timeUnits.date.second >= 0 && timeUnits.date.second < 60)) {
      CDBError("Second out of range");
      return 1;
    }

    // Calculate date since offset for units
    timeUnits.dateSinceOffset = 0;

    //       timeUnits.dateSinceOffset = dateToOffset(
    if (timeUnits.unitType == CTIME_UNITTYPE_DAYS) {
      if (mode == CTIME_MODE_360day) {
        timeUnits.dateSinceOffset += timeUnits.date.year * 360;
        timeUnits.dateSinceOffset += CTIME_CALENDARTYPE_360day_MonthsCumul[(timeUnits.date.month - 1)];
        timeUnits.dateSinceOffset += (timeUnits.date.day - 1);
        //         timeUnits.dateSinceOffset +=(((double)timeUnits.date.hour)/24.);  TODO
        //         timeUnits.dateSinceOffset +=(((double)timeUnits.date.minute)/(24*60.)); TODO
        //         timeUnits.dateSinceOffset +=(((double)timeUnits.date.second)/(24*60*60.));   TODO
      }
      if (mode == CTIME_MODE_365day) {
        timeUnits.dateSinceOffset += timeUnits.date.year * 365;
        timeUnits.dateSinceOffset += CTIME_CALENDARTYPE_365day_MonthsCumul[(timeUnits.date.month - 1)];
        timeUnits.dateSinceOffset += (timeUnits.date.day - 1);
        timeUnits.dateSinceOffset += (((double)timeUnits.date.hour) / 24.);
        timeUnits.dateSinceOffset += (((double)timeUnits.date.minute) / (24 * 60.));
        timeUnits.dateSinceOffset += (((double)timeUnits.date.second) / (24 * 60 * 60.));

        //         CDBDebug("timeUnits.date.month = %f",float(timeUnits.date.month));
        //
        //
        //
        //         CDBDebug("Y = %f",float(timeUnits.date.year*365));
        //         CDBDebug("m = %f",float(CTIME_CALENDARTYPE_365day_MonthsCumul[(timeUnits.date.month-1)]));
        //         CDBDebug("D = %f",float(timeUnits.date.day));
        //         CDBDebug("H = %f",(((double)timeUnits.date.hour)/24.));
        //         CDBDebug("M = %f",(((double)timeUnits.date.minute)/(24*60.)));
        //         CDBDebug("S = %f",(((double)timeUnits.date.second)/(24*60*60.)));
        //
        //
        //         CDBDebug("dateSinceOffset = %f",timeUnits.dateSinceOffset);
      }

    } else {
      CDBError("timeUnits.unitType %d not yet supported!!!", timeUnits.unitType);
      hasError = true;
    }

    if (timeUnits.dateSinceOffset == 0) {
      CDBWarning("timeUnits.dateSinceOffset == 0, probably the unit parsing has failed!");
    }

    //       CDBDebug("timeUnits.date.year   = %d",timeUnits.date.year);
    //       CDBDebug("timeUnits.date.month  = %d",timeUnits.date.month);
    //       CDBDebug("timeUnits.date.day    = %d",timeUnits.date.day);
    //       CDBDebug("timeUnits.date.hour   = %d",timeUnits.date.hour);
    //       CDBDebug("timeUnits.date.minute = %d",timeUnits.date.minute);
    //       CDBDebug("timeUnits.date.second = %f",timeUnits.date.second);
    /*
    for(int d=1;d<366;d++){
      CDBDebug("day %d = %d",d,getMonthByDayInYear(d,CTIME_CALENDARTYPE_365day_MonthsCumul));
    }

    CDBDebug("dateSinceOffset = %f",timeUnits.dateSinceOffset);

    double d = 20652;

    Date date =getDate(d);
    double o = dateToOffset(date);

    CDBDebug("2006-07-01: In = %f, out = %s and %f",d,dateToISOString(date).c_str(),o);

    for(double d=20652;d<20652+366;d++){
      Date date =getDate(d);
      double o = dateToOffset(date);

      CDBDebug("In = %f, out = %s and %f",d,dateToISOString(date).c_str(),o);
    }*/

    if (hasError) {
      return 1;
    }

    isInitialized = true;
    return 0;
  }

  // Mode is in YYYYMM format
  if (currentUnit.indexOf("YYYYMM") >= 0) {
    mode = CTIME_MODE_YYYYMM;
    isInitialized = true;
    return 0;
  }

  // Mode is in YYYYMMDD format as nunmber
  if (currentUnit.equals("day as %Y%m%d.%f")) {
    mode = CTIME_MODE_YYYYMMDD_NUMBER;
    isInitialized = true;
    return 0;
  }

  // Try udunits
  if (!utIsInit() && utInit("") != 0) {
    CDBError("Couldn't initialize Unidata units library, try setting UDUNITS_PATH to udunits.dat or try setting UDUNITS2_XML_PATH to udunits2.xml");
    return 1;
  }

  size_t l = units.length();
  char szUnits[l + 1];
  szUnits[l] = '\0';
  for (size_t j = 0; j < l; j++) {
    szUnits[j] = units[j];
    if (szUnits[j] == 'U') szUnits[j] = 32;
    if (szUnits[j] == 'T') szUnits[j] = 32;
    if (szUnits[j] == 'C') szUnits[j] = 32;
    if (szUnits[j] == 'Z') szUnits[j] = 32;
  }
  scanUnits = szUnits;

  if (utScan(scanUnits.c_str(), &dataunits) != 0) {
    CDBError("internal error: udu_fmt_time can't parse data unit string: %s", scanUnits.c_str());
    return 1;
  }
  mode = CTIME_MODE_UTCALENDAR;

  isInitialized = true;
  return 0;
}

int CTime::getMonthByDayInYear(int day, int *monthsCumul) {
  for (int j = 1; j < 13; j++) {
    if (monthsCumul[j] >= day) {
      return j;
    }
  }
  CDBError("Month not found for day %d", day);
  return -1;
}

CTime::Date CTime::getDate(double offset) {
  if (!isInitialized) {
    CDBError("getDate: not initialized");
    throw CTIME_CONVERSION_ERROR;
  }
  Date date;
  date.offset = offset;

  if (mode == CTIME_MODE_360day) {
    if (timeUnits.unitType == CTIME_UNITTYPE_DAYS) {
      double newOffset = timeUnits.dateSinceOffset + offset;
      date.year = int(newOffset / 360);
      newOffset -= (date.year * 360);

      date.month = getMonthByDayInYear(newOffset, CTIME_CALENDARTYPE_360day_MonthsCumul);
      if (date.month <= 0) {
        CDBError("date.month <= 0");
        throw CTIME_CONVERSION_ERROR;
      }
      newOffset -= (CTIME_CALENDARTYPE_360day_MonthsCumul[date.month - 1]);

      date.day = newOffset;

      newOffset -= date.day;
      ;
      if (newOffset > 0) {
        date.hour = int(newOffset * 24) % 24;
        newOffset -= float(date.hour) / 24;
        date.minute = int(newOffset * 24 * 60) % 60;
        newOffset -= float(date.minute) / (60 * 24);
        date.second = int(newOffset * 24 * 60 * 60) % 60;
        newOffset -= float(date.second) / (60 * 60 * 24);
      } else {
        date.hour = 0;
        date.minute = 0;
        date.second = 0;
      }
      date.day++;
      int numDaysInMonth = CTIME_CALENDARTYPE_360day_Months[date.month - 1];
      if (date.day > numDaysInMonth) {
        date.month++;
        date.day = 1;
        if (date.month > 12) {
          date.month = 1;
          date.year++;
        }
      }
    }
  }

  if (mode == CTIME_MODE_365day) {
    if (timeUnits.unitType == CTIME_UNITTYPE_DAYS) {
      double newOffset = timeUnits.dateSinceOffset + offset;
      //       CDBDebug("timeUnits.dateSinceOffset = %f",timeUnits.dateSinceOffset);
      //       CDBDebug("newOffset = %f",newOffset);
      //       CDBDebug("offset = %f",offset);
      date.year = int(newOffset / 365);
      newOffset -= (date.year * 365);

      date.month = getMonthByDayInYear(newOffset, CTIME_CALENDARTYPE_365day_MonthsCumul);
      if (date.month <= 0) {
        CDBError("date.month <= 0");
        throw CTIME_CONVERSION_ERROR;
      }
      newOffset -= (CTIME_CALENDARTYPE_365day_MonthsCumul[date.month - 1]);

      date.day = newOffset;

      //       CDBDebug("YMD: %f %f %f",float(date.year),float(date.month),float(date.day));

      newOffset -= date.day;
      ;
      if (newOffset > 0) {
        // CDBDebug("newOffset>0");
        date.hour = int(newOffset * 24) % 24;
        newOffset -= float(date.hour) / 24;
        date.minute = int(newOffset * 24 * 60) % 60;
        newOffset -= float(date.minute) / (60 * 24);
        date.second = int(newOffset * 24 * 60 * 60) % 60;
        newOffset -= float(date.second) / (60 * 60 * 24);
        //         CDBDebug("newOffset>0: %f %f %f",float(date.hour),float(date.minute),float(date.second));
        //
        //         CDBDebug("Remaining offset: %f (should be zero)",newOffset);
      } else {
        date.hour = 0;
        date.minute = 0;
        date.second = 0;
      }
      date.day++;
      int numDaysInMonth = CTIME_CALENDARTYPE_365day_Months[date.month - 1];
      if (date.day > numDaysInMonth) {
        date.month++;
        date.day = 1;
        if (date.month > 12) {
          date.month = 1;
          date.year++;
        }
      }
    } else {
      CDBError("Other unittype than CTIME_UNITTYPE_DAYS not supported");
      throw CTIME_CONVERSION_ERROR;
    }
  }

  if (mode == CTIME_MODE_YYYYMM) {
    int yyyymm = int(offset);
    date.year = int(yyyymm / 100);
    date.month = yyyymm - (int(yyyymm / 100) * 100);
    date.day = 1;
    date.hour = 0;
    date.minute = 0;
    date.second = 0;
  }

  if (mode == CTIME_MODE_YYYYMMDD_NUMBER) {
    int yyyymm = int(offset);
    date.year = int(yyyymm / 10000);
    date.month = int((yyyymm - (date.year * 10000)) / 100);
    date.day = yyyymm - (date.month * 100 + date.year * 10000);
    date.month++;
    date.day++;
    date.hour = 0;
    date.minute = 0;
    date.second = 0;
  }

  if (mode == CTIME_MODE_UTCALENDAR) {
    float s = 0;
    int status = utCalendar(date.offset, &dataunits, &date.year, &date.month, &date.day, &date.hour, &date.minute, &s);
    if (status != 0) {
      //       CDBError("dataunits: %d", dataunits);
      CDBError("OffsetToAdaguc: Internal error: utCalendar, status = [%d]", status);
      throw CTIME_CONVERSION_ERROR;
    }
    date.second = s;
  }
  return date;
}

double CTime::dateToOffset(Date date) {
  if (mode == CTIME_MODE_360day && timeUnits.unitType == CTIME_UNITTYPE_DAYS) {
    double offset = date.year * 360;
    offset += CTIME_CALENDARTYPE_360day_MonthsCumul[(date.month - 1)];
    offset += (date.day - 1);
    offset += (((float)date.hour) / 24.);
    offset += (((float)date.minute) / (24 * 60.));
    offset += (((float)date.second) / (24 * 60 * 60.));
    offset -= timeUnits.dateSinceOffset;
    return offset;
  }

  if (mode == CTIME_MODE_365day && timeUnits.unitType == CTIME_UNITTYPE_DAYS) {
    double offset = date.year * 365;
    offset += CTIME_CALENDARTYPE_365day_MonthsCumul[(date.month - 1)];
    offset += (date.day - 1);
    offset += (((float)date.hour) / 24.);
    offset += (((float)date.minute) / (24 * 60.));
    offset += (((float)date.second) / (24 * 60 * 60.));
    offset -= timeUnits.dateSinceOffset;
    return offset;
  }

  if (mode == CTIME_MODE_YYYYMM) {
    return int(date.year) * 100 + int(date.month);
  }

  if (mode == CTIME_MODE_YYYYMMDD_NUMBER) {
    return int(date.year) * 10000 + int(date.month - 1) * 100 + int(date.day - 1);
  }

  if (mode == CTIME_MODE_UTCALENDAR) {
    double offset = 0;
    int status = utInvCalendar(date.year, date.month, date.day, date.hour, date.minute, (int)date.second, &dataunits, &offset);
    if (status != 0) {
      if (status == UT_ENOINIT) {
        CDBError("dateToOffset: UT_ENOINIT");
      } else if (status == UT_EINVALID) {

        CDBError("dateToOffset:UT_EINVALID utInvCalendar with args '%s' and units '%s'", dateToString(date).c_str(), currentUnit.c_str());
      } else {
        CDBError("dateToOffset: Internal error: utInvCalendar with args '%s'", dateToString(date).c_str());
      }
      throw CTIME_CONVERSION_ERROR;
    }
    return offset;
  }
  CDBError("dateToOffset: Internal error: unknown mode %d with unitType %d", mode, timeUnits.unitType);
  throw CTIME_CONVERSION_ERROR;
}

CTime::Date CTime::stringToDate(const char *szTime) {
  size_t timeLength = strlen(szTime);
  if (timeLength < 15) {
    CDBError("stringToDate internal error: invalid time format: [%s]", szTime);
    throw CTIME_CONVERSION_ERROR;
  }
  Date date;
  char szTemp[64];
  safestrcpy(szTemp, szTime, 4);
  date.year = atoi(szTemp);
  safestrcpy(szTemp, szTime + 4, 2);
  date.month = atoi(szTemp);
  safestrcpy(szTemp, szTime + 6, 2);
  date.day = atoi(szTemp);
  safestrcpy(szTemp, szTime + 9, 2);
  date.hour = atoi(szTemp);
  safestrcpy(szTemp, szTime + 11, 2);
  date.minute = atoi(szTemp);
  safestrcpy(szTemp, szTime + 13, 2);
  date.second = (float)atoi(szTemp);
  if (timeLength > 18) {
    safestrcpy(szTemp, szTime + 16, 3);
    date.second += ((float)atoi(szTemp)) / 1000;
  }
  //  CDBDebug("szTime %s %s %f",szTime,szTemp,date.second);
  try {
    date.offset = dateToOffset(date);
  } catch (int e) {
    CDBError("Exception in stringToDate with input %s", szTime);
    throw e;
  }
  Date checkDate = getDate(date.offset);
  CT::string checkStr = dateToString(checkDate);
  std::string strTime = szTime;
  if (!checkStr.equals(strTime.substr(0, 15))) {
    CDBError("stringToDate internal error: intime is different from outtime:  \"%s\" != \"%s\"", szTime, checkStr.c_str());
    throw CTIME_CONVERSION_ERROR;
  }
  if (checkDate.offset != date.offset) {
    CDBError("stringToDate internal error: intime is different from outtime:  \"%f\" != \"%f\"", date.offset, checkDate.offset);
    throw CTIME_CONVERSION_ERROR;
  }
  return date;
}

CTime::Date CTime::ISOStringToDate(const char *szTime) {
  if (strlen(szTime) < 19) {

    CDBError("ISOStringToDate internal error: intime is too short: '%s'", szTime);
    throw CTIME_CONVERSION_ERROR;
  }
  Date date;
  char szTemp[64];
  safestrcpy(szTemp, szTime, 4);
  date.year = atoi(szTemp);
  safestrcpy(szTemp, szTime + 5, 2);
  date.month = atoi(szTemp);
  safestrcpy(szTemp, szTime + 8, 2);
  date.day = atoi(szTemp);
  safestrcpy(szTemp, szTime + 11, 2);
  date.hour = atoi(szTemp);
  safestrcpy(szTemp, szTime + 14, 2);
  date.minute = atoi(szTemp);
  safestrcpy(szTemp, szTime + 17, 2);
  szTemp[19] = 'Z';
  szTemp[20] = '0';
  date.second = (float)atoi(szTemp);
  try {
    date.offset = dateToOffset(date);
  } catch (int e) {
    CDBError("ISOStringToDate: input '%s'", szTime);
    throw e;
  }
  // CDBDebug("date.offset %f",date.offset);
  Date checkDate = getDate(date.offset);
  CT::string checkStr = dateToISOString(checkDate);
  // CDBDebug("checkStr %s",checkStr.c_str());
  checkStr.setChar(19, 'Z');
  checkStr.setSize(20);
  if (!checkStr.equals(szTime)) {
    CDBError("stringToDate internal error: intime is different from outtime:  \"%s\" != \"%s\"", szTime, checkStr.c_str());
    throw CTIME_CONVERSION_ERROR;
  }
  if (checkDate.offset != date.offset) {
    CDBError("stringToDate internal error: intime is different from outtime:  \"%f\" != \"%f\"", date.offset, checkDate.offset);
    throw CTIME_CONVERSION_ERROR;
  }
  return date;
}

CT::string CTime::dateToString(Date date) {
  CT::string s;
  int second = date.second;

  int minute = date.minute;
  //   if(date.second>=60.){
  //     second-=60;minute+=1;
  //   }
  s.print("%04d%02d%02dT%02d%02d%02d", date.year, date.month, date.day, date.hour, minute, second);
  return s;
}

CT::string CTime::dateToISOString(Date date) {
  CT::string s;
  float second = date.second;
  // int minute = date.minute;
  //   if(date.second>=60.){
  //     second-=60;minute+=1;
  //   }
  // s.print("%04d-%02d-%02dT%02d:%02d:%09f",date.year,date.month,date.day,date.hour,minute,second);

  int seconds = int(second);
  int milliseconds = int((second - seconds) * 1000);
  if (milliseconds != 0) {
    s.print("%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", date.year, date.month, date.day, date.hour, date.minute, seconds, milliseconds);
  } else {
    s.print("%04d-%02d-%02dT%02d:%02d:%02dZ", date.year, date.month, date.day, date.hour, date.minute, seconds);
  }

  return s;
}

CTime::Date CTime::freeDateStringToDate(const char *szTime) {
  size_t len = strlen(szTime);

  // 01234567890123
  // 20100201090000
  if (len == 14) {
    if (szTime[8] != 'T') {
      CT::string date = "";
      date.concatlength(szTime + 0, 8);
      date.concat("T");
      date.concatlength(szTime + 8, 6);
      date.concat("Z");
      CDBDebug("Fixing time to [%s]", date.c_str());
      return stringToDate(date.c_str());
    }
  }

  // 201002010900
  if (len == 12) {
    CT::string date = "";
    date.concatlength(szTime + 0, 8);
    date.concat("T");
    date.concatlength(szTime + 8, 4);
    date.concat("00Z");
    return stringToDate(date.c_str());
  }

  if (len < 14) {
    CDBError("freeDateStringToDate: datestring %s has invalid length %lu", szTime, len);
    throw CTIME_CONVERSION_ERROR;
  }
  // 2010-01-01T00:00:00.000000
  // 2010-01-01T00:00:00.000000
  // 012345678901234567890
  if (szTime[4] == '-' && szTime[7] == '-' && szTime[13] == ':' && szTime[16] == ':') {
    CT::string date = "";
    date.concatlength(szTime + 0, 4);
    date.concat("-");
    date.concatlength(szTime + 5, 2);
    date.concat("-");
    date.concatlength(szTime + 8, 2);
    date.concat("T");
    date.concatlength(szTime + 11, 2);
    date.concat(":");
    date.concatlength(szTime + 14, 2);
    date.concat(":");
    date.concatlength(szTime + 17, 2);
    date.concat("Z");
    try {
      return ISOStringToDate(date.c_str());
    } catch (int e) {
      CDBError("freeDateStringToDate exception on '%s'", szTime);
      throw e;
    }
  }

  // 20041201T00:00:00.000000
  // 012345678901234567890
  if (szTime[8] == 'T' && szTime[11] == ':' && szTime[14] == ':') {
    CT::string date = "";
    date.concatlength(szTime + 0, 8);
    date.concat("T");
    date.concatlength(szTime + 9, 2);

    date.concatlength(szTime + 12, 2);

    date.concatlength(szTime + 15, 2);
    date.concat("Z");
    return stringToDate(date.c_str());
  }

  // 20100101T000000
  // 012345678901234567890
  if (szTime[8] == 'T') {
    CT::string date = "";
    date.concatlength(szTime + 0, 8);
    date.concat("T");
    date.concatlength(szTime + 9, 6);
    date.concat("Z");
    return stringToDate(date.c_str());
  }

  // 2008-05-13T12:10Z
  // 012345678901234567890
  if (szTime[4] == '-' && szTime[7] == '-' && szTime[10] == 'T' && szTime[13] == ':' && szTime[16] == 'Z') {
    CT::string date = "";
    date.concatlength(szTime + 0, 4);
    date.concat("-");
    date.concatlength(szTime + 5, 2);
    date.concat("-");
    date.concatlength(szTime + 8, 2);
    date.concat("T");
    date.concatlength(szTime + 11, 2);
    date.concat(":");
    date.concatlength(szTime + 14, 2);
    date.concat(":");
    date.concatlength("00", 2);
    date.concat("Z");
    try {
      return ISOStringToDate(date.c_str());
    } catch (int e) {
      CDBError("freeDateStringToDate exception");
      throw e;
    }
  }

  // CDBError("Format for date string \"%s\" not recognised",szTime);

  throw CTIME_CONVERSION_ERROR;
  return CTime::Date();
}

CT::string CTime::currentDateTime() {
  timeval curTime;
  gettimeofday(&curTime, NULL);
  int milli = curTime.tv_usec / 1000;

  char buffer[80];
  strftime(buffer, 80, "%Y-%m-%dT%H:%M:%S", gmtime(&curTime.tv_sec));

  char currentTime[100] = "";
  snprintf(currentTime, 99, "%s.%03dZ", buffer, milli);

  return currentTime;
}

double CTime::quantizeTimeToISO8601(double offsetOrig, CT::string period, CT::string method) {
  double offsetLow;
  double offsetHigh;
  // P1Y
  // P1D
  // PT1M
  CTime *thisTime = this;
  Date date = thisTime->getDate(offsetOrig);

  if (period.indexOf("T") != -1) { // Contains HMS

    auto items = period.split("T");
    if (items.size() != 2) {
      throw(-1);
    }
    CT::string hmsPart = items[1];

    // hmsPart contains 15M, 1M, 12S, 6H, etc...
    if (hmsPart.indexOf("H") != -1) { // 6H
      hmsPart.replaceSelf("H", "");
      int H = hmsPart.toInt();
      int origH = date.hour;
      date.minute = 0;
      date.second = 0;
      int restH = origH % H;
      date.hour = origH - restH;
      offsetLow = thisTime->dateToOffset(date);
      if (restH > 0) {
        offsetHigh = offsetLow + H * 3600;
      } else {
        offsetHigh = offsetLow;
      }
    } else if (hmsPart.indexOf("M") != -1) { // 5M
      hmsPart.replaceSelf("M", "");
      int M = hmsPart.toInt();
      int origM = date.minute;
      date.second = 0;
      int restM = origM % M;
      date.minute = origM - restM;
      offsetLow = thisTime->dateToOffset(date);
      if (restM > 0) {
        offsetHigh = offsetLow + 60 * M;
      } else {
        offsetHigh = offsetLow;
      }
    } else if (hmsPart.indexOf("S") != -1) { // 30S
      hmsPart.replaceSelf("S", "");
      int S = hmsPart.toInt();
      int origS = date.second;
      int restS = origS % S;
      date.second = origS - restS;
      offsetLow = thisTime->dateToOffset(date);
      if (restS > 0) {
        date.second = origS + restS;
        offsetHigh = offsetLow + S;
      } else {
        offsetHigh = offsetLow;
      }
    }

  } else { // Contains YMD

    CT::string hmsPart = period;
    hmsPart.replaceSelf("P", "");

    // hmsPart contains 15M, 1M, 12S, 6H, etc...
    if (hmsPart.indexOf("Y") != -1) { // 6H
      hmsPart.replaceSelf("Y", "");
      int Y = hmsPart.toInt();
      int origY = date.year;
      date.month = 1;
      date.day = 1;
      date.hour = 0;
      date.minute = 0;
      date.second = 0;
      int restY = origY % Y;
      date.year = origY - restY;
      offsetLow = thisTime->dateToOffset(date);
      if (restY > 0) {
        date.year = date.year + Y;
      }
      offsetHigh = thisTime->dateToOffset(date);
    } else if (hmsPart.indexOf("M") != -1) { // 5M
      date.day = 1;
      date.hour = 0;
      date.minute = 0;
      date.second = 0;
      hmsPart.replaceSelf("M", "");
      int M = hmsPart.toInt();
      int origM = date.month;
      int restM = origM % M;
      date.month = origM - restM;
      offsetLow = thisTime->dateToOffset(date);
      if (restM > 0) {
        date.month = date.month + M;
      }
      offsetHigh = thisTime->dateToOffset(date);
    } else if (hmsPart.indexOf("D") != -1) { // 30S
      date.hour = 0;
      date.minute = 0;
      date.second = 0;
      hmsPart.replaceSelf("D", "");
      int D = hmsPart.toInt();
      int origD = date.day;
      int restD = origD % D;
      date.day = origD - restD;
      offsetLow = thisTime->dateToOffset(date);
      if (restD > 0) {
        date.day = date.day + D;
      }
      offsetHigh = thisTime->dateToOffset(date);
    }
  }

  if (method.equals("low")) {
    offsetOrig = offsetLow;
  } else if (method.equals("high")) {
    offsetOrig = offsetHigh;
  } else if (method.equals("round")) {
    double diffL = fabs(offsetOrig - offsetLow);
    double diffH = fabs(offsetOrig - offsetHigh);
    if (diffL < diffH) {
      offsetOrig = offsetLow;
    } else {
      offsetOrig = offsetHigh;
    }
  }
  return offsetOrig;
}

CT::string CTime::quantizeTimeToISO8601(CT::string value, CT::string period, CT::string method) {
  CT::string newDateString = value;
  // CDBDebug("quantizetime with for value %s with period %s and method %s", value.c_str(), period.c_str(), method.c_str());
  try {
    CTime time;
    time.init("seconds since 0000-01-01T00:00:00Z", "");
    double offsetOrig = time.dateToOffset(time.freeDateStringToDate(value.c_str()));
    double quantizedOffset = time.quantizeTimeToISO8601(offsetOrig, &period, &method);
    newDateString = time.dateToISOString(time.getDate(quantizedOffset));
  } catch (int e) {
    CDBError("Exception in quantizetime with message %s", CTime::getErrorMessage(e).c_str());
  }
  // CDBDebug("New date is %s", newDateString.c_str());
  return newDateString;
  // return "2016-01-13T09:50:00Z";
}

time_t CTime::getEpochTimeFromDateString(CT::string dateString) {
  struct tm result;
  std::vector<CT::string> formats;
  formats.push_back("%a %b %d %H:%M:%S %Y"); /* 'Sun Sep 22 13:23:18 2019' string  */
  formats.push_back("%Y-%m-%dT%H:%M:%S");    /* 'yyyy-mm-ddTHH:MM:SS' string  */

  for (size_t j = 0; j < formats.size(); j++) {
    CT::string format = formats[j];
    if (strptime(dateString.c_str(), format.c_str(), &result) != NULL) {
      /* Disable timezone, try to do conversion in UTC */
      time_t timeSinceEpoch = 0;
      result.tm_isdst = 0;
      timeSinceEpoch = mktime(&result) - timezone;
      return timeSinceEpoch;
    }
  }

  CDBDebug("Warning: CTime::getEpochTimeFromDateString: Unable to convert [%s] to epoch time.", dateString.c_str());
  throw(CTIME_CONVERSION_ERROR);
}

CTime::Date CTime::subtractPeriodFromDate(CTime::Date date, CT::string period) {
  CTime::Date datePeriod = periodToDate(period);
  return this->subtractPeriodFromDate(date, datePeriod);
}

CTime::Date CTime::subtractPeriodFromDate(CTime::Date date, CTime::Date datePeriod) {
  Date newDate;
  newDate.year = date.year - datePeriod.year;
  newDate.month = date.month - datePeriod.month;
  newDate.day = date.day - datePeriod.day;
  newDate.hour = date.hour - datePeriod.hour;
  newDate.minute = date.minute - datePeriod.minute;
  newDate.second = date.second - datePeriod.second;
  double offset = this->dateToOffset(newDate);
  return this->offsetToDate(offset);
}

CTime::Date CTime::addPeriodToDate(CTime::Date date, CT::string period) {
  CTime::Date datePeriod = periodToDate(period);
  return this->addPeriodToDate(date, datePeriod);
}

CTime::Date CTime::addPeriodToDate(CTime::Date date, CTime::Date datePeriod) {
  Date newDate;
  newDate.year = date.year + datePeriod.year;
  newDate.month = date.month + datePeriod.month;
  newDate.day = date.day + datePeriod.day;

  newDate.second = date.second + datePeriod.second;
  newDate.minute = date.minute + datePeriod.minute;
  newDate.hour = date.hour + datePeriod.hour;

  while (newDate.second >= 60) {
    newDate.second -= 60;
    newDate.minute++;
  }

  while (newDate.minute >= 60) {
    newDate.minute -= 60;
    newDate.hour++;
  }

  while (newDate.hour >= 24) {
    newDate.hour -= 24;
    newDate.day++;
  }

  double offset = this->dateToOffset(newDate);
  return this->offsetToDate(offset);
}

CT::string CTime::dateToPeriod(CTime::Date date) {
  CT::string period = "P";
  if (date.year > 0) period.printconcat("%dY", date.year);
  if (date.month > 0) period.printconcat("%dM", date.month);
  if (date.day > 0) period.printconcat("%dD", date.day);
  if (date.hour > 0 || date.minute > 0 || date.second > 0) {
    period.concat("T");
  }
  if (date.hour > 0) period.printconcat("%dH", date.hour);
  if (date.minute > 0) period.printconcat("%dM", date.minute);
  if (date.second > 0) period.printconcat("%0.fS", date.second);
  return period;
}

CTime::Date CTime::periodToDate(CT::string period) {
  std::vector<CT::string> p = period.split("T");
  if (p.size() < 1 || !p[0].startsWith("P")) {
    CDBError("Invalid time period %s", period.c_str());
    throw(__LINE__);
  }
  CT::string ymdPeriodPart = p[0].substring(1, -1);
  CT::string hmsPeriodPart = p.size() > 1 ? p[1] : "";

  int indexY = ymdPeriodPart.indexOf("Y");
  int indexMo = ymdPeriodPart.indexOf("M");
  int indexD = ymdPeriodPart.indexOf("D");

  int indexH = hmsPeriodPart.indexOf("H");
  int indexMi = hmsPeriodPart.indexOf("M");
  int indexS = hmsPeriodPart.indexOf("S");

  Date dateOperator;
  dateOperator.year = ymdPeriodPart.substring(0, indexY == -1 ? 0 : indexY).toInt();
  dateOperator.month = ymdPeriodPart.substring(std::max({0, indexY + 1}), indexMo == -1 ? 0 : indexMo).toInt();
  dateOperator.day = ymdPeriodPart.substring(std::max({0, indexY + 1, indexMo + 1}), indexD == -1 ? 0 : indexD).toInt();

  dateOperator.hour = hmsPeriodPart.substring(0, indexH == -1 ? 0 : indexH).toInt();
  dateOperator.minute = hmsPeriodPart.substring(std::max({0, indexH + 1}), indexMi == -1 ? 0 : indexMi).toInt();
  dateOperator.second = hmsPeriodPart.substring(std::max({0, indexH + 1, indexMi + 1}), indexS == -1 ? 0 : indexS).toInt();

  return dateOperator;
}