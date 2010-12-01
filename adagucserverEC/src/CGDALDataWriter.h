#ifndef CGDALDataWriter_H
#define CGDALDataWriter_H
#include "CServerParams.h"
#include "CDataSource.h"
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
#include "CTypes.h"
#include "CServerError.h"
#include "CDrawImage.h"

#include "CImageWarper.h"

#include "CXMLGen.h"
#include "Definitions.h"
#include "CPGSQLDB.h"
#include "CDataReader.h"
#include "CADAGUC_time.h"

#include "CIBaseDataWriterInterface.h"


class CGDALDataWriter: public CBaseDataWriterInterface{
  private:
    CServerParams *srvParam;
    GDALDriverH  hMemDriver1,hMemDriver2,hOutputDriver;
    GDALDatasetH hMemDS1,hMemDS2,hOutputDS;
    double adfDstGeoTransform[6];
    double adfSrcGeoTransform[6];
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
    //int nrMetadataItems;
    //CT::string * Metadata;
    std::vector <CT::string *> metaDataList;
    CT::string * Times;
    CT::string * InputProducts;
    CT::string TimeUnit;
    char szTemp[MAX_STR_LEN+1];
    char szTemp2[MAX_STR_LEN+1];
    double dfNoData;
    DEF_ERRORFUNCTION();
    CDataSource *_dataSource;
    void generateGetCoverageFileName(char *pszTempFileName);
    void generateString(char *s, const int len);
  public:
    CGDALDataWriter(){
      Times=NULL;
      InputProducts=NULL;
    }
    ~CGDALDataWriter(){
      for(size_t j=0;j<metaDataList.size();j++){
        delete metaDataList[j];
      }
      
      if(Times!=NULL)delete[] Times;Times=NULL;
      if(InputProducts!=NULL)delete[] InputProducts;InputProducts=NULL;
    }
    // Virtual functions
    int init(CServerParams *srvParam,CDataSource *dataSource, int nrOfBands);
    int addData(std::vector <CDataSource*> &dataSources);
    int end();
};
#endif
