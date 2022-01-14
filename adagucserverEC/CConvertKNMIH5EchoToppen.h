/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Geo Spatial Team gstf@knmi.nl
 * Date:     2022-01-12
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

#ifndef CCONVERTKNMIH5ECHOTOPPEN_H
#define CCONVERTKNMIH5ECHOTOPPEN_H
#include "CDataSource.h"
class CConvertKNMIH5EchoToppen {
private:
  DEF_ERRORFUNCTION();
  static int checkIfKNMIH5EchoToppenFormat(CDFObject *cdfObject);

public:
  static int convertKNMIH5EchoToppenHeader(CDFObject *cdfObject);
  static int convertKNMIH5EchoToppenData(CDataSource *dataSource, int mode);
};
#endif
