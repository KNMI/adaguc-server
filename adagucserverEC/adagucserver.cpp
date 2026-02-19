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
#include <cdfVariableCache.h>
#include "fork_server.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

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

int run_adaguc_once(int argc, char **argv, char **envp, bool is_forked) {
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

  proj_clear_cache();
  BBOXProjectionClearCache();

  closeLogFile();
  varCacheClear();

  return status;
}

int main(int argc, char **argv, char **envp) {
  // TODO:
  // If these lines are commented out, the calls the /edr/collections/instances/<my-instance> fail to return data
  // because the call to `request=getreferencetimes` does not contain useful output
  // Also regular calls (e.g. GetMap) include logging statements in the image response
  setvbuf(stdout, NULL, _IONBF, 0); // turn off buffering
  setvbuf(stderr, NULL, _IONBF, 0); // turn off buffering

  const char *ADAGUC_FORK_SOCKET_PATH = getenv("ADAGUC_FORK_SOCKET_PATH");
  if (ADAGUC_FORK_SOCKET_PATH != NULL) {
    return run_as_fork_service(run_adaguc_once, argc, argv, envp);
  } else {
    // normal flow without unix socket server/fork
    return run_adaguc_once(argc, argv, envp, false);
  }
}
