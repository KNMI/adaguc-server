#ifndef CImageWarper_H
#define CImageWarper_H
#include "CServerParams.h"
#include "CDataReader.h"
#include "CDrawImage.h"
#include "CDefinitions.h"
#include <proj_api.h>
#include <math.h>
#include "CDebugger.h"
#include "CStopWatch.h"

void floatToString(char * string,size_t maxlen,float number);
class CImageWarper{
//  CNetCDFReader reader;
  private:

    double dfMaxExtent[4];
    int dMaxExtentDefined;
    bool convertRadiansDegreesDst,convertRadiansDegreesSrc,requireReprojection;
    DEF_ERRORFUNCTION();
    unsigned char pixel;
    CDataSource * _sourceImage;
    CGeoParams * _geoDest;
    CT::string destinationCRS;
    int _decodeCRS(CT::string *CRS);
    std::vector <CServerConfig::XMLE_Projection*> *prj;
  public:
    CImageWarper(){
      prj=NULL;
    }
    projPJ sourcepj,destpj,latlonpj;
    int initreproj(CDataSource *sourceImage,CGeoParams *GeoDest,std::vector <CServerConfig::XMLE_Projection*> *prj);
    int closereproj();
    int reprojpoint(double &dfx,double &dfy);
    int reprojpoint_inv(double &dfx,double &dfy);
    void reprojBBOX(double *df4PixelExtent);
    int reprojfromLatLon(double &dfx,double &dfy);
    int reprojToLatLon(double &dfx,double &dfy);
    int decodeCRS(CT::string *outputCRS, CT::string *inputCRS);
    int decodeCRS(CT::string *outputCRS, CT::string *inputCRS,std::vector <CServerConfig::XMLE_Projection*> *prj);
    int findExtent(CDataSource *sourceImage,double * dfBBOX);
    bool isProjectionRequired(){return requireReprojection;}
    void floatToString(char * string,size_t maxlen,float number);
  };
  

#endif
