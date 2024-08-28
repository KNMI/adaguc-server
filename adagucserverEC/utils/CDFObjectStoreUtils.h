/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2024-08-28
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

#ifndef CDFOBJECTSTOREUTILS_H
#define CDFOBJECTSTOREUTILS_H

#include "CDataSource.h"

#define ADDDIMENSION_TYPE_REGEXP_EXTRACTTIME "filename_regexp_extracttime"

/**
 * Allows adding a custom dimension. This is useful for files which do not have a time dimension.
 * The dimension can be added and populated with information from for example the filename using regular expressions.
 *
 * The following configuration for a layer will do this for the file /data/adaguc-data/MTG/day_highres_knmi_weur_20240731_72.png.
 * The inforomation is extracted from the filename using regular expressions, see the AddDimension section.
 *

  <Layer type="database">
    <Name>day_highres_knmi_weur</Name>
    <Title>day_highres_knmi_weur</Title>
    <FilePath filter="^day_highres_knmi_weur_2.*\.png$">/data/adaguc-data/MTG/</FilePath>
    <AddDimension
      name="time"
      type="filename_regexp_extracttime"
      year='^.*\_(\d{4})\d{2}\d{2}_\d{2}\.png$'
      month='^.*\_\d{4}(\d{2})\d{2}_\d{2}\.png$'
      day='^.*\_\d{4}\d{2}(\d{2})_\d{2}\.png$'
      hour='^.*\_\d{4}\d{2}\d{2}_(\d{2})\.png$'
    />
    <Dimension name="time">time</Dimension>
    <Projection
        proj4="+proj=stere +ellps=WGS84 +lat_0=61.0 +lon_0=0.0 +units=m +no_defs"
        minx="-1450000.0"
        miny="0.0"
        maxx="1650000.0"
        maxy="-1850000.0"
    />
    <Variable>pngdata</Variable>
    <RenderMethod>rgba</RenderMethod>
  </Layer>

*
*/
void handleAddDimension(CDataSource *dataSource, CDFObject *cdfObject, CT::string fileLocationToOpen);

#endif