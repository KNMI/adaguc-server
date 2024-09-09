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

#include <iostream>
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

long long toSeconds(const TimeInterval &interval) {
  // Convert a TimeInterval to approximate seconds for sorting
  return interval.seconds + interval.minutes * 60 + interval.hours * 3600 + interval.days * 86400 + interval.months * 2592000 + interval.years * 31536000;
}

CT::string estimateISO8601Duration(const std::vector<CT::string> &timestamps, double threshold) {
  // Size considered too small to find a pattern
  if (timestamps.size() < 4) return CT::string("");

  std::vector<TimeInterval> intervals;
  CTime ctime;
  ctime.init("seconds since 1970", NULL);

  // Parse all timestamps into tm structs
  std::vector<CTime::Date> parsedTimes;
  for (const auto &timestamp : timestamps) {
    parsedTimes.push_back(ctime.ISOStringToDate(timestamp.c_str()));
  }

  // Calculate intervals between consecutive timestamps
  for (size_t i = 1; i < parsedTimes.size(); ++i) {
    intervals.push_back(calculateTimeInterval(parsedTimes[i - 1], parsedTimes[i]));
  }

  // Count occurrences of each interval in terms of total seconds
  // Count occurrences of each interval in terms of total seconds
  std::map<long long, int> intervalFrequency;
  std::map<long long, TimeInterval> intervalMap;
  for (const auto &interval : intervals) {
    long long intervalInSeconds = toSeconds(interval);
    intervalFrequency[intervalInSeconds]++;
    intervalMap[intervalInSeconds] = interval;
  }

  // Retrieve the most frequent and smallest interval
  // In case of missing incoming values, we will find some occurrences of
  // larger intervals.
  // Heuristic: This interval has to present itself at least 80% of the time
  long long mostFrequentIntervalInSeconds = 0;
  int maxFrequency = 0;
  //  Each entry has [intervalInSeconds,frequency]
  for (const auto &entry : intervalFrequency) {
    if (entry.second > maxFrequency || (entry.second == maxFrequency && entry.first < mostFrequentIntervalInSeconds)) {
      mostFrequentIntervalInSeconds = entry.first;
      maxFrequency = entry.second;
    }
  }

  // Check if all intervals are the same
  // const TimeInterval &firstInterval = intervals[0];
  // for (size_t i = 1; i < intervals.size(); ++i) {
  //   if (intervals[i].years != firstInterval.years || intervals[i].months != firstInterval.months || intervals[i].days != firstInterval.days || intervals[i].hours != firstInterval.hours ||
  //       intervals[i].minutes != firstInterval.minutes || intervals[i].seconds != firstInterval.seconds) {
  //     return CT::string(""); // No consistent interval found
  //   }
  // }

  // If all intervals are the same, convert the first interval to ISO8601 format
  // return toISO8601Interval(firstInterval);
  // Check if the most frequent interval meets the threshold
  // Check if the most frequent interval meets the threshold
  double frequencyPercentage = static_cast<double>(maxFrequency) / intervals.size();
  if (frequencyPercentage < threshold) {
    return CT::string(""); // No interval meets the threshold requirement
  }

  // Use the most frequent interval
  TimeInterval mostFrequentInterval = intervalMap[mostFrequentIntervalInSeconds];
  return toISO8601Interval(mostFrequentInterval);
}
