

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

#ifndef Definitions_H
#define Definitions_H

#define ADAGUCSERVER_VERSION "4.1.0" // Please also update in the Dockerfile to the same version

// CConfigReaderLayerType
#define CConfigReaderLayerTypeUnknown 0
#define CConfigReaderLayerTypeDataBase 1
#define CConfigReaderLayerTypeStyled 2
#define CConfigReaderLayerTypeCascaded 3
#define CConfigReaderLayerTypeBaseLayer 4
#define CConfigReaderLayerTypeLiveUpdate 6

// CRequest
// Service options
#define SERVICE_WMS 1
#define SERVICE_WCS 2
#define SERVICE_METADATA 3

// Request options
#define REQUEST_WMS_GETCAPABILITIES 1
#define REQUEST_WMS_GETMAP 2
#define REQUEST_WMS_GETLEGENDGRAPHIC 3
#define REQUEST_WMS_GETFEATUREINFO 4
#define REQUEST_WCS_GETCAPABILITIES 5
#define REQUEST_WCS_DESCRIBECOVERAGE 6
#define REQUEST_WCS_GETCOVERAGE 7
#define REQUEST_WMS_GETMETADATA 8
#define REQUEST_WMS_GETPOINTVALUE 10
#define REQUEST_WMS_GETREFERENCETIMES 11
#define REQUEST_WMS_GETHISTOGRAM 12
#define REQUEST_WMS_GETSCALEBAR 13
#define REQUEST_METADATA_GETMETADATA 14
#define REQUEST_UPDATEDB 100

// Legend
#define LEGEND_WIDTH 300
#define LEGEND_HEIGHT 400

// WMS versions
#define WMS_VERSION_1_0_0 1
#define WMS_VERSION_1_1_1 2
#define WMS_VERSION_1_3_0 3

// Exceptions
#define EXCEPTIONS_PLAINTEXT 0
#define WMS_EXCEPTIONS_IMAGE 1
#define WMS_EXCEPTIONS_BLANKIMAGE 2
#define WMS_EXCEPTIONS_XML_1_1_1 3
#define WCS_EXCEPTIONS_XML_1_0_0 4
#define WMS_EXCEPTIONS_XML_1_3_0 5

// ADAGUC Internal exceptions
#define CEXCEPTION_NULLPOINTER 1000

// Image formats
#define IMAGEFORMAT_IMAGEPNG8 0
#define IMAGEFORMAT_IMAGEGIF 1
#define IMAGEFORMAT_IMAGEPNG24 2
#define IMAGEFORMAT_IMAGEPNG32 3
#define IMAGEFORMAT_IMAGEWEBP 4
#define IMAGEFORMAT_IMAGEPNG8_NOALPHA 5

#define SERVERIMAGEMODE_8BIT 0
#define SERVERIMAGEMODE_RGBA 1

// WCS Versions
#define WCS_VERSION_1_0 1

#define DEFAULT_FONT "./fonts/FreeSans.ttf"

// CNetCDFReader opening options
#define CNETCDFREADER_MODE_OPEN_HEADER 1     /* Only set the header in the cdfObject, like dimensions, variables and attributes. Data can still be undefined */
#define CNETCDFREADER_MODE_OPEN_ALL 2        /* Open all, including data */
#define CNETCDFREADER_MODE_GET_METADATA 3    /* Process only data needed for metadata */
#define CNETCDFREADER_MODE_OPEN_DIMENSIONS 4 /* Open dimensions only, like used in the WMS GetCapabilities generation of CXMLGen.*/
#define CNETCDFREADER_MODE_OPEN_EXTENT 5     /* Open a subset of the grid, read data for given extent (bbox) */

// Web Coverage restriction and Get Feature Info restriction
#define ALLOW_NONE 1
#define ALLOW_WCS 2
#define ALLOW_GFI 4
#define ALLOW_METADATA 8
#define SHOW_QUERYINFO 16

#define MAX_IMAGE_WIDTH 16384
#define MAX_IMAGE_HEIGHT 16384

#define RENDERHINT_DISCRETECLASSES "discreteclasses"

#define ADAGUC_USE_CAIRO

// #define CImgWarpBilinear_DEBUG
// #define CImgWarpBilinear_TIME
// #define MEASURETIME

// #define CDATAREADER_DEBUG
// #define CCDFNETCDFIO_DEBUG
//  Debug settings:
/*
#define CIMAGEDATAWRITER_DEBUG
#define CDATAREADER_DEBUG

#define CImgWarpBilinear_DEBUG
#define CImgWarpBilinear_TIME

//Log a summary of different time durations during execution
//#define MEASURETIME

//#define CXMLGEN_DEBUG
//#define CCDFNETCDFIO_DEBUG
#define CDATAREADER_DEBUG
#define CDATASOURCE_DEBUG

#define CDATAREADER_DEBUG
#define CCDFNETCDFIO_DEBUG
#define CXMLGEN_DEBUG
#define CIMAGEDATAWRITER_DEBUG
#define CXMLGEN_DEBUG
#define MEASURETIME
*/

// #define MEASURETIME
// #define ADAGUC_TILESTITCHER_DEBUG
// #define DEBUGON
#ifdef DEBUGON
#define CDATASOURCE_DEBUG
#define CREQUEST_DEBUG
#define CIMAGEDATAWRITER_DEBUG
#define CDATAREADER_DEBUG
#define CXMLGEN_DEBUG
#define CCDFNETCDFIO_DEBUG
#define CCONVERTASCAT_DEBUG
#define MEASURETIME
#define CIMGWARPNEARESTNEIGHBOUR_DEBUG
#define CDFOBJECTSTORE_DEBUG
#define ADAGUC_TILESTITCHER_DEBUG
#endif
// #define MEASURETIME
// #define CCDFNETCDFIO_DEBUG
// #define CXMLGEN_DEBUG
// #define CDATAREADER_DEBUG
// #define CCDFNETCDFIO_DEBUG
// #define CCDFNETCDFIO_DEBUG_OPEN
// #define CDATAREADER_DEBUG

// #define MEASURETIME
// #define CDATAREADER_DEBUG
// #define CIMAGEDATAWRITER_DEBUG
// #define CDATASOURCE_DEBUG
#define CSLD_DEBUG

// #define ENABLE_CURL in Makefile

// #define ADAGUC_USE_GDAL
#endif

// HTTP status codes
// Success
#define HTTP_STATUSCODE_200_OK 0
// Client errors
#define HTTP_STATUSCODE_404_NOT_FOUND 32
#define HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY 33

#define LOCK_METADATATABLE_ID 1

#ifndef rad2deg
#define rad2deg (180. / M_PI) // conversion for rad to deg
#endif

#ifndef deg2rad
#define deg2rad (M_PI / 180.) // conversion for deg to rad
#endif

#define radians(aDeg) (deg2rad * aDeg)
