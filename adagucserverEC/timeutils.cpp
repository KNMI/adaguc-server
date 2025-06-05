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

#include <map>
#include "timeutils.h"

TimeInterval calculateTimeInterval(const CTime::Date &start, const CTime::Date &end) {
  TimeInterval interval = {0, 0, 0, 0, 0, 0};

  interval.seconds = int(end.second - start.second);
  interval.minutes = end.minute - start.minute;
  interval.hours = end.hour - start.hour;
  interval.days = end.day - start.day;
  interval.months = end.month - start.month;
  interval.years = end.year - start.year;

  // Adjust number of minutes if there are negative seconds,
  // meaning there were less than 60 seconds between timestamps.
  if (interval.seconds < 0) {
    interval.seconds += 60.0;
    interval.minutes--;
  }
  // Adjust number of hours if there are negative minutes,
  // meaning there were less than 60 minutes between timestamps.
  if (interval.minutes < 0) {
    interval.minutes += 60;
    interval.hours--;
  }
  // Adjust number of days if there are negative hours,
  // meaning there were less than 24 hours between timestamps.
  if (interval.hours < 0) {
    interval.hours += 24;
    interval.days--;
  }
  // Adjust number of days and months using month length
  // and leap year calculations, also adjust in case of
  // negative days.
  if (interval.days < 0) {
    // Take month length into consideration (month 1 being January and 12 December)
    static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int previousMonth = (end.month == 1) ? 12 : end.month - 1;
    int year = (end.month == 1) ? end.year - 1 : end.year;

    // Adjust for leap years, using the following rules:
    // - In general, a year is a leap year if divisible by 4
    // - In general, end-of-century years (divisible by 100) are NOT
    // - But if divisible by 400, it IS a leap year
    int daysPrevMonth = daysInMonth[previousMonth - 1];
    if (previousMonth == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
      daysPrevMonth = 29;
    }
    interval.days += daysPrevMonth;
    interval.months--;
  }
  if (interval.months < 0) {
    interval.months += 12;
    interval.years--;
  }

  return interval;
}

CT::string toISO8601Interval(const TimeInterval &interval) {
  // Start with the period 'P'
  CT::string result("P");

  // Append years, months, or days if present
  if (interval.years > 0) {
    result.printconcat("%dY", interval.years);
  }
  if (interval.months > 0) {
    result.printconcat("%dM", interval.months);
  }
  if (interval.days > 0) {
    result.printconcat("%dD", interval.days);
  }

  // Add hour part T if there are hours, minutes, or seconds
  if (interval.hours > 0 || interval.minutes > 0 || interval.seconds > 0) {
    result += "T";

    if (interval.hours > 0) {
      result.printconcat("%dH", interval.hours);
    }
    if (interval.minutes > 0) {
      result.printconcat("%dM", interval.minutes);
    }
    if (interval.seconds > 0) {
      result.printconcat("%dS", interval.seconds);
    }
  }
  return result;
}

// Heuristic estimation of duration in ISO8601 format, given an array of timestamps
// Checks that a consistent interval is (generally found), with room for inaccuracies (holes in data)
// controlled by the threshold argument.
CT::string estimateISO8601Duration(const std::vector<CT::string> &timestamps, double threshold) {
  // Size considered too small to find a pattern
  if (timestamps.size() < 4) return CT::string("");

  std::vector<TimeInterval> intervals;

  CTime *ctime = CTime::GetCTimeEpochInstance();

  // Parse all timestamps into tm structs
  std::vector<CTime::Date> parsedTimes;
  for (const auto &timestamp : timestamps) {
    parsedTimes.push_back(ctime->ISOStringToDate(timestamp.c_str()));
  }

  // Calculate intervals between consecutive timestamps
  for (size_t i = 1; i < parsedTimes.size(); ++i) {
    intervals.push_back(calculateTimeInterval(parsedTimes[i - 1], parsedTimes[i]));
  }

  TimeInterval smallestInterval = *std::min_element(intervals.begin(), intervals.end());

  // Count occurrences of each interval in terms of total seconds
  std::map<TimeInterval, int> intervalFrequency;
  for (const auto &interval : intervals) {
    intervalFrequency[interval]++;
  }

  // Sort each pair by number of occurrences
  const std::pair<TimeInterval, int> &mostFrequentIntervalInfo =
      *std::max_element(intervalFrequency.begin(), intervalFrequency.end(), [](const std::pair<TimeInterval, int> &a, const std::pair<TimeInterval, int> &b) { return a.second < b.second; });

  TimeInterval mostFrequentInterval = mostFrequentIntervalInfo.first;

  // If the smallest interval is not the most frequent, we do not have a regular interval
  if (!(smallestInterval == mostFrequentInterval)) return "";

  // Otherwise, it is possible we have some missing data. Check if frequency is over threshold
  if ((double(mostFrequentIntervalInfo.second) / double(timestamps.size() - 1)) >= threshold) {
    // We return the interval corresponding to this estimation
    return toISO8601Interval(mostFrequentInterval);
  }

  // No estimation possible
  return "";
}
