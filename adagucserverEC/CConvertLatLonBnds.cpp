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

bool CConvertLatLonBnds::isThisLatLonBndsData(CDFObject *cdfObject) {
  CDF::Attribute *attr = cdfObject->getAttributeNE("USE_ADAGUC_LATLONBNDS_CONVERTER");
  if ((attr != NULL) && attr->getDataAsString().toLowerCase().equals("true")) {
    return true;
  }
  auto *pointLon = cdfObject->getVariableNE("lon_bnds");
  auto *pointLat = cdfObject->getVariableNE("lat_bnds");
  auto *gridpointDim = cdfObject->getDimensionNE("gridpoint");
  if (pointLon == nullptr || pointLat == nullptr || gridpointDim == nullptr) {
    return false;
  }

  auto *lonGridPointDim = pointLon->getDimensionNE("gridpoint");
  auto *latGridPointDim = pointLat->getDimensionNE("gridpoint");
  if (lonGridPointDim == nullptr || latGridPointDim == nullptr) {
    return false;
  }

  if ((pointLon->dimensionlinks.size() > 1) && (pointLon->dimensionlinks.size() > 1)) {
    cdfObject->setAttributeText("USE_ADAGUC_LATLONBNDS_CONVERTER", "true");
    return true;
  }

  return false;
};
