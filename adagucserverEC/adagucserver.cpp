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

#include "adagucserver.h"
#include "CDebugger.h"
#include "utils/ConfigurationUtils.h"
#include "utils/writelogfile.h"
#include "utils/processCMDArgs.h"
#include "traceTimings/traceTimings.h"
#include "CReportWriter.h"
#include "CDBFactory.h"
#include "ProjCache.h"
#include "Types/ProjectionStore.h"
#include <cmath>
#include "CXMLSerializerInterface.h"
#include "CServerParams.h"
#include "CServerConfig_CPPXSD.h"
#include "CDebugger.h"
#include "Definitions.h"
#include "CTString.h"
#include "CCDFDataModel.h"
#include "COGCDims.h"
#include "CStopWatch.h"
#include "Types/CPointTypes.h"

#include "CStyleConfiguration.h"

#include "CGeoJSONData.h"
#include <proj.h>

int processQueryStringRequest() {
  /* Process the OGC request */
  setErrorFunction(serverErrorFunction);
  setWarningFunction(serverWarningFunction);
  setDebugFunction(serverDebugFunction);
  CRequest request;
  if (setCRequestConfigFromEnvironment(&request) != 0) {
    CDBError("Unable to read configuration file.");
  }
  request.runRequest();
  readyerror();
  return getStatusCode();
}

int main(int argc, char **argv, char **envp) {
  traceTimingsCheckEnabled();
  checkLogSettings();

  int status = processCMDArgs(argc, argv, envp);

  // Not a CMD line request, process as query string request
  if (status == -1) {
    status = processQueryStringRequest();
  }

  /* Print the check report formatted as JSON. */
  CReportWriter::writeJSONReportToFile();

  CCachedDirReader::free();

  CTime::cleanInstances();

  CDFObjectStore::getCDFObjectStore()->clear();

  CDBFactory::clear();

  BBOXProjectionClearCache();

  closeLogFile();

  return status;
}
