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
  /**
   * @brief Quickly checks if the format is suitable for this converter
   *
   * @param cdfObject
   * @return int zero when this is indeed a echotoppen HDF5 file.
   */
  static int checkIfKNMIH5EchoToppenFormat(CDFObject *cdfObject);

  /**
   * @brief Calculate the flight level based on the given echotoppen height from the HDF5 file
   *
   * @param height
   * @return int
   */
  static int calcFlightLevel(float height);

public:
  /**
   * @brief Populate the cdfObject, by defining dimensions, variables and attributes, should not yet read any data. This function adjusts the cdfObject by creating a virtual 2D variable named
   * echotoppen.
   *
   * @param cdfObject
   * @return int
   */
  static int convertKNMIH5EchoToppenHeader(CDFObject *cdfObject);

  /**
   * @brief Set the data for the variable in screenspace coordinates as defined in dataSource->srvParams->geoParams. Wheter all data is read or only the CDM structure depends on the mode.
   *
   * @param dataSource
   * @param mode CNETCDFREADER_MODE_OPEN_HEADER or CNETCDFREADER_MODE_OPEN_ALL
   * @return int
   */
  static int convertKNMIH5EchoToppenData(CDataSource *dataSource, int mode);
};
#endif
