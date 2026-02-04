/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  Class with static functions to create tiles for files in the db.
 * Author:   Maarten Plieger, maarten.plieger "at" knmi.nl
 * Date:     2021-12-23
 *
 ******************************************************************************
 *
 * Copyright 2021, Royal Netherlands Meteorological Institute (KNMI)
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

#ifndef CCREATETILES_H
#define CCREATETILES_H

#include "CDebugger.h"
#include "CDataSource.h"

/**
 * @brief Class with static functions to create tiles for files in the db.
 *
 */
class CCreateTiles {
public:
  /**
   * @brief Create tiles for all files.
   *
   * @param dataSource
   * @param scanFlags
   * @return int
   */
  static int createTiles(CDataSource *dataSource, int scanFlags);

  /**
   * @brief Create tiles For a single file
   *
   * @param dataSource
   * @param scanFlags
   * @param fileToTile
   * @return int
   */
  static int createTilesForFile(CDataSource *dataSource, int scanFlags, CT::string fileToTile);
};

#endif
