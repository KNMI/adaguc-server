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
#include "traceTimings.h"

TEST(TraceTimings, traceTimingsEnableAndInit) {
  traceTimingsEnableAndInit();
  auto report = traceTimingsGetReport();
  CHECK(report.startsWith(",[total") == true);

  traceTimingsSpanStart(TraceTimingType::WARPIMAGE);
  traceTimingsSpanStart(TraceTimingType::DBCHECKTABLE);
  traceTimingsSpanStart(TraceTimingType::WARPIMAGE);
  traceTimingsSpanEnd(TraceTimingType::DBCHECKTABLE);
  traceTimingsSpanStart(TraceTimingType::DB);
  traceTimingsSpanEnd(TraceTimingType::DB);
  traceTimingsSpanStart(TraceTimingType::DBCHECKTABLE);
  traceTimingsSpanEnd(TraceTimingType::DBCHECKTABLE);
  traceTimingsSpanEnd(TraceTimingType::WARPIMAGE);
  traceTimingsSpanStart(TraceTimingType::DBCHECKTABLE);
  traceTimingsSpanEnd(TraceTimingType::DBCHECKTABLE);
  traceTimingsSpanEnd(TraceTimingType::WARPIMAGE);

  // Closed one to many
  traceTimingsSpanEnd(TraceTimingType::WARPIMAGE);

  // Never started
  traceTimingsSpanEnd(TraceTimingType::FSOPEN);

  auto mapReport = traceTimingsGetMap();
  CHECK(mapReport[TraceTimingType::WARPIMAGE].numevents == 2);
  CHECK(mapReport[TraceTimingType::DB].numevents == 1);
  CHECK(mapReport[TraceTimingType::DBCHECKTABLE].numevents == 3);
}

int main() {
  TestResult tr;
  TestRegistry::runAllTests(tr);
  if (tr.failureCount != 0) {
    return 1;
  }
  return 0;
}
