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
#include <sstream>

// Type to represent the relative interval between two timestamps
struct TimeInterval {
  int years;
  int months;
  int days;
  int hours;
  int minutes;
  int seconds;

  // Function to convert the interval to approximate total seconds
  long long toSeconds() const { return seconds + minutes * 60 + hours * 3600 + days * 86400 + months * 2592000 + years * 31536000; }

  // < operator based on total seconds
  bool operator<(const TimeInterval &other) const { return this->toSeconds() < other.toSeconds(); };

  // Equality operator
  bool operator==(const TimeInterval &other) const {
    return years == other.years && months == other.months && days == other.days && hours == other.hours && minutes == other.minutes && seconds == other.seconds;
  }

  std::string toString() const {
    std::ostringstream oss;
    if (years > 0) oss << years << " year(s) ";
    if (months > 0) oss << months << " month(s) ";
    if (days > 0) oss << days << " day(s) ";
    if (hours > 0) oss << hours << " hour(s) ";
    if (minutes > 0) oss << minutes << " minute(s) ";
    if (seconds > 0 || (years == 0 && months == 0 && days == 0 && hours == 0 && minutes == 0)) oss << seconds << " second(s)";

    return oss.str();
  }
};

struct TimeInterval calculateTimeInterval(const CTime::Date &start, const CTime::Date &end);
CT::string toISO8601Interval(const TimeInterval &interval);
CT::string estimateISO8601Duration(const std::vector<CT::string> &timestamps, double threshold = 0.8);
