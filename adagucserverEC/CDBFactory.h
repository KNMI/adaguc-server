/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2015-05-06
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

#ifndef CDBFACTORY_H
#define CDBFACTORY_H

#include "CDBAdapter.h"
#include "CDBAdapterPostgreSQL.h"

class CDBFactory {
private:
private:
  DEF_ERRORFUNCTION();

public:
  static CDBAdapter *staticCDBAdapter;

  static CDBAdapter *getDBAdapter(CServerConfig::XMLE_Configuration *cfg);
  static void clear();
};

#endif