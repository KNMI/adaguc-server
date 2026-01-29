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

#include "CDebugger.h"
#include "CppUnitLite/TestHarness.h"
#include "CTString.h"
#include "timeutils.h"
#include "Types/LayerMetadataType.h"
#include "utils/XMLGenUtils.h"

TEST(CalculateTimeInterval, TimeUtils) {
  CTime::Date start = {};
  start.minute = 30;
  start.hour = 15;
  start.day = 29;
  start.month = 8;
  start.year = 2024;
  start.second = 0.0;

  CTime::Date end = {};
  end.second = 45.0;
  end.minute = 45;
  end.hour = 16;
  end.day = 29;
  end.month = 8;
  end.year = 2025;

  TimeInterval expected = {1, 0, 0, 1, 15, 45};
  TimeInterval result = calculateTimeInterval(start, end);

  CHECK(result.years == expected.years);
  CHECK(result.months == expected.months);
  CHECK(result.days == expected.days);
  CHECK(result.hours == expected.hours);
  CHECK(result.minutes == expected.minutes);
  CHECK(result.seconds == expected.seconds);
}

TEST(ToISO8601Interval, TimeUtils) {
  TimeInterval interval = {1, 2, 10, 5, 30, 45};
  CT::string expected("P1Y2M10DT5H30M45S");

  CT::string result = toISO8601Interval(interval);
  CHECK(result == expected);
}

TEST(ToSeconds, TimeUtils) {
  TimeInterval interval = {0, 1, 0, 1, 0, 20}; // 1 month, 1 hour, 20 seconds
  long expected = 2595620;

  long result = interval.toSeconds();
  CHECK(result == expected);
}

TEST(EstimateISO8601Duration, TimeUtils) {
  // Most straightforward case
  std::vector<CT::string> timestamps = {"2024-07-29T15:30:45Z", "2024-07-29T16:30:45Z", "2024-07-29T17:30:45Z", "2024-07-29T18:30:45Z"};
  CT::string expected("PT1H");
  CT::string result = estimateISO8601Duration(timestamps);
  std::cout << "Result: " << result.c_str() << "\n";
  CHECK(result == expected);

  // Case with one timestep missing
  std::vector<CT::string> timestamps_with_gap = {"2024-07-29T15:30:45Z", "2024-07-29T16:30:45Z", "2024-07-29T18:30:45Z", "2024-07-29T19:30:45Z",
                                                 "2024-07-29T20:30:45Z", "2024-07-29T21:30:45Z", "2024-07-29T22:30:45Z", "2024-07-29T23:30:45Z"};
  CT::string expected_with_gap("PT1H");
  CT::string result_with_gap = estimateISO8601Duration(timestamps_with_gap);
  CHECK(result_with_gap == expected_with_gap);

  // Case in February of a leap year
  std::vector<CT::string> leap_year_timestamps = {"2024-02-28T23:30:45Z", "2024-02-29T00:30:45Z", "2024-02-29T01:30:45Z", "2024-02-29T02:30:45Z"};
  CT::string expected_leap_year("PT1H");
  CT::string result_leap_year = estimateISO8601Duration(leap_year_timestamps);
  CHECK(result_leap_year == expected_leap_year);

  // Case of a complex interval (5 days and 10 hours)
  std::vector<CT::string> complex_interval_timestamps = {"2024-07-01T08:00:00Z", "2024-07-06T18:00:00Z", "2024-07-12T04:00:00Z", "2024-07-17T14:00:00Z", "2024-07-23T00:00:00Z"};
  CT::string expected_complex_interval("P5DT10H");
  CT::string result_complex_interval = estimateISO8601Duration(complex_interval_timestamps);
  CHECK(result_complex_interval == expected_complex_interval);
}

TEST(checkDependenciesBetweenDims, TimeUtils) {
  int result = 0;
  MetadataLayer ml;
  ml.dataSource = new CDataSource();
  ml.dataSource->cfgLayer = new CServerConfig::XMLE_Layer();

  auto timeDim = new CServerConfig::XMLE_Dimension();
  timeDim->value = "time";
  ml.dataSource->cfgLayer->Dimension.push_back(timeDim);
  result = checkDependenciesBetweenDims(&ml);
  CHECK(result == XMLGENUTILS_CHECKDEP_DATASOURCE_NO_ISO_DURATION)
  timeDim->attr.defaultV = "reference_time+PT1H";

  result = checkDependenciesBetweenDims(&ml);
  CHECK(result == XMLGENUTILS_CHECKDEP_DATASOURCE_NO_DIMS_IN_LAYERMETADATA)

  ml.layerMetadata.dimList.push_back({.serviceName = "time", .cdfName = "", .units = "", .values = "", .defaultValue = "?", .type = "", .hasMultipleValues = 1, .hidden = false});
  result = checkDependenciesBetweenDims(&ml);
  CHECK(result == XMLGENUTILS_CHECKDEP_DATASOURCE_NO_DIMS_IN_LAYERMETADATA)
  CHECK(ml.layerMetadata.dimList[0].defaultValue.equals(""));

  auto refTimeDim = new CServerConfig::XMLE_Dimension();
  refTimeDim->value = "reference_time";
  ml.dataSource->cfgLayer->Dimension.push_back(refTimeDim);

  result = checkDependenciesBetweenDims(&ml);
  CHECK(result == XMLGENUTILS_CHECKDEP_DATASOURCE_NO_DIMS_IN_LAYERMETADATA)

  ml.layerMetadata.dimList.push_back(
      {.serviceName = "reference_time", .cdfName = "", .units = "", .values = "", .defaultValue = "2025-05-10T12:00:00Z", .type = "", .hasMultipleValues = 1, .hidden = false});
  result = checkDependenciesBetweenDims(&ml);
  CHECK(result == 0)
  // New default value should be 1 hour more then default value of reference time
  CHECK(ml.layerMetadata.dimList[0].defaultValue.equals("2025-05-10T13:00:00Z") == true)

  // Try other cases
  ml.layerMetadata.dimList[1].defaultValue = "2025-05-10T08:00:00Z";
  result = checkDependenciesBetweenDims(&ml);
  CHECK(result == 0)
  CHECK(ml.layerMetadata.dimList[0].defaultValue.equals("2025-05-10T09:00:00Z") == true)

  // Try other cases
  ml.layerMetadata.dimList[1].defaultValue = "2025-05-10T23:00:00Z";
  result = checkDependenciesBetweenDims(&ml);
  CHECK(result == 0)
  CHECK(ml.layerMetadata.dimList[0].defaultValue.equals("2025-05-11T00:00:00Z") == true)

  // Try other cases
  ml.layerMetadata.dimList[1].defaultValue = "2025-05-10T18:00:00Z";
  result = checkDependenciesBetweenDims(&ml);
  CHECK(result == 0)
  CHECK(ml.layerMetadata.dimList[0].defaultValue.equals("2025-05-10T19:00:00Z") == true)

  timeDim->attr.defaultV = "reference_time+PT1S";
  ml.layerMetadata.dimList[1].defaultValue = "2025-05-10T23:00:59Z";
  result = checkDependenciesBetweenDims(&ml);
  CHECK(result == 0)
  CHECK(ml.layerMetadata.dimList[0].defaultValue.equals("2025-05-10T23:01:00Z") == true)

  ml.layerMetadata.dimList[1].defaultValue = "2025-05-10T23:59:59Z";
  result = checkDependenciesBetweenDims(&ml);
  CHECK(result == 0)
  CHECK(ml.layerMetadata.dimList[0].defaultValue.equals("2025-05-11T00:00:00Z") == true)

  timeDim->attr.defaultV = "reference_time+PT3600S";
  ml.layerMetadata.dimList[1].defaultValue = "2025-05-10T18:00:00Z";
  result = checkDependenciesBetweenDims(&ml);
  CHECK(result == 0)
  CHECK(ml.layerMetadata.dimList[0].defaultValue.equals("2025-05-10T19:00:00Z") == true)

  timeDim->attr.defaultV = "reference_time";
  ml.layerMetadata.dimList[1].defaultValue = "2025-05-10T18:00:00Z";
  result = checkDependenciesBetweenDims(&ml);
  CHECK(result == XMLGENUTILS_CHECKDEP_DATASOURCE_NO_ISO_DURATION)

  timeDim->attr.defaultV = "reference_time+PT0H";
  ml.layerMetadata.dimList[1].defaultValue = "2025-05-10T18:00:00Z";
  result = checkDependenciesBetweenDims(&ml);
  CHECK(result == 0)
  CHECK(ml.layerMetadata.dimList[0].defaultValue.equals("2025-05-10T18:00:00Z") == true)
}

int main() {
  TestResult tr;
  TestRegistry::runAllTests(tr);
  if (tr.failureCount != 0) {
    return 1;
  }
  return 0;
}
