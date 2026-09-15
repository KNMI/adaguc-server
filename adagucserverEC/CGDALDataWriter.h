/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl, GST - GeoSpatialTeam KNMI
 * Date:     2026-09-10
 *
 ******************************************************************************
 *
 * Copyright 2026, Royal Netherlands Meteorological Institute (KNMI)
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

#ifdef ADAGUC_USE_GDAL
#ifndef CGDALDataWriter_H
#define CGDALDataWriter_H
#include <gdal.h>
#include <gdal_pam.h>
#include <gdal_priv.h>
#include <gdal_frmts.h>
#include <cpl_string.h>
#include "CIBaseDataWriterInterface.h"
#include <ogr_srs_api.h>
#include <cpl_conv.h>
#include <ogr_spatialref.h>
#include <ctype.h>


#include "CXMLGen.h"


#define MAX_STR_LEN 8191

class CGDALDataWriter : public CBaseDataWriterInterface {
private:
  std::string driverName;
  struct GdalDrawFunctionState {
    size_t width;
    size_t height;
    void *data;
  };

  template <class T> static void drawFunction(int x, int y, T val, GDWState &, GdalDrawFunctionState &drawFunctionState) {
    if (x >= 0 && y >= 0 && x < (int)drawFunctionState.width && y < (int)drawFunctionState.height) {
      ((T *)drawFunctionState.data)[x + y * drawFunctionState.width] = val;
    }
  }

  CServerParams *srvParam;
  GDALDriverH hMemDriver2, hOutputDriver;
  GDALDatasetH destinationGDALDataSet, hOutputDS;
  double adfDstGeoTransform[6];
  double dfDstBBOX[4];
  double dfSrcBBOX[4];
  CDataReader reader;
  GDALDataType datatype;

  int currentBandNr;
  int NrOfBands;
  std::string mimeType;
  std::string customOptions;
  std::string *InputProducts;
  std::string TimeUnit;
  double dfNoData;

  CDataSource *_dataSource;

  std::string generateGetCoverageFileName();
  std::string getDimensionValue(int d, CCDFDims *dims);

public:
  CGDALDataWriter() { InputProducts = NULL; }
  ~CGDALDataWriter() {
    if (InputProducts != NULL) {
      delete[] InputProducts;
    };
    InputProducts = NULL;
  }
  // Virtual functions
  int init(CServerParams *srvParam, CDataSource *dataSource, int nrOfBands);
  int addData(std::vector<CDataSource *> &dataSources);
  int end();

  friend std::string generateUniqueGetCoverageFileName(CGDALDataWriter *dataWriter);
};
#endif
#endif
