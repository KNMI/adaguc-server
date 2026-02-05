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

#ifndef CAUTOCONFIGURE_H
#define CAUTOCONFIGURE_H
#include <cmath>
#include "CDebugger.h"
#include "CDataSource.h"
#include "CServerError.h"
#include "CDirReader.h"

// #include "CADAGUC_time.h"
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFHDF5IO.h"
#include "CProj4ToCF.h"
#include "CStopWatch.h"
#include <sys/stat.h>
#include "CDBFileScanner.h"
#include "CDFObjectStore.h"
#include "CDataPostProcessors/CDataPostProcessor.h"

class CAutoConfigure {
private:
  /**
   * Load a generic file header into the datasource. Usually the most recent file from a series is taken. The file header can for example be used to determine automatically the available dimensions.
   * @param dataSource
   * @return Zero on success.
   */
  static int justLoadAFileHeader(CDataSource *dataSource);

public:
  static int autoConfigureDimensions(CDataSource *dataSource);
  static int autoConfigureStyles(CDataSource *dataSource);

  /**
   * Find the default filename for a datasource from the database
   * @param dataSource
   * @param fileName - The filename to return
   * @returns Zero on success.
   */
  static int getFileNameForDataSource(CDataSource *dataSource, std::string &fileName);
};
#endif
