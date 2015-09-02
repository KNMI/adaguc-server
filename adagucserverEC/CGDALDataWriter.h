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

#include "Definitions.h"
#ifdef ADAGUC_USE_GDAL
#ifndef CGDALDataWriter_H
#define CGDALDataWriter_H
#include "CServerParams.h"
#include "CDataSource.h"
#include "CDataReader.h"
#include "gdalwarper.h"
#include <gdal.h>
#include <gdal_pam.h>
#include <gdal_priv.h>
#include <gdal_frmts.h>
#include <cpl_string.h>
#include <ogr_srs_api.h>
#include <cpl_conv.h>
#include <ogr_spatialref.h>
#include <ctype.h>
#include "CDebugger.h"
#include "CTypes.h"
#include "CServerError.h"
#include "CDrawImage.h"

#include "CImageWarper.h"

#include "CXMLGen.h"

#include "CDataReader.h"

#include "CIBaseDataWriterInterface.h"
#define MAX_STR_LEN 8191

class CGDALDataWriter: public CBaseDataWriterInterface{
  private:
    CT::string driverName;
    class Settings{
    public:
      size_t width;
      size_t height;
      void *data;
    };


    template <class T>
    static void drawFunction(int x,int y,T val, void *_settings){
      Settings*settings = (Settings*)_settings;
      if(x>=0&&y>=0&&x<(int)settings->width&&y<(int)settings->height){
        ((T*)settings->data)[x+y*settings->width]=val;
      }
    };

    
    CServerParams *srvParam;
    GDALDriverH  hMemDriver2,hOutputDriver;
    GDALDatasetH destinationGDALDataSet,hOutputDS;
    double adfDstGeoTransform[6];
    double dfDstBBOX[4];
    double dfSrcBBOX[4];
    CDataReader reader;
    GDALDataType datatype;
    CT::string tmpFileName;
    GDALWarpOptions *psWarpOptions;
    int currentBandNr;
    int NrOfBands;
    CT::string mimeType;
    CT::string customOptions;
    CT::string * InputProducts;
    CT::string TimeUnit;
    char szTemp[MAX_STR_LEN+1];
    char szTemp2[MAX_STR_LEN+1];
    double dfNoData;
    DEF_ERRORFUNCTION();
    CDataSource *_dataSource;
    void generateUniqueGetCoverageFileName(char *pszTempFileName);
    CT::string generateGetCoverageFileName();
    void generateString(char *s, const int len);
    
    CT::string getDimensionValue(int d,CCDFDims *dims){
      
      CT::string value;
      if(dims->isTimeDimension(d)){
      CTime adagucTime;
        try{
          value = "0";
          adagucTime.init(TimeUnit.c_str());
          double offset = adagucTime.dateToOffset(adagucTime.ISOStringToDate(dims->getDimensionValue(d).c_str()));
          value.print("%f",offset);
        }catch(int e){
        }
      }else{
        value.print("%s",dims->getDimensionValue(d).c_str());
      }
      return value;
    }
    
  public:
    CGDALDataWriter(){
      InputProducts=NULL;
    }
    ~CGDALDataWriter(){
      if(InputProducts!=NULL)delete[] InputProducts;InputProducts=NULL;
    }
    // Virtual functions
    int init(CServerParams *srvParam,CDataSource *dataSource, int nrOfBands);
    int addData(std::vector <CDataSource*> &dataSources);
    int end();
};
#endif
#endif
