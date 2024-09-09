/******************************************************************************
 *
 * Copyright 2024, Royal Netherlands Meteorological Institute (KNMI)
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

#include "CTString.h"
#include "CTime.h"
#include <ctime>

// Type to represent the relative interval between two timestamps
struct TimeInterval {
  int years;
  int months;
  int days;
  int hours;
  int minutes;
  int seconds;
};

TimeInterval calculateTimeInterval(const CTime::Date &start, const CTime::Date &end);
CT::string toISO8601Interval(const TimeInterval &interval);
long long toSeconds(const TimeInterval &interval);
CT::string estimateISO8601Duration(const std::vector<CT::string> &timestamps, double threshold = 0.8);
