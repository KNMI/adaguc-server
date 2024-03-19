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

#include "CConvertLatLonBnds.h"
#include "CFillTriangle.h"
#include "CImageWarper.h"

// #define CConvertLatLonBnds_DEBUG

const char *CConvertLatLonBnds::className = "CConvertLatLonBnds";

bool CConvertLatLonBnds::isThisLatLonBndsData(CDFObject *cdfObject) {
  try {
    CDF::Attribute *attr = cdfObject->getAttributeNE("USE_ADAGUCCONVERTER");
    if ((attr != NULL) && (attr->getDataAsString().equals("LatLonBnds") == true)) {
      return true;
    }
  } catch (int e) {
    // Go on
  }
  try {
    CDF::Variable *pointLon = cdfObject->getVariable("lon_bnds");
    CDF::Variable *pointLat = cdfObject->getVariable("lat_bnds");
  } catch (int e) {
    return false;
  }
  try {
    CDF::Dimension *gridpointDim = cdfObject->getDimension("gridpoint");
    CDF::Variable *pointLon = cdfObject->getVariable("lon");
    CDF::Variable *pointLat = cdfObject->getVariable("lat");
    pointLon->getDimension("gridpoint");
    pointLat->getDimension("gridpoint");
    if ((pointLon->dimensionlinks.size() == 1) && (pointLon->dimensionlinks.size() == 1)) {
      return true;
    }

  } catch (int e) {
  }
  return false;
};
